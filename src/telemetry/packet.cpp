#include "packet.h"

#define SEPARATOR_A 0xea
#define SEPARATOR_B 0xff

/*
Telemetry packet format:

Packets will always be 32bytes long
| = 0xeaff, separator sequence, 2bytes
TEMP = Average between battery and motor temperature

-----------------------------------------------------
|8byte|  2bytes   |  4bytes  |     2byte     |4bytes|
-----------------------------------------------------
|[TIME]|[BATTVOLTS]|[BATTAMPS]|[THROTTLEPCNT]|[TEMP]|
-----------------------------------------------------

*/

namespace Telemetry
{

void formatPacket(sensor_data* sensors, unsigned char** outData)
{
    // Header
    (*outData)[0] = SEPARATOR_A;
    (*outData)[1] = SEPARATOR_B;
    
    // Current time
    // All of this division splits the 8 byte long time into individual bytes
    (*outData)[2] = time(NULL)/256/256/256/256/256/256/256;
    (*outData)[3] = time(NULL)/256/256/256/256/256/256;
    (*outData)[4] = time(NULL)/256/256/256/256/256;
    (*outData)[5] = time(NULL)/256/256/256/256;
    (*outData)[6] = time(NULL)/256/256/256;
    (*outData)[7] = time(NULL)/256/256;
    (*outData)[9] = time(NULL)/256;
    (*outData)[8] = time(NULL);
    (*outData)[10] = SEPARATOR_A;
    (*outData)[11] = SEPARATOR_B;

    // Battery voltage
    (*outData)[12] = sensors->battVolt/256;
    (*outData)[13] = sensors->battVolt;
    (*outData)[14] = SEPARATOR_A;
    (*outData)[15] = SEPARATOR_B;

    // Battery current
    // This will likely need to be adjusted as it's a float*
    (*outData)[16] = sensors->battCur/256/256/256;
    (*outData)[17] = sensors->battCur/256/256;
    (*outData)[18] = sensors->battCur/256;
    (*outData)[19] = sensors->battCur;
    (*outData)[20] = SEPARATOR_A;
    (*outData)[21] = SEPARATOR_B;

    // Throttle
    (*outData)[22] = sensors->throttle/256;
    (*outData)[23] = sensors->throttle;
    (*outData)[24] = SEPARATOR_A;
    (*outData)[25] = SEPARATOR_B;

    // Temperature
    (*outData)[26] = ((sensors->battTemp+sensors->controlTemp)/2)/256/256/256;
    (*outData)[27] = ((sensors->battTemp+sensors->controlTemp)/2)/256/256;
    (*outData)[28] = ((sensors->battTemp+sensors->controlTemp)/2)/256;
    (*outData)[29] = (sensors->battTemp+sensors->controlTemp)/2;
    (*outData)[30] = SEPARATOR_A;
    (*outData)[31] = SEPARATOR_B;
}

}