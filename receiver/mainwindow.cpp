#include "mainwindow.h"
#include "radio.h"
#include <QTimer>
#include <QValueAxis>
#include <QFileDialog>
#include <QMessageBox>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qRegisterMetaType<sensor_data>();
    qRegisterMetaType<gps_pos>();
    lastRx = -1;

    // Signals
    connect(ui->fileExit, &QAction::triggered, this, &MainWindow::exit);
    connect(ui->fileConnect, &QAction::triggered, this, &MainWindow::connectTelem);
    connect(ui->fileDisconnect, &QAction::triggered, this, &MainWindow::disconnectTelem);
    connect(ui->fileExport, &QAction::triggered, this, &MainWindow::exportJson);

    // Create graph objects
    voltsSeries = new QLineSeries();
    currentSeries = new QLineSeries();
    voltsChart = new QChart();
    currentChart = new QChart();

    // Set up graph appearance; no background, no legend, anti aliased
    voltsChart->legend()->hide();
    currentChart->legend()->hide();
    voltsChart->setBackgroundVisible(false);
    currentChart->setBackgroundVisible(false);
    ui->voltsGraph->setRenderHint(QPainter::Antialiasing);
    ui->currentGraph->setRenderHint(QPainter::Antialiasing);

    // Add the new series to the graphs
    // they are currently empty but this is still needed for creating axes
    voltsChart->addSeries(voltsSeries);
    currentChart->addSeries(currentSeries);

    // Set up graph axes
    voltsChart->createDefaultAxes();
    currentChart->createDefaultAxes();
    voltsChart->axes(Qt::Horizontal)[0]->setLabelsVisible(false);
    voltsChart->axes(Qt::Vertical)[0]->setLabelsColor(QApplication::palette().text().color());
    voltsChart->axes(Qt::Vertical)[0]->setTitleText("Battery Voltage (V)");
    voltsChart->axes(Qt::Vertical)[0]->setTitleBrush(QBrush(QApplication::palette().text().color()));
    currentChart->axes(Qt::Horizontal)[0]->setLabelsVisible(false);
    currentChart->axes(Qt::Vertical)[0]->setLabelsColor(QApplication::palette().text().color());
    currentChart->axes(Qt::Vertical)[0]->setTitleText("Battery Current (A)");
    currentChart->axes(Qt::Vertical)[0]->setTitleBrush(QBrush(QApplication::palette().text().color()));
    currentChart->axes(Qt::Vertical)[0]->setRange(0, 100); // 100A max

    // Add the charts to the UI and update text labels
    ui->voltsGraph->setChart(voltsChart);
    ui->currentGraph->setChart(currentChart);
    updateData();
}

void MainWindow::packetCallback(sensor_data sensors, gps_pos gps)
{
    // Append the new data to the graphs
    voltsSeries->append(this->sensors.size(), sensors.battVolt);
    currentSeries->append(this->sensors.size(), sensors.battCur);

    // Update the axes to fit the new data
    // TODO: Find a cleaner way to do this
    float highestVoltage = sensors.battVolt;
    for(int i = 0; i < this->sensors.size(); i++)
        if(this->sensors[i].battVolt > highestVoltage)
            highestVoltage = this->sensors[i].battVolt;
    voltsChart->axes(Qt::Vertical)[0]->setRange(0, highestVoltage);
    float highestAmperage = sensors.battCur;
    for(int i = 0; i < this->sensors.size(); i++)
        if(this->sensors[i].battCur > highestAmperage)
            highestAmperage = this->sensors[i].battCur;
    currentChart->axes(Qt::Vertical)[0]->setRange(0, highestAmperage);

    // Redraw the graphs
    voltsChart->removeSeries(voltsSeries);
    currentChart->removeSeries(currentSeries);
    voltsChart->addSeries(voltsSeries);
    currentChart->addSeries(currentSeries);
    
    // Add the sensor data to a vector for future use (exporting)
    this->sensors.push_back(sensors);
    this->times.push_back(time(NULL));
    lastRx = time(NULL);

    // Update the GUI labels
    updateData();
}

void MainWindow::updateData()
{
    // If there's no data, do nothing
    if(sensors.size() <= 0)
        return;
        
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

void MainWindow::exportJson()
{
    // Export the sensor data to json
    nlohmann::json j;
    for(int i = 0; i < sensors.size(); i++) { // Add data to the json file
        j["packets"][i]["time"] = times[i];
        j["packets"][i]["throttle"] = sensors[i].throttle;
        j["packets"][i]["battVolt"] = sensors[i].battVolt;
        j["packets"][i]["battCur"] = sensors[i].battCur;
        j["packets"][i]["battTemp"] = sensors[i].battTemp;
    }

    // Save the file
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "", tr("JSON Files (*.json)"));
    if(fileName != "") {
        // If the file name doesn't have the correct json extension, add it
        // TODO: Move this to the QFileDialog itself, so the extension is considered in replace warnings
        if(!fileName.endsWith(".json"))
            fileName += ".json";
        std::ofstream file(fileName.toStdString());
        file << j.dump(4);
        file.close();
    }
    else
        QMessageBox::warning(this, "Error", "Cannot save file. Location was not valid.");
}

void MainWindow::disconnectTelem() { Radio::close(); }
#ifdef GUI_RX
void MainWindow::connectTelem() { Radio::init(this); }
#endif
void MainWindow::exit(int code) { std::exit(code); }