#pragma once

#include <spdlog/spdlog.h>
#include <hidapi/hidapi.h>
#include "alltraxvars.h"

typedef void (*mcu_receive_callback_t)(unsigned char* data, size_t len);

namespace Alltrax
{

void setReceiveCallback(mcu_receive_callback_t callback);
bool initMotorController();
void beginRead();
void endRead();
void readWorker();
bool readAddress(uint32_t addr, uint numBytes, char** outData);
bool getInfo();
bool sendData(char reportID, char* addressFunction, char** outData, char length);
void cleanup();

bool readVars(Var* vars, int varCount);
bool readVar(Var var);
}