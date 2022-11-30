#include "version.h"
#include "mainwindow.h"

#include <spdlog/spdlog.h>
#include <QApplication>

int main(int argc, char* argv[])
{
    spdlog::info("Alltrax Telemetry GUI " VERSION);
    QApplication app(argc, argv);
    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}