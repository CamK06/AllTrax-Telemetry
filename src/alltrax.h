#pragma once

#include <spdlog/spdlog.h>
#include <libusb-1.0/libusb.h>

namespace Alltrax
{

void initializeLibUSB();
bool findMotorController();
void openMotorController();
void libUSBLogCB(libusb_context *ctx, enum libusb_log_level level, const char* str);

}