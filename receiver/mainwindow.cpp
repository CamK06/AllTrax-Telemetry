#include "mainwindow.h"
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->fileExit, &QAction::triggered, this, &MainWindow::exit);

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
    for(int i = 0; i < sensors.size(); i++) {
        voltsSeries->append(i, sensors[i].battVolt);
        currentSeries->append(i, sensors[i].battCur);
    }
    voltsChart->removeAllSeries();
    currentChart->removeAllSeries();
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
    ui->throttleLabel->setText(QString("Throttle: %1%").arg(sensors[sensors.size()-1].throttle));
    ui->tempLabel->setText(QString("Temperature (C): %1°").arg(sensors[sensors.size()-1].battTemp));

    // Battery labels
    ui->voltsLabel->setText(QString("Voltage: %1V").arg(sensors[sensors.size()-1].battVolt));
    ui->currentLabel->setText(QString("Current: %1A").arg(sensors[sensors.size()-1].battCur));
}

void MainWindow::exit(int code) { std::exit(code); }