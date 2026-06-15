#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QLabel>
#include <QMainWindow>
#include <QStackedWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include "processeswidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

    public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    private:
    ProcessWidget *processesScreen;
    QLabel *metricsScreen;
    QStackedWidget *stackedWidget;
    QAction *metricsAction;
    QAction *processesAction;
    void showMetricsScreen();
    void showProcessesScreen();
    void init();
};

#endif // MAINWINDOW_H
