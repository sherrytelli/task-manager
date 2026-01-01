#ifndef PROCESSESWIDGET_H
#define PROCESSESWIDGET_H

#include <qt6/QtWidgets/QtWidgets>
#include <qt6/QtWidgets/QTableWidget>
#include <qt6/QtCore/QDir>
#include <qt6/QtCore/QStringList>
#include <qt6/QtWidgets/QVBoxLayout>
#include <qt6/QtCore/QTimer>
#include <qt6/QtCore/QFileInfoList>
#include <qt6/QtCore/QString>
#include <qt6/QtCore/QFile>
#include <qt6/QtCore/QTextStream>
#include <qt6/QtWidgets/QTableWidgetItem>

class ProcessWidget: public QWidget {
    Q_OBJECT;

    public:
    ProcessWidget(QWidget *parent = nullptr);

    private:
    //widget to store the process table
    QTableWidget *tableWidget;

    //object to control "/proc" directory
    QDir *procDir;

    //function to update the process list
    void updateProcessesList();
};

#endif // !PROCESSESWIDGET_H
