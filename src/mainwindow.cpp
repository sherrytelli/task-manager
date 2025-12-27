#include "mainwindow.h"

//MainWindow constructor definition
MainWindow::MainWindow(QWidget *parent): QMainWindow(parent) {
    //setting main window title
    setWindowTitle("Task Manager");

    //setting main window size
    resize(600, 700);

    //calling the init function
    init();
}

//MainWindow destructor definition
MainWindow::~MainWindow() {}

void MainWindow::init() {
    //create central widget
    QWidget *centralWidget = new QWidget(this);

    //setting the central widget
    setCentralWidget(centralWidget);

    //populating placeholders
    metricsScreen = new QLabel("Metrics Screen", this);
    processesScreen = new QLabel("Processes Screen", this);

    //centering placeholders
    metricsScreen->setAlignment(Qt::AlignCenter);
    processesScreen->setAlignment(Qt::AlignCenter);

    //changing placeholder font size and background color
    metricsScreen->setStyleSheet("font-size: 24px; background-color: black");
    processesScreen->setStyleSheet("font-size: 24px; background-color: black");

    //creating stacked widget to stack placeholders
    stackedWidget = new QStackedWidget(this);

    //adding placeholders in stacked widget
    stackedWidget->addWidget(metricsScreen);
    stackedWidget->addWidget(processesScreen);

    //creating horizontal layout to place our stacked widgets
    QVBoxLayout *layout = new QVBoxLayout(this);

    //adding stacked widget to layout
    layout->addWidget(stackedWidget);

    //setting the layout for cental widget
    centralWidget->setLayout(layout);

    //creating toolbar to add the central widget
    QToolBar *toolBar = new QToolBar(this);

    //setting up the toolbar
    toolBar->setMovable(false);
    toolBar->setFloatable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    //creating the actions
    metricsAction = new QAction("Metrics", this);
    processesAction = new QAction("Processes", this);

    //adding actions to the toolbar
    toolBar->addAction(metricsAction);
    toolBar->addSeparator();
    toolBar->addAction(processesAction);

    //adding tool to the main window
    addToolBar(Qt::TopToolBarArea, toolBar);

    //connecting actions to the apporpriate functions;
    connect(metricsAction, &QAction::triggered, this, &MainWindow::showMetricsScreen);
    connect(processesAction, &QAction::triggered, this, &MainWindow::showProcessesScreen);

    //initially show metric screen;
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
