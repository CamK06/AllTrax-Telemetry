#include "alltrax.h"
#include "alltraxvars.h"
#include <thread>

namespace Alltrax
{
// General vars
hid_device* motorController = nullptr;
bool useChecksum = true; // This is always true in our controller
bool fakeData = false;

// Monitor mode vars
mcu_mon_callback_t rxCallback = nullptr;
bool monThreadRunning = false;
std::thread monThread;
int monDelay = 0;

bool initMotorController(bool useFakeData)
{
    // Warn the user if the receive callback has not been set
    if(rxCallback == nullptr)
        spdlog::warn("Receive callback is not set! No data will be received from the motor controller!");

    // Attempt to open the motor controller from its VID and PID
    if(!useFakeData) {
        spdlog::info("Searching for motor controller...");
        hid_init();
        motorController = hid_open(ALLTRAX_VID, ALLTRAX_PID, NULL);
        if(!motorController) {
            spdlog::error("Controller not found!");
            spdlog::error("Exiting...");
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

    spdlog::debug("Packet sent:");
    for(int i = 0; i < 64; i++) {
		printf("%02X ", packet[i]);
		if(i == 15 || i == 31 || i == 47)
			printf("\n");
	}
    printf("\n");

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
        result = readVars(Vars::infoVars, 6);
        spdlog::debug("Motor controller model: {}", Vars::model.getValue());
        spdlog::debug("Motor controller build date: {}", Vars::buildDate.getValue());
        spdlog::debug("Motor controller serial number: {}", Vars::serialNum.getVal());
        spdlog::debug("Motor controller boot revision: {}", Vars::bootRev.convertToReal());
        spdlog::debug("Motor controller program type: {}", Vars::programType.convertToReal());
        spdlog::debug("Motor controller hardware revision: {}", Vars::hardwareRev.getVal());
    }
    else {
        result = true;
        spdlog::debug("Running fake motor contoller");
    }
    return result;
}

// This entire function is overcommented to hell and back because honestly even I barely understand *why* Alltrax's original software does half of the stuff
// that it does here, the comments are just my best guesses as why they do what they do, and I'll never remember those guesses without the comments
bool readVars(Var** vars, int varCount)
{
    // Check if we're attempting to read the same variable multiple times
    // This code kinda sucks but I don't care, it'll be someone elses problem, not mine
    for(int i = 0; i < varCount; i++) 
        for(int j = i+1; j < varCount; j++)
            if(vars[i]->getAddr() == vars[j]->getAddr())
                spdlog::warn("Variable '{}' read twice in readVars call", vars[i]->getName());

    // Read each variable in vars
    for(int i = 0; i < varCount; i++) {
        
        // Determine the address of the last byte to be read
        // For single variables this is just the address plus the length in bytes
        uint lastByte = vars[i]->getAddr() + (vars[i]->getNumBytes() * vars[i]->getArrayLen());
        int lastVar = 0;

        // For multiple variables, the length parameter is from the current variable to the end of the last, also in bytes
        // This is calculated in this for loop
        for(int j = i+1; j < varCount; j++) {
            // Original Alltrax program skips over null array elements using a while loop here
            // We don't do that (array should never be null), but I'm leaving this note in case it needs to be added in the future

            // Break if the distance from the end of the current var to the start of the next is >= 20bytes
            // For some reason this causes the data length to be all weird, it works fine if we use continue instead of break but
            // that is probably quite incorrect...
            if((vars[j]->getAddr() - lastByte) >= 20)
                //break;
                continue;

            // Set lastByte to be the last byte of the current variable (this in most cases will be the last variable in the array)
            lastByte = vars[j]->getAddr() + (vars[j]->getNumBytes() * vars[j]->getArrayLen());
            lastVar = j;
        }

        // Read the variable from the controller
        // We calculate data length on the spot here; (lastByte-vars[i].getAddr())
        unsigned char* dataIn = new unsigned char[lastByte-vars[i]->getAddr()];
        bool result = readAddress(vars[i]->getAddr(), lastByte-vars[i]->getAddr(), &dataIn);
        if(!result) { // Read failed
            spdlog::error("Failed to read variable '{}' from motor controller!", vars[i]->getName());
            return result;
        }

        // Parse the read data into appropriate long arrays for Var
        for(int j = i; j <= lastVar; j++) {
            
            // Copy the variables data from dataIn into varData
            unsigned char* varData = new unsigned char[vars[j]->getArrayLen() * vars[j]->getNumBytes()];
            memcpy(varData, dataIn+(vars[j]->getAddr()-vars[i]->getAddr()), vars[j]->getArrayLen() * vars[j]->getNumBytes());
            long* varValue = new long[vars[j]->getArrayLen()];
            
            // Parse all of the data into appropriate formatting for the long array used by Vars
            switch(vars[j]->getType()) {
                case VarType::BOOL:
                    for(int k = 0; k < vars[j]->getArrayLen(); k++)
                        if(varData[k] == 0)
                            varValue[k] = 0;
                        else
                            varValue[k] = 1;
                    break;

                // Single byte values like 8 bit numbers and strings are just 1:1 mapped
                case VarType::BYTE:
                case VarType::SBYTE:
                case VarType::STRING:
                    for(int k = 0; k < vars[j]->getArrayLen(); k++)
                        varValue[k] = varData[k];
                    break;

                case VarType::INT16:
                case VarType::UINT16:
                    for(int k = 0; k < vars[j]->getArrayLen(); k++)
                        varValue[k] = (varData[k*2+1] << 8) + varData[k*2];
                    break;

                // Uint32 and Int32 looked the same in the original program, this *may* however not be the case
                case VarType::UINT32:
                case VarType::INT32:
                    for(int k = 0; k < vars[j]->getArrayLen(); k++)
                        varValue[k] = (long)((varData[k*4+3] << 24) + (varData[k*4+2] << 16) + (varData[k*4+1] << 8) + varData[k*4]);
                    break;
            }
            vars[j]->setValue(varValue, vars[j]->getArrayLen());
            delete varValue;
            delete varData;
        }
        i = lastVar;
    }
    return true;
}

bool readVar(Var* var)
{
    Var* vars[] = { var };
    return readVars(vars, 1);
}

bool readAddress(uint32_t addr, uint numBytes, unsigned char** outData)
{
    spdlog::debug("Reading address 0x{0:x} from controller...", addr);
    spdlog::debug("Bytes to read {}", numBytes);

    // Read the bytes in groups of 56bytes
    int i = 0;
    while((i * 56) < numBytes) {
        // Calculate the length to ask the controller for
        uint len  = numBytes - (i * 56); // len = bytes to read - bytes already read
        if(len > 56u)
            len = 56u;
        else if(len <= 0)
            break;
        
        // Send the read command to the controller
        bool result = sendData(1, addr + (56 * i), nullptr, len);
        if(!result)
            return result;

        // Read the data
        unsigned char buf[65];
        int bytesRead = hid_read(motorController, buf, 65);
        for(int j = 0; j < bytesRead-8; j++)
            (*outData)[j+(i*56)] = buf[j+8];
        i++;
    }

    spdlog::debug("Successfully read {} bytes from 0x{:x}", numBytes, addr);
    return true;
}

bool readSensors(sensor_data* sensors)
{
    if(!readVars(Vars::telemetryVars, 6))
        return false;

    // Set the values in the output struct to the read data
    sensors->battVolt = Vars::battVoltage.convertToReal();
	sensors->battCur = Vars::outputAmps.convertToReal() * (double)Vars::throttleLocal.getVal() / 4095.0;
	sensors->throttle = 100.0 * Vars::throttlePos.convertToReal() / 4095.0;
	sensors->controlTemp = Vars::mcuTemp.convertToReal();
	sensors->battTemp = Vars::battTemp.convertToReal();
    return true;
}

void generateFakeData(sensor_data* sensors)
{
    sensors->battVolt = fabs(sin(time(NULL)))*13;
    sensors->battCur = fabs(cos(time(NULL)))*25;
    sensors->throttle = fabs(cos(time(NULL)))*100;
    sensors->battTemp = 23.3+(cos(time(NULL))*2);
    sensors->controlTemp = 26.4+(sin(time(NULL))*4);
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
    free(sensors);
}

void cleanup()
{
    monThreadRunning = false;
    if(monThread.joinable())
        monThread.join();
    hid_close(motorController);
}

void setMonitorCallback(mcu_mon_callback_t callback) { rxCallback = callback; }

}
