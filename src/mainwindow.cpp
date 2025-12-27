#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), CentralWidget(new QWidget(this)), Layout(new QVBoxLayout(this)), HelloLabel(new QLabel(this)) {
    setupUI();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    HelloLabel->setAlignment(Qt::AlignCenter);
    HelloLabel->setStyleSheet("QLabel { font-size: 24px; font-weight: bold; color: #333; }");
    
    Layout->addWidget(HelloLabel);
    Layout->setAlignment(Qt::AlignCenter);
    
    setCentralWidget(CentralWidget);
    
    setWindowTitle("Task Manager");
    resize(400, 300);
}
