#pragma once

#include "packet.h"

#ifdef GUI_RX
#include "../receiver/mainwindow.h"
#include <QMetaObject>
Q_DECLARE_METATYPE(sensor_data);
#else
typedef void (*radio_rx_callback_t)(unsigned char* data);
#endif

namespace Radio
{

void sendSensors(sensor_data* sensors);
void sendData(unsigned char* data, int len);
void receiveData(int sig);
#ifdef GUI_RX
void init(MainWindow* mainWindow);
#else
void init();
#endif

}