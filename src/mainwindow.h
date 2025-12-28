#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qt6/QtWidgets/QMainWindow>
#include <qt6/QtWidgets/QtWidgets>
#include <qt6/QtWidgets/QStackedWidget>
#include <qt6/QtWidgets/QVBoxLayout>
#include <qt6/QtGui/QAction>

class MainWindow : public QMainWindow {
    Q_OBJECT;

    public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    private:
    //placeholder for processes screen
    QLabel *processesScreen;
    //placeholder for metrics screen
    QLabel *metricsScreen;

    QStackedWidget *stackedWidget;
    QAction *metricsAction;
    QAction *processesAction;

    void showMetricsScreen();
    void showProcessesScreen();
    void init();
};

#endif // MAINWINDOW_H
