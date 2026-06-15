#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Task Manager");
    resize(600, 700);
    init();
}

MainWindow::~MainWindow() = default;

void MainWindow::init() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    metricsScreen = new QLabel("Metrics Screen", this);
    processesScreen = new ProcessWidget(this);

    metricsScreen->setAlignment(Qt::AlignCenter);
    metricsScreen->setStyleSheet("font-size: 24px; background-color: black");

    stackedWidget = new QStackedWidget(this);
    stackedWidget->addWidget(metricsScreen);
    stackedWidget->addWidget(processesScreen);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(stackedWidget);
    centralWidget->setLayout(layout);

    QToolBar *toolBar = new QToolBar(this);
    toolBar->setMovable(false);
    toolBar->setFloatable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    metricsAction = new QAction("Metrics", this);
    processesAction = new QAction("Processes", this);

    toolBar->addAction(metricsAction);
    toolBar->addSeparator();
    toolBar->addAction(processesAction);

    addToolBar(Qt::TopToolBarArea, toolBar);

    connect(metricsAction, &QAction::triggered, this, &MainWindow::showMetricsScreen);
    connect(processesAction, &QAction::triggered, this, &MainWindow::showProcessesScreen);

    showMetricsScreen();
}

void MainWindow::showMetricsScreen() {
    stackedWidget->setCurrentIndex(0);
    metricsAction->setEnabled(false);
    processesAction->setEnabled(true);
}

void MainWindow::showProcessesScreen() {
    stackedWidget->setCurrentIndex(1);
    metricsAction->setEnabled(true);
    processesAction->setEnabled(false);
}
