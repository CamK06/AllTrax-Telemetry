#pragma once

#include "../alltrax/alltrax.h"

namespace Telemetry
{

void formatPacket(sensor_data* sensors, unsigned char** outData);

}