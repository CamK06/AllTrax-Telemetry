#pragma once

#include "packet.h"
#include "gps.h"
#define USE_IP

typedef void (*radio_rx_callback_t)(sensor_data sensors, gps_pos gps);

namespace Radio
{

void sendSensors(sensor_data* sensors, gps_pos* gps);
void sendData(unsigned char* data, int len);
void receiveData(int sig);
void close();
void init(const char* port, bool client = false);
void setRxCallback(radio_rx_callback_t callback);

}
