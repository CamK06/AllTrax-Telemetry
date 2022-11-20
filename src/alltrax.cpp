#include "alltrax.h"
#include "alltraxmem.h"
#include "alltraxvars.h"
#include <thread>

#define ALLTRAX_VID 0x23D4
// This PID might also be 2, the original code uses PID to determine whether or not to use checksums
// If the PID is 1, then checksums are used
#define ALLTRAX_PID 0x0001 

#include <spdlog/spdlog.h>

namespace Alltrax
{

hid_device* motorController = nullptr;
mcu_receive_callback_t rxCallback = nullptr;
char* receivedData = nullptr;
bool readThreadRunning = false;
std::thread readThread;
bool useChecksum = true;

bool initMotorController()
{
    // Warn the user if the receive callback has not been set
    if(rxCallback == nullptr)
        spdlog::warn("Receive callback is not set! No data will be received from the motor controller!");

    spdlog::info("Searching for motor controller...");
    hid_init();

    // Attempt to open the motor controller based on its VID and PID
    motorController = hid_open(ALLTRAX_VID, ALLTRAX_PID, NULL);
    if(!motorController) {
        hid_exit();
        return false;
    }

    // Reset the controller
    spdlog::info("Motor controller opened successfully");
    unsigned char data[1] = { 0x3 };
    int result = hid_write(motorController, data, 1);
    if(result == -1) {
        spdlog::error("Failed to reset motor controller");
        return false;
    }

    VarBool testVal("What h", 0xffffff);
    testVal.getValue();

    return true;
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
    return false;

    // Send the packet
    int result = hid_write(motorController, packet, 64);
    if(result == -1)
        return false;
    return true;
}

bool getInfo()
{
    // Read the motor controller's model
    // Numbers here are from last var read in GetInfo; HardwareRev
    // So, TODO: Implement the Var array bullshit the original code does
    // And implement ReadVars
    // OR keep this as is and read one by one?
    // 4 is NumBytes, 1 is ArrayLength
    uint len = (uint)((ulong)134218800u + (ulong)((long)(4 * 1)));


    char* modelName;
    readAddress(AlltraxVars::modelName.addr, len - AlltraxVars::modelName.addr, &modelName);
    spdlog::debug("Read model name: {}", modelName);

    return false;
}

bool readAddress(uint32_t addr, uint numBytes, char** outData)
{
    spdlog::debug("Reading address 0x{0:x} from controller...", addr);
    *outData = new char[numBytes];

    // Send the data
    // Not sure why alltrax does this math stuff, but they do so I do it too
    int num = 0;
    while((long)(num * 56) < (long)((ulong)numBytes)) {
        uint num2  = (uint)((ulong)numBytes - (ulong)((long)(num * 56)));
        if(num2 > 56u)
            num2 = 56u;
        
        bool result = sendData(1, (uint)((ulong)addr + (ulong)((long)(56 * num))), nullptr, num2);
        if(!result)
            return result;
        
        // Read the data
        for(int i = 0; i < num2; i++)
            *outData[(num*56)+i] = receivedData[i];
        num++;
    }

    spdlog::debug("Successfully read address from controller");
    return true;
}

void readWorker()
{
    unsigned char* dataIn = new unsigned char[64];
    while(readThreadRunning) {
        hid_read(motorController, dataIn, 64); // This may need to be 65 bytes or some other number
        rxCallback(dataIn, 64);
    }
}

void beginRead()
{
    // Begin the read thread
    readThread = std::thread(&readWorker);
    readThreadRunning = true;
    receivedData = new char[56];
    for(int i = 0; i < 56; i++)
        receivedData = 0x00;
}

void endRead()
{
    // Clean up the read thread
    readThreadRunning = false;
    if(readThread.joinable())
        readThread.join();
    delete receivedData;
}

void cleanup()
{
    endRead();
}

void setReceiveCallback(mcu_receive_callback_t callback) { rxCallback = callback; }

}