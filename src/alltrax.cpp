#include "alltrax.h"

#define ALLTRAX_VID 0x23D4
#define ALLTRAX_PID 0x0001 // Might also be 2 

#include <spdlog/spdlog.h>

namespace Alltrax
{

bool libUsbInitialized = false;
libusb_device* motorController = nullptr;

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
    if(!libUsbInitialized || motorController == nullptr) {
        spdlog::error("Cannot open motor controller:");
        spdlog::error("libUSB not initialized or motor controller not found");
        return;
    }

    spdlog::info("Opening motor controller...");
    // TODO: Implement.
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
            motorController = device;
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

}