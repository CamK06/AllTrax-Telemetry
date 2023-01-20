#include "packet.h"
#include <string.h>
#include <stdint.h>

#define SEPARATOR_A 0xea
#define SEPARATOR_B 0xff

/*
Telemetry packet format:

Packets will always be 32bytes long
| = 0xeaff, separator sequence, 2bytes
TEMP = Average between battery and motor temperature

-----------------------------------------------------
|8byte|  4bytes   |  4bytes  |     2byte     |4bytes
-----------------------------------------------------
|[TIME]|[BATTVOLTS]|[BATTAMPS]|[THROTTLEPCNT]|[TEMP]
-----------------------------------------------------

*/

namespace Telemetry
{

void decodePacket(unsigned char* data, sensor_data* sensors, time_t* recTimeOut)
{
    // Decode the packet by simply memcpying into each sensor variable
    // with the offset and size of each variable in the packet
    if(recTimeOut != nullptr)
        memcpy(recTimeOut, data+2, sizeof(time_t));
    memcpy(&sensors->battVolt, data+12, sizeof(float));
    memcpy(&sensors->battCur, data+18, sizeof(float));
    memcpy(&sensors->throttle, data+24, sizeof(int16_t));
    memcpy(&sensors->battTemp, data+28, sizeof(float));
    memcpy(&sensors->controlTemp, data+28, sizeof(float));
}

void formatPacket(sensor_data* sensors, gps_pos* gps, unsigned char** outData)
{
    // Header
    (*outData)[0] = SEPARATOR_A;
    (*outData)[1] = SEPARATOR_B;
    
    // Current time
    time_t curTime = time(NULL);
    memcpy((*outData)+2, &curTime, sizeof(time_t));
    (*outData)[10] = SEPARATOR_A;
    (*outData)[11] = SEPARATOR_B;

    // Battery voltage
    memcpy((*outData)+12, &sensors->battVolt, sizeof(float));
    (*outData)[16] = SEPARATOR_A;
    (*outData)[17] = SEPARATOR_B;

    // Battery current
    memcpy((*outData)+18, &sensors->battCur, sizeof(float));
    (*outData)[22] = SEPARATOR_A;
    (*outData)[23] = SEPARATOR_B;

    // Throttle
    memcpy((*outData)+24, &sensors->throttle, sizeof(int16_t));
    (*outData)[26] = SEPARATOR_A;
    (*outData)[27] = SEPARATOR_B;

    // Temperature
    float avgTemp = (sensors->battTemp+sensors->controlTemp)/2;
    memcpy((*outData)+28, &avgTemp, sizeof(float));
}

}