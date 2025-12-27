#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <qt6/QtWidgets/QLabel>
#include <qt6/QtWidgets/QMainWindow>
#include <qt6/QtWidgets/QVBoxLayout>
#include <qt6/QtWidgets/QWidget>

class MainWindow : public QMainWindow {
    private:
    QWidget *CentralWidget;
    QVBoxLayout *Layout;
    QLabel *HelloLabel;

    public:
    Q_OBJECT;

    public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    private:
    void setupUI();
};

#endif // MAINWINDOW_H
