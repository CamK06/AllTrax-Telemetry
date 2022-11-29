#pragma once

#include <spdlog/spdlog.h>
#include <hidapi/hidapi.h>
#include "alltraxvars.h"

#define ALLTRAX_VID 0x23D4
#define ALLTRAX_PID 0x0001 // This is 1 on our controller 

struct sensor_data
{
	int throttle; 
	double battVolt;
	double battCur;
	double battTemp;
	double controlTemp;
};
typedef void (*mcu_mon_callback_t)(sensor_data* sensors);

namespace Alltrax
{
extern bool monThreadRunning;

// Wrapper functions
bool initMotorController(bool useFakeData = false);
bool getInfo();
void cleanup();

// Monitor mode functions
void startMonitor(int interval = 3);
void stopMonitor();
bool readSensors(sensor_data* sensors);
void setMonitorCallback(mcu_mon_callback_t callback);
void monitorWorker();
void generateFakeData(sensor_data* sensors);

// Controller I/O functions
bool sendData(char reportID, char* addressFunction, char** outData, char length);
bool readAddress(uint32_t addr, uint numBytes, unsigned char** outData);
bool readVars(Var** vars, int varCount);
bool readVar(Var* var);
}
