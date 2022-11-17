#pragma once

#include <spdlog/spdlog.h>
#include <hidapi/hidapi.h>

typedef void (*mcu_receive_callback_t)(unsigned char* data, size_t len);

namespace Alltrax
{

void setReceiveCallback(mcu_receive_callback_t callback);
bool initMotorController();
void beginRead();
void endRead();
void readWorker();
bool getInfo();
void sendData(char reportID, char* addressFunction, char* data, char length);
void cleanup();

// Old libUSB code
//void initializeLibUSB();
//void libUSBLogCB(libusb_context *ctx, enum libusb_log_level level, const char* str);

}