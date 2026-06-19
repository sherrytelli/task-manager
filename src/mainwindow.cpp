#include "mainwindow.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QScreen>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      statusLabel(new QLabel(this)),
      memoryLabel(new QLabel(this)),
      refreshLabel(new QLabel(this)) {
    setWindowTitle("Task Manager");

    QRect screenGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    int defaultWidth = qMin(900, screenGeometry.width() - 100);
    int defaultHeight = qMin(750, screenGeometry.height() - 100);
    resize(defaultWidth, defaultHeight);

    init();
}

MainWindow::~MainWindow() = default;

void MainWindow::init() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    metricsScreen = new MetricsWidget(this);
    processesScreen = new ProcessWidget(this);

    stackedWidget = new QStackedWidget(this);
    stackedWidget->addWidget(metricsScreen);
    stackedWidget->addWidget(processesScreen);

    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(stackedWidget);

    QToolBar *toolBar = new QToolBar(this);
    toolBar->setMovable(false);
    toolBar->setFloatable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    toolBar->setStyleSheet(
        "QToolBar {"
        "  background-color: #1a1a2e;"
        "  border-bottom: 1px solid #0f3460;"
        "  padding: 0px;"
        "}"
        "QToolBar::separator {"
        "  width: 1px;"
        "  background: #0f3460;"
        "  margin: 6px 4px;"
        "}"
        "QToolButton {"
        "  background-color: transparent;"
        "  color: #a0a0a0;"
        "  border: none;"
        "  border-bottom: 2px solid transparent;"
        "  padding: 10px 20px;"
        "  font-size: 13px;"
        "}"
        "QToolButton:hover {"
        "  background-color: #16213e;"
        "  color: #e0e0e0;"
        "}"
        "QToolButton:checked {"
        "  background-color: #16213e;"
        "  color: #e0e0e0;"
        "  border-bottom: 2px solid #4a90d9;"
        "  font-weight: bold;"
        "}");

    metricsAction = new QAction("Metrics", this);
    metricsAction->setCheckable(true);
    processesAction = new QAction("Processes", this);
    processesAction->setCheckable(true);

    toolBar->addAction(metricsAction);
    toolBar->addSeparator();
    toolBar->addAction(processesAction);

    addToolBar(Qt::TopToolBarArea, toolBar);

    connect(metricsAction, &QAction::triggered, this, &MainWindow::showMetricsScreen);
    connect(processesAction, &QAction::triggered, this, &MainWindow::showProcessesScreen);

    // Status bar
    QStatusBar *statusBar = this->statusBar();
    statusBar->addWidget(statusLabel);
    statusBar->addPermanentWidget(memoryLabel);
    statusBar->addPermanentWidget(refreshLabel);

    updateStatusBar(0, 0, 0);

    connect(processesScreen, &ProcessWidget::refreshComplete, this, &MainWindow::updateStatusBar);

    showMetricsScreen();
}

void MainWindow::showMetricsScreen() {
    stackedWidget->setCurrentIndex(0);
    metricsAction->setChecked(true);
    processesAction->setChecked(false);
}

void MainWindow::showProcessesScreen() {
    stackedWidget->setCurrentIndex(1);
    metricsAction->setChecked(false);
    processesAction->setChecked(true);
}

void MainWindow::updateStatusBar(int processCount, quint64 totalMem, quint64 usedMem) {
    statusLabel->setText(QString("Processes: %1").arg(processCount));

    auto formatMem = [](quint64 bytes) -> QString {
        double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
        return QString("Memory: %1 MB").arg(mb, 0, 'f', 1);
    };

    if (totalMem > 0) {
        double totalMb = static_cast<double>(totalMem) / (1024.0 * 1024.0);
        double usedMb = static_cast<double>(usedMem) / (1024.0 * 1024.0);
        memoryLabel->setText(QString("Memory: %1 / %2 MB (%3%)")
            .arg(usedMb, 0, 'f', 1)
            .arg(totalMb, 0, 'f', 1)
            .arg(static_cast<int>((static_cast<double>(usedMem) / totalMem) * 100.0)));
    } else {
        memoryLabel->setText(formatMem(usedMem));
    }

    refreshLabel->setText(QString("Last refresh: %1")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
}
