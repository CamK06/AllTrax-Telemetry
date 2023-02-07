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

    // Graphing
    int k = 0;
    QLineSeries *voltsSeries;
    QLineSeries *currentSeries;
    QChart *voltsChart;
    QChart *currentChart;
};