#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolBar>
#include "metricswidget.h"
#include "processeswidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    private:
    ProcessWidget *processesScreen;
    MetricsWidget *metricsScreen;
    QStackedWidget *stackedWidget;
    QAction *metricsAction;
    QAction *processesAction;
    QLabel *statusLabel;
    QLabel *memoryLabel;
    QLabel *refreshLabel;
    void showMetricsScreen();
    void showProcessesScreen();
    void init();
    void updateStatusBar(int processCount, quint64 totalMem, quint64 usedMem);
};

#endif // MAINWINDOW_H
