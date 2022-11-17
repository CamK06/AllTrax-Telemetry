#include "alltrax.h"
#include "alltraxmem.h"
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

    return true;
}

void sendData(char reportID, uint addressFunction, char* data, char length)
{
    // Properly format the address function
    char address[4] = {
        0, 0, 0, (char)addressFunction
    };
    address[0] = (char)(addressFunction/256u/256/256);
    address[1] = (char)(addressFunction/256u/256u);
    address[2] = (char)(addressFunction/256u);

    // Build the packet to send
    char* packet = new char[64];
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
        packet[2] = (char)num;
        packet[3] = (char)num;
    }

    // Add data to the packet
    for(int i = 0; i < length; i++)
        packet[i+1] = data[i];

    // Send the packet
    hid_write(motorController, (const unsigned char*)packet, 64);
}

bool getInfo()
{
    // TODO: Implement
    return false;
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
}

void endRead()
{
    // Clean up the read thread
    readThreadRunning = false;
    if(readThread.joinable())
        readThread.join();
}

void cleanup()
{
    endRead();
}

void setReceiveCallback(mcu_receive_callback_t callback) { rxCallback = callback; }

/*
+---------------+
|OLD LIBUSB CODE|
+---------------+

bool libUsbInitialized = false;
libusb_device* motorControllerDev = nullptr;
libusb_device_handle* motorController = nullptr;

void initializeLibUSB()
{
    spdlog::info("Initializing libUSB...");
    libusb_init(NULL);
    libusb_set_log_cb(NULL, &libUSBLogCB, LIBUSB_LOG_CB_GLOBAL);
    libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
    libUsbInitialized = true;
}

void openMotorController()
{
    // If libUSB has not been initialized or we have not found a motor controller
    if(!libUsbInitialized || motorControllerDev == nullptr) {
        spdlog::error("Cannot open motor controller:");
        spdlog::error("libUSB not initialized or motor controller not found");
        return;
    }

    // Open the motor controller 
    spdlog::info("Opening motor controller...");
    int result = libusb_open(motorControllerDev, &motorController);
    if(result == 0)
        return;

    // If this is reached, the motor controller failed to open
    switch(result)
    {
        case LIBUSB_ERROR_NO_MEM:
            spdlog::error("Failed open motor controller! Out of memory.");
            break;
        case LIBUSB_ERROR_ACCESS:
            spdlog::error("Failed open motor controller! Access denied.");
            break;
        case LIBUSB_ERROR_NO_DEVICE:
            spdlog::error("Failed to open motor controller! Device not found.");
            break;
        default:
            spdlog::error("Failed to open motor controller! Unknown error.");
            break;
    }
    exit(-1);
}

bool findMotorController()
{
    // If libUSB has not been initialized, we cannot continue
    // (Perhaps it would make sense to initialize it here?)
    if(!libUsbInitialized) {
        spdlog::error("libUSB must be initialized before searching for a motor controller!");
        return false;
    }

    // Enumerate USB devices
    spdlog::info("Searching for motor controller...");
    libusb_device** devices;
    size_t deviceCount = libusb_get_device_list(NULL, &devices);
    spdlog::debug("{} USB devices found", deviceCount);

    // Iterate through devices and look for the motor controller
    for(size_t i = 0; i < deviceCount; i++) {
        libusb_device* device = devices[i];
        libusb_device_descriptor desc;
        libusb_get_device_descriptor(device, &desc);

        // Check if this device is the motor controller by comparing the USB Vendor ID and Product ID
        if(desc.idVendor == ALLTRAX_VID && desc.idProduct == ALLTRAX_PID) {
            // We've found the controller, open it then free the device list
            spdlog::info("Motor controller found!");
            spdlog::debug("Device: {}\nBus: {}", i, libusb_get_bus_number(device));
            motorControllerDev = device;
            openMotorController();
            libusb_free_device_list(devices, 1);
            return true;
        }
    }

    // Clean up
    libusb_free_device_list(devices, 1);
    return false;
}

void libUSBLogCB(libusb_context *ctx, enum libusb_log_level level, const char* str)
{
    switch(level) {
        case LIBUSB_LOG_LEVEL_DEBUG:
            spdlog::debug(str);
            break;
        case LIBUSB_LOG_LEVEL_ERROR:
            spdlog::error(str);
            break;
        case LIBUSB_LOG_LEVEL_INFO:
            spdlog::info(str);
            break;
        case LIBUSB_LOG_LEVEL_WARNING:
            spdlog::warn(str);
            break;
        default:
            spdlog::debug(str);
            break;
    }
}
*/

}