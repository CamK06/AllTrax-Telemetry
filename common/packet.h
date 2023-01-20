#pragma once

#include "gps.h"
#include <ctime>
#include <cstddef>
#include <stdint.h>

struct sensor_data
{
	int16_t throttle; 
	float battVolt;
	float battCur;
	float battTemp;
	float controlTemp;
};
typedef void (*mcu_mon_callback_t)(sensor_data* sensors);

namespace Telemetry
{

void formatPacket(sensor_data* sensors, gps_pos* gps, unsigned char** outData);
void decodePacket(unsigned char* data, sensor_data* sensors, time_t* recTimeOut = nullptr);

}