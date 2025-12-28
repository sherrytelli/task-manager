#ifndef PROCESSESWIDGET_H
#define PROCESSESWIDGET_H

#include <qt6/QtWidgets/QtWidgets>
#include <qt6/QtWidgets/QTableWidget>
#include <qt6/QtCore/QProcess>

class ProcessWidget: public QWidget {
    Q_OBJECT;

    public:
    ProcessWidget(QWidget *parent = nullptr);

    private:
    //widget to store the process table
    QTableWidget *tableWidget;

    //process variable to store the subprocess
    QProcess *processReader;

    //function to update the process list
    void updateProcessesList();
};

#endif // !PROCESSESWIDGET_H
