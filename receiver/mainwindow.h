#pragma once

#include "version.h"
#include "packet.h"
#include "gps.h"
#include "./ui_mainwindow.h"

#include <QMainWindow>
#include <QGuiApplication>
#include <QChartView>
#include <QLineSeries>
#include <QChart>
#include <QChartGlobal> 
#include <thread>
#include <vector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
public slots:
    void packetCallback(sensor_data sensors, gps_pos gps);
private:
    void connectTelem();
    void disconnectTelem();
    void updateData();
    void exportJson();
    void exit(int code);

    Ui::MainWindow *ui;

    // Data
    time_t lastRx;
    std::vector<time_t> times;
    std::vector<sensor_data> sensors;
    std::vector<gps_pos> positions;

    // Graphing
    int k = 0;
    QLineSeries *voltsSeries;
    QLineSeries *currentSeries;
    QChart *voltsChart;
    QChart *currentChart;

    // Battery charge table
    const float chargeTable[11][2] = {
        {12.7, 100},
        {12.5, 90},
        {12.42, 80},
        {12.32, 70},
        {12.20, 60},
        {12.06, 50},
        {11.90, 40},
        {11.75, 30},
        {11.58, 20},
        {11.31, 10},
        {10.50, 0}
    };
};