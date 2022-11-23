#include "alltrax.h"
#include "alltraxvars.h"
#include <thread>

#define ALLTRAX_VID 0x23D4
#define ALLTRAX_PID 0x0001 // This is 1 on our controller 

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

    // Attempt to open the motor controller from its VID and PID
    motorController = hid_open(ALLTRAX_VID, ALLTRAX_PID, NULL);
    if(!motorController) {
        spdlog::error("Controller not found!");
        spdlog::error("Exiting...");
        hid_exit();
        return false;
    }

    getInfo();
    spdlog::debug("Controller model: {}", Vars::model.getValue());

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
    readVars(Vars::infoVars, 9);

    return false;
}

// This entire function is overcommented to hell and back because honestly even I barely understand *why* Alltrax's original software does half of the stuff
// that it does here, the comments are just my best guesses as why they do what they do, and I'll never remember those guesses without the comments
bool readVars(Var* vars, int varCount)
{
    // Check if we're attempting to read the same variable multiple times
    // This code kinda sucks but I don't care, it'll be someone elses problem, not mine
    for(int i = 0; i < varCount; i++) 
        for(int j = i+1; j < varCount; j++)
            if(vars[i].getAddr() == vars[j].getAddr())
                spdlog::warn("Variable '{}' read twice in readVars call", vars[i].getName());

    // Read each variable in vars
    for(int i = 0; i < varCount; i++) {
        
        // Determine the address of the last byte to be read
        // For single variables this is just the address plus the length in bytes
        uint lastByte = vars[i].getAddr() + (vars[i].getNumBytes() * vars[i].getArrayLen());
        int lastVar = 0;

        // For multiple variables, the length parameter is from the current variable to the end of the last, also in bytes
        // This is calculated in this for loop
        for(int j = i+1; j < varCount; j++) {
            // Original Alltrax program skips over null array elements using a while loop here
            // We don't do that (array should never be null), but I'm leaving this note in case it needs to be added in the future

            // Break if the distance from the end of the current var to the start of the next is >= 20bytes
            // For some reason this causes the data length to be all weird, it works fine if we use continue instead of break but
            // that is probably quite incorrect...
            //if((vars[j].getAddr() - lastByte) >= 20U)
            //    break;
            //    continue;

            // Set lastByte to be the last byte of the current variable (this in most cases will be the last variable in the array)
            lastByte = vars[j].getAddr() + (vars[j].getNumBytes() * vars[j].getArrayLen());
            lastVar = j;
        }

        // Read the variable from the controller
        // We calculate data length on the spot here; (lastByte-vars[i].getAddr())
        char* dataIn;
        bool result = readAddress(vars[i].getAddr(), lastByte-vars[i].getAddr(), &dataIn);
        if(!result) { // Read failed
            spdlog::error("Failed to read variable '{}' from motor controller!", vars[i].getName());
            return result;
        }

        // Parse the read data into appropriate long arrays for Var
        for(int j = i; j <= lastVar; j++) {

            // Copy read data for this variable into a new array (this makes no sense, the indexing seems all wrong)
            // NOTE: I'm 90% certain the purpose of this is to fill varData with just the bytes for the current variable determined by the for loop
            // or in other words, varData should be the bytes from vars[j].address to vars[j].addres+vars[j].arraylength*vars[j].bytes
            char* varData = new char[vars[j].getArrayLen() * vars[j].getNumBytes()];
            for(int k = 0; k < vars[j].getArrayLen() * vars[j].getNumBytes(); k++)
                varData[k] = dataIn[(vars[j].getAddr()-vars[i].getAddr())+k]; // this will PROBABLY segfault... but it's what AllTrax does so ????????
            long* varValue = new long[vars[j].getArrayLen()];

            // Parse all of the data into appropriate formatting for the long array used by Vars
            switch(vars[j].getType()) {
                case VarType::BOOL:
                    for(int k = 0; k < vars[j].getArrayLen(); k++) {
                        if(varData[k] == 0)
                            varValue[k] = 0;
                        else
                            varValue[k] = 1;
                    }
                    break;

                // Single byte values like 8 bit numbers and strings are just 1:1 mapped
                case VarType::BYTE:
                case VarType::SBYTE:
                case VarType::STRING:
                    for(int k = 0; k < vars[j].getArrayLen(); k++)
                        varValue[k] = varData[k];
                    break;

                case VarType::UINT16:
                    for(int k = 0; k < vars[j].getArrayLen(); k++)
                        varValue[k] = (varData[k*2+1] << 8) + varData[k*2];
                    break;

                // Uint32 and Int32 looked the same in the original program, this *may* however not be the case
                case VarType::UINT32:
                case VarType::INT32:
                    for(int k = 0; k < vars[j].getArrayLen(); k++)
                        varValue[k] = (long)((varData[k*4+3] << 24) + (varData[k*4+2] << 16) + (varData[k*4+1] << 8) + varData[k*4]);
                    break;
            }
            vars[j].setValue(varValue, vars[j].getArrayLen());
        }
        i = lastVar;
    }
    return true;
}

bool readVar(Var var)
{
    Var vars[] = { var };
    return readVars(vars, 1);
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

    spdlog::debug("Successfully read {} bytes from {0x{0:x}", numBytes, addr);
    return true;
}

void readWorker()
{
    unsigned char* dataIn = new unsigned char[64];
    while(readThreadRunning) {
        hid_read(motorController, dataIn, 64); // This may need to be 65 bytes or some other number
        memcpy(receivedData+8, dataIn, 56); // Copy the data portion of the packet into receivedData
        rxCallback(dataIn, 64);
    }
}

void beginRead()
{
    // Begin the read thread
    receivedData = new char[56];
    for(int i = 0; i < 56; i++)
        receivedData = 0x00;
    readThread = std::thread(&readWorker);
    readThreadRunning = true;
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
