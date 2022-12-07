#include "mainwindow.h"
#include "radio.h"
#include <QTimer>
#include <spdlog/spdlog.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qRegisterMetaType<sensor_data>();
    lastRx = -1;

    // Signals
    connect(ui->fileExit, &QAction::triggered, this, &MainWindow::exit);
    connect(ui->fileConnect, &QAction::triggered, this, &MainWindow::connectTelem);

    // Set up graphs
    voltsSeries = new QLineSeries();
    currentSeries = new QLineSeries();
    voltsChart = new QChart();
    currentChart = new QChart();
    voltsChart->legend()->hide();
    voltsChart->setBackgroundVisible(false);
    ui->voltsGraph->setRenderHint(QPainter::Antialiasing);
    ui->voltsGraph->setChart(voltsChart);
    currentChart->legend()->hide();
    currentChart->setBackgroundVisible(false);
    ui->currentGraph->setRenderHint(QPainter::Antialiasing);
    ui->currentGraph->setChart(currentChart);

    updateData();
}

void MainWindow::updateData()
{
    if(sensors.size() <= 0)
        return;

    // Update charts
    voltsChart->removeAllSeries();
    currentChart->removeAllSeries();
    voltsSeries = new QLineSeries();
    currentSeries = new QLineSeries();
    for(int i = 0; i < sensors.size(); i++) {
        voltsSeries->append(i, sensors[i].battVolt);
        currentSeries->append(i, sensors[i].battCur);
    }
    voltsChart->addSeries(voltsSeries);
    currentChart->addSeries(currentSeries);
    voltsChart->createDefaultAxes();
    currentChart->createDefaultAxes();

    // Update chart text because Qt is weird
    voltsChart->axisX()->setLabelsVisible(false);
    voltsChart->axisY()->setLabelsColor(QApplication::palette().text().color());
    voltsChart->axisY()->setTitleText("Battery Voltage (V)");
    voltsChart->axisY()->setTitleBrush(QBrush(QApplication::palette().text().color()));
    currentChart->axisX()->setLabelsVisible(false);
    currentChart->axisY()->setLabelsColor(QApplication::palette().text().color());
    currentChart->axisY()->setTitleBrush(QBrush(QApplication::palette().text().color()));
    currentChart->axisY()->setTitleText("Battery Current (A)");

    // Update labels

    // General labels
    if(lastRx != -1) {
        char* timeStr = new char[16];
        tm* time = localtime(&lastRx);
        strftime(timeStr, 16, "%X", time);
        ui->lastRxLabel->setText(QString("Last Received: %1").arg(timeStr));
    }
    else
        ui->lastRxLabel->setText("Last Received: N/A");
    ui->throttleLabel->setText(QString("Throttle: %1%").arg(sensors[sensors.size()-1].throttle));
    ui->tempLabel->setText(QString("Temperature (C): %1°").arg(sensors[sensors.size()-1].battTemp));

    // Battery labels
    ui->voltsLabel->setText(QString("Voltage: %1V").arg(sensors[sensors.size()-1].battVolt));
    ui->currentLabel->setText(QString("Current: %1A").arg(sensors[sensors.size()-1].battCur));
}

void MainWindow::packetCallback(sensor_data sensors)
{
    this->sensors.push_back(sensors);
    lastRx = time(NULL);
    updateData();
}

void MainWindow::connectTelem()
{
    Telemetry::initRadio(this);
}

void MainWindow::exit(int code) { std::exit(code); }