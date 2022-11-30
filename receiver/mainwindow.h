#pragma once

#include "version.h"
#include "./ui_mainwindow.h"

#include <QMainWindow>
#include <QGuiApplication>
#include <thread>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
private:
    Ui::MainWindow *ui;
};