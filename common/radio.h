#pragma once

#include "packet.h"
#include "gps.h"

#ifdef GUI_RX
#include "../receiver/mainwindow.h"
#include <QMetaObject>
Q_DECLARE_METATYPE(sensor_data);
Q_DECLARE_METATYPE(gps_pos);
#else
typedef void (*radio_rx_callback_t)(unsigned char* data);
#endif

namespace Radio
{

void sendSensors(sensor_data* sensors, gps_pos* gps);
void sendData(unsigned char* data, int len);
void receiveData(int sig);
void close();
#ifdef GUI_RX
void init(MainWindow* mainWindow, const char* port);
#else
void init(const char* port);
#endif

}
