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

    // Create search bar widget
    searchLineEdit = new QLineEdit(this);

    // Add search bar to layout
    tableLayout->addWidget(searchLineEdit);

    // Connect textChanged signal to filter function
    connect(searchLineEdit, &QLineEdit::textChanged,
            this, &ProcessWidget::filterProcesses);

    setLayout(tableLayout);

    //initialize last search text
    lastSearchText = "";

    //creating the process Directory object;
    procDir = new QDir("/proc/");

    //setting filter for procDir
    procDir->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    //calling the update processProcessList funtion
    updateProcessesList();

    //creating timer to automatically call updateProcessesList after the specified interval
    QTimer *refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &ProcessWidget::updateProcessesList);
    refreshTimer->start(2000); // Start updating every 2 seconds
}

//this function updates the table Widget
void ProcessWidget::updateProcessesList() {
    //resetting all the rows in the table
    tableWidget->setRowCount(0);

    //setting all the processes from procDir
    procDir->refresh();
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

    //reapply the filter after repopulating the table
    filterProcesses(lastSearchText);
}

//this function filters the table based on the search text
void ProcessWidget::filterProcesses(const QString &filterText) {
    // Store the filter text for reapplying after refresh
    lastSearchText = filterText;

    // Get current text to check (trim whitespace)
    QString searchText = filterText.trimmed();

    // Iterate through all rows and hide/show based on match
    for (int row = 0; row < tableWidget->rowCount(); row++) {
        // Get the command from column 2 (0=PID, 1=USER, 2=COMMAND)
        QTableWidgetItem *item = tableWidget->item(row, 2);

        // Check if item exists and if command matches search text
        if (item) {
            QString command = item->text();
            bool match = searchText.isEmpty() ||
                         command.contains(searchText, Qt::CaseInsensitive);

            // Set row visibility
            tableWidget->setRowHidden(row, !match);
        }
    }
}
