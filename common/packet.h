#pragma once

#include <ctime>
#include <cstddef>

struct sensor_data
{
	int throttle; 
	double battVolt;
	double battCur;
	double battTemp;
	double controlTemp;
};
typedef void (*mcu_mon_callback_t)(sensor_data* sensors);


namespace Telemetry
{

void formatPacket(sensor_data* sensors, unsigned char** outData);

}