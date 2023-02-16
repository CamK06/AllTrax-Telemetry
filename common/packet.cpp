#include "packet.h"
#include <string.h>
#include <stdint.h>

#define SEPARATOR_A 0xea
#define SEPARATOR_B 0xff

/*
Telemetry packet format:

Packets will always be 64bytes long
| = 0xeaff, separator sequence, 2bytes
TEMP = Average between battery and motor temperature

---------------------------------------------------------------------------------------
|8byte|  4bytes   |  4bytes  |     2byte     |4bytes|  16bytes  |  16bytes    2bytes
---------------------------------------------------------------------------------------
|[TIME]|[BATTVOLTS]|[BATTAMPS]|[THROTTLEPCNT]|[TEMP]|[LAT]|[LONG]|[VELOCITY][USR][KEY]|
---------------------------------------------------------------------------------------

*/

namespace Telemetry
{

void decodePacket(unsigned char* data, sensor_data* sensors, gps_pos* gps, time_t* recTimeOut)
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
    memcpy(&gps->latitude, data+32, sizeof(double));
    memcpy(&gps->longitude, data+42, sizeof(double));
    memcpy(&gps->velocity, data+52, sizeof(double));
    sensors->userSwitch = data[60];
    sensors->pwrSwitch = data[61];
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

    // Position
    memcpy((*outData)+32, &gps->latitude, sizeof(double));
    (*outData)[40] = SEPARATOR_A;
    (*outData)[41] = SEPARATOR_B;
    memcpy((*outData)+42, &gps->longitude, sizeof(double));
    (*outData)[50] = SEPARATOR_A;
    (*outData)[51] = SEPARATOR_B;

    // Velocity
    memcpy((*outData)+52, &gps->velocity, sizeof(double));

    // Switches
    (*outData)[60] = sensors->userSwitch;
    (*outData)[61] = sensors->pwrSwitch;

    // Footer
    (*outData)[62] = SEPARATOR_A;
    (*outData)[63] = SEPARATOR_B;
}

}
