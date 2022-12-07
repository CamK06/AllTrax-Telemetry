#pragma once

#include "packet.h"
#include "mainwindow.h"
#include <QMetaObject>

Q_DECLARE_METATYPE(sensor_data);

namespace Telemetry
{
void handle_sigio(int sig);
void initRadio(MainWindow* mainWindow);

}