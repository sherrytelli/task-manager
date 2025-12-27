#include "mainwindow.h"

//MainWindow constructor definition
MainWindow::MainWindow(QWidget *parent): QMainWindow(parent) {
    setWindowTitle("Task Manager");
    resize(600, 800);
}

//MainWindow destructor definition
MainWindow::~MainWindow() {}
