#include "processeswidget.h"

ProcessWidget::ProcessWidget(QWidget *parent): QWidget(parent) {
    //creating table
    tableWidget = new QTableWidget(this);

    //setting the no of columns in the table
    tableWidget->setColumnCount(3);

    //setting table headers
    tableWidget->setHorizontalHeaderLabels({"PID", "USER", "COMMAND"});

    //setting table edit triggers
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

    //setting table row selection behaviour
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    //setting table header visibility
    tableWidget->horizontalHeader()->setStretchLastSection(false);
    tableWidget->verticalHeader()->setVisible(false);

    //setting the layout for the table widget
    QVBoxLayout *tableLayout = new QVBoxLayout(this);
    tableLayout->addWidget(tableWidget);
    setLayout(tableLayout);

    //creating the process Directory object;
    procDir = new QDir("/proc/");

    //setting filter for procDir
    procDir->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    //calling the update processProcessList funtion
    updateProcessesList();

    //creating timer to automatically call updateProcessesList after the specified interval
    QTimer::singleShot(2000, this, &ProcessWidget::updateProcessesList);
}

//this function updates the table Widget
void ProcessWidget::updateProcessesList() {
    //resetting all the rows in the table
    tableWidget->setRowCount(0);

    //setting all the processes from procDir
    QFileInfoList processList = procDir->entryInfoList();

    //iterating over the pid folders
    for(auto &pidDir: processList) {
        //constructing the path to the processes status file;
        QString pidStatusFilePath = pidDir.absoluteFilePath() + "/status";

        //creating the file object to read the status file
        QFile *statusFileObj = new QFile(pidStatusFilePath);

        //variable to store the process pid
        QString processpid = pidDir.fileName();

        //skipping the file if it fails to open
        if(!statusFileObj->open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        //variable to store the process name;
        QString processName;

        //variable to store the user id that started the process
        QString processUserID;

        //creating a text stream to read the status file
        QTextStream statusFileReadStream(statusFileObj);

        //variable to store the lines of the read stream;
        QString line;

        //storing the name of the process
        line = statusFileReadStream.readLine();
        processName = line.slice(6);

        //storing the name of user that started the command
        line = statusFileReadStream.readLine();
        line = statusFileReadStream.readLine();
        line = statusFileReadStream.readLine();
        line = statusFileReadStream.readLine();
        line = statusFileReadStream.readLine();
        line = statusFileReadStream.readLine();
        line = statusFileReadStream.readLine();
        line = statusFileReadStream.readLine();
        processUserID = line.slice(5, 4);

        //closing the file after reading
        statusFileObj->close();

        //adding the data to the table
        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);
        tableWidget->setItem(row, 0, new QTableWidgetItem(processpid));
        tableWidget->setItem(row, 1, new QTableWidgetItem(processUserID));
        tableWidget->setItem(row, 2, new QTableWidgetItem(processName));
    }
}
