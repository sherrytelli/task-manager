#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qt6/QtWidgets/QLabel>
#include <qt6/QtWidgets/QMainWindow>

class MainWindow : public QMainWindow {
    Q_OBJECT;

    public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    private:
    void createLabel();
};

#endif // MAINWINDOW_H
