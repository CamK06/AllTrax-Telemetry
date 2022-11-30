#pragma once

#include "../alltrax/alltrax.h"

namespace Telemetry
{

void txPacket(unsigned char* data, int len);
void txSensors(sensor_data* sensors);
void initIP();

}