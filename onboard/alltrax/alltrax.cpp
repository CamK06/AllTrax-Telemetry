#include "alltrax.h"
#include "alltraxvars.h"
#include <thread>
#include <fstream>
#include "../util.h"
#include <iostream>

namespace Alltrax
{
// General vars
hid_device* motorController = nullptr;
bool useChecksum = true; // This is always true in our controller
bool fakeData = false;
unsigned char rxBuf[64];

// Monitor mode vars
mcu_mon_callback_t rxCallback = nullptr;
bool monThreadRunning = false;
std::thread monThread;
int monDelay = 0;

bool initMotorController(bool useFakeData)
{
    // Warn the user if the receive callback has not been set
    if(rxCallback == nullptr)
        flog::warn("Receive callback is not set! No data will be received from the motor controller!");

    // Attempt to open the motor controller from its VID and PID
    if(!useFakeData) {
        flog::info("Searching for motor controller...");
        hid_init();
        
        motorController = hid_open(ALLTRAX_VID, ALLTRAX_PID, NULL);
        if(!motorController) {
            flog::error("Controller not found!");
            flog::error("Exiting...");
            hid_exit();
            return false;
        }
    }

    // Read the controllers basic info, return a bool indicating if the read succeeded or not
    fakeData = useFakeData;
    return getInfo();
}

bool sendData(char reportID, uint addressFunction, unsigned char* data, unsigned char length)
{
    // Properly format the address function
    unsigned char address[4] = {
        0, 0, 0, (unsigned char)addressFunction
    };
    address[0] = (unsigned char)(addressFunction/256u/256u/256u);
    address[1] = (unsigned char)(addressFunction/256u/256u);
    address[2] = (unsigned char)(addressFunction/256u);

    // Build the packet to send
    unsigned char* packet = new unsigned char[64];
    for(int i = 0; i < 64; i++)
        packet[i] = 0x00;
    packet[0] = reportID;
    packet[1] = length;
    for(int i = 0; i < 4; i++)
        packet[4+i] = address[i];

    // Checksum calculation
    if(useChecksum) {
        int num = (int)(packet[0] + packet[1]);
        for(int i = 4; i < 64; i++)
            num += (int)packet[i];
        packet[2] = (unsigned char)num;
        packet[3] = (unsigned char)(num / 256);
    }

    // Add data to the packet
    if(data != nullptr)
        for(int i = 0; i < length; i++)
            packet[i+1] = data[i];
            
    // Cleanup
    delete[] data;

    // Send the packet
    int result = hid_write(motorController, packet, 64);
    if(result == -1)
        return false;
    return true;
}

bool getInfo()
{
    bool result = false;
    if(!fakeData) {
        result = readVars(Vars::infoVars, 3); // TODO: Fix tcache chunk error when reading all vars
        flog::debug("Motor controller model: {}", Vars::model.getValue());
        flog::debug("Motor controller build date: {}", Vars::buildDate.getValue());
        flog::debug("Motor controller serial number: {}", Vars::serialNum.getVal());
        flog::debug("Motor controller boot revision: {}", Vars::bootRev.convertToReal());
        flog::debug("Motor controller program type: {}", Vars::programType.convertToReal());
        flog::debug("Motor controller hardware revision: {}", Vars::hardwareRev.getVal());
    }
    else {
        result = true;
        flog::debug("Running fake motor contoller");
    }
    return result;
}

bool readVars(Var** vars, int varCount)
{
    Var** varsToRead = new Var*[varCount];
    int* alreadyRead = new int[varCount];
    for(int i = 0; i < varCount; i++)
        alreadyRead[i] = -1;
    int toRead = 0;

    // Check if we're attempting a double read
    for(int i = 0; i < varCount; i++)
        for(int j = i+1; j < varCount; j++)
            if(vars[i]->getAddr() == vars[j]->getAddr())
                flog::warn("Variable '{}' read twice in readVars call", vars[i]->getName());
    
    // Iterate through each variable to read, if we're reading more than one variable
    for(int i = 0; i < varCount; i++) {

        // Don't attempt to read the same variable twice
        for(int j = 0; j < varCount; j++)
            if(alreadyRead[j] == i)
                continue;
        varsToRead[toRead] = vars[i];
        toRead++;

        uint lastByte = (vars[i]->getAddr())+(vars[i]->getNumBytes()*vars[i]->getArrayLen());

        // Iterate over every variable after the current one
        for(int j = i+1; j < varCount; j++) {
            
            // Don't attempt to read the same variable twice
            for(int k = 0; k < varCount; k++)
                if(alreadyRead[k] == j)
                    continue;

            // If the variable is within 20 bytes of the current one
            if(lastByte-vars[j]->getAddr() > 0 && lastByte-vars[j]->getAddr() < 20) {
                varsToRead[toRead] = vars[j];
                toRead++;
                lastByte = vars[j]->getAddr()+(vars[j]->getNumBytes()*vars[j]->getArrayLen());
            }
            else
                continue;
        }

        // Read the variable
        unsigned char* data = new unsigned char[lastByte-vars[i]->getAddr()];
        if(!readAddress(vars[i]->getAddr(), lastByte-vars[i]->getAddr(), &data)) {
            flog::error("Failed to read variable {} from motor controller!", vars[i]->getName());
            return false;
        }

        // Parse the variables
        for(int j = 0; j < toRead; j++) {
            Var* var = varsToRead[j];
            int start = var->getAddr()-varsToRead[j]->getAddr();

            // Copy the variable data into the variable while flipping the endianness
            long* varData = new long[var->getArrayLen()];
            for(int l = 0; l < var->getArrayLen(); l++) {
                varData[l] = 0;
                for(int k = 0; k < var->getNumBytes(); k++)
                    varData[l] += data[start+l*var->getNumBytes()+k] << (k*8);
            }
            var->setValue(varData, var->getArrayLen());
            delete[] varData;
        }
        toRead = 0;
        for(int j = 0; j < varCount; j++)
            alreadyRead[j] = -1;
        //delete[] data;
    }
    //delete[] varsToRead;
    delete[] alreadyRead;
    return true;
}

bool readVar(Var* var)
{
    Var* vars[] = { var };
    return readVars(vars, 1);
}

bool readAddress(uint32_t addr, uint numBytes, unsigned char** outData)
{
    // Read the bytes in groups of 56bytes
    int i = 0;
    while((i * 56) < numBytes) {
        // Calculate the length to ask the controller for
        uint len = numBytes - (i * 56); // len = bytes to read - bytes already read
        if(len > 56u)
            len = 56u;
        else if(len <= 0)
            break;
        
        // Send the read command to the controller
        bool result = sendData(1, addr + (56 * i), nullptr, len);
        if(!result)
            return result;

        // Read the data
        int bytesRead = hid_read(motorController, rxBuf, 64); // Was 65 before for some reason
        for(int j = 0; j < bytesRead-8; j++)
            (*outData)[j+(i*56)] = rxBuf[j+8];
        i++;
    }
    return true;
}

bool readSensors(sensor_data* sensors)
{
    if(!readVars(Vars::telemetryVars, 6))
        return false;

    // TODO: Fix this reading multiple vars
    readVar(&Vars::battVoltage);
    readVar(&Vars::throttleLocal);
    readVar(&Vars::throttlePos);
    readVar(&Vars::outputAmps);
    readVar(&Vars::battTemp);
    readVar(&Vars::mcuTemp);
    readVar(&Vars::keySwitch);
    readVar(&Vars::userSwitch);

    // Get the throttle
    sensors->throttle = Vars::throttlePos.convertToReal();
    if(sensors->throttle < 0)
        sensors->throttle = 0;
    else if(sensors->throttle > 4095)
        sensors->throttle = 4095;

    // Set the values in the output struct to the read data
    sensors->battVolt = Vars::battVoltage.convertToReal();
	sensors->battCur = Vars::outputAmps.convertToReal() * (double)Vars::throttleLocal.getVal() / 4095.0;
	sensors->throttle = 100.0 * sensors->throttle / 4095.0;
	sensors->controlTemp = Vars::mcuTemp.convertToReal();
	sensors->battTemp = Vars::battTemp.convertToReal();
    sensors->userSwitch = Vars::userSwitch.getValue();
    sensors->pwrSwitch = Vars::keySwitch.getValue();
    return true;
}

void generateFakeData(sensor_data* sensors)
{
    sensors->battVolt = fabs(sin(time(NULL)))*13;
    sensors->battCur = fabs(cos(time(NULL)))*25;
    sensors->throttle = fabs(cos(time(NULL)))*100;
    sensors->battTemp = 23.3+(cos(time(NULL))*2);
    sensors->controlTemp = 26.4+(sin(time(NULL))*4);
    sensors->userSwitch = rand() > (RAND_MAX/2);
    sensors->pwrSwitch = rand() > (RAND_MAX/2);
}

void startMonitor(int interval)
{
    monDelay = interval;
    monThreadRunning = true;
    monThread = std::thread(monitorWorker);
}

void stopMonitor()
{
    monThreadRunning = false;
    if(monThread.joinable())
        monThread.join();
}

void monitorWorker()
{
    sensor_data* sensors = (sensor_data*)malloc(sizeof(sensor_data));
    while(monThreadRunning) {
        if(!fakeData)
            readSensors(sensors);
        else
            generateFakeData(sensors);
        rxCallback(sensors);
        std::this_thread::sleep_for(std::chrono::seconds(monDelay));
    }
    delete sensors;
}

void cleanup()
{
    stopMonitor();
    hid_close(motorController);
}

void setMonitorCallback(mcu_mon_callback_t callback) { rxCallback = callback; }

}
