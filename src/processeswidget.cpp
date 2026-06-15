#include "processeswidget.h"

ProcessWidget::ProcessWidget(QWidget *parent) : QWidget(parent) {
    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(3);
    tableWidget->setHorizontalHeaderLabels({"PID", "USER", "COMMAND"});
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->horizontalHeader()->setStretchLastSection(false);
    tableWidget->verticalHeader()->setVisible(false);

    QVBoxLayout *tableLayout = new QVBoxLayout(this);
    tableLayout->addWidget(tableWidget);

    searchLineEdit = new QLineEdit(this);
    tableLayout->addWidget(searchLineEdit);

    connect(searchLineEdit, &QLineEdit::textChanged,
            this, &ProcessWidget::filterProcesses);

    setLayout(tableLayout);
    lastSearchText = "";

    procDir = new QDir("/proc/");
    procDir->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    updateProcessesList();

    QTimer *refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &ProcessWidget::updateProcessesList);
    refreshTimer->start(2000);
}

void ProcessWidget::updateProcessesList() {
    tableWidget->setRowCount(0);

    procDir->refresh();
    const QFileInfoList processList = procDir->entryInfoList();

    for (auto &pidDir : processList) {
        // Skip non-numeric PIDs (e.g., kernel threads directories that aren't numeric)
        bool ok = false;
        pidDir.fileName().toInt(&ok);
        if (!ok)
            continue;

        // constructing the path to the processes status file
        const QString pidStatusFilePath = pidDir.absoluteFilePath() + "/status";

        // Stack-allocated QFile (RAII - no manual delete needed)
        QFile statusFileObj(pidStatusFilePath);

        // variable to store the process pid
        const QString processPid = pidDir.fileName();

        // Skip the file if it fails to open (permission denied, deleted process, etc.)
        if (!statusFileObj.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        // Parse the status file line by line, searching for Name: and Uid: labels
        QString processName;
        QString processUid;

        QTextStream statusFileReadStream(&statusFileObj);
        QString line;
        while (statusFileReadStream.readLineInto(&line)) {
            if (line.startsWith("Name:")) {
                processName = line.mid(5).trimmed();
            } else if (line.startsWith("Uid:")) {
                // Uid format: "Uid:\t<real>\teffective\tsaved\tfs" — use real UID (first value)
                QStringList uidParts = line.split('\t', Qt::SkipEmptyParts);
                if (uidParts.size() >= 2) {
                    processUid = uidParts[1].trimmed();
                }
                break;  // Uid is always near the top, no need to read further
            }
        }

        statusFileObj.close();

        // Input validation: skip if we couldn't parse Name or Uid
        // This handles zombie processes, permission-denied, or corrupted status files
        if (processName.isEmpty() || processUid.isEmpty()) {
            continue;
        }

        // adding the data to the table
        const int row = tableWidget->rowCount();
        tableWidget->insertRow(row);
        tableWidget->setItem(row, 0, new QTableWidgetItem(processPid));
        tableWidget->setItem(row, 1, new QTableWidgetItem(processUid));
        tableWidget->setItem(row, 2, new QTableWidgetItem(processName));
    }

    //reapply the filter after repopulating the table
    filterProcesses(lastSearchText);
}

void ProcessWidget::filterProcesses(const QString &filterText) {
    lastSearchText = filterText;
    const QString searchText = filterText.trimmed();

    for (int row = 0; row < tableWidget->rowCount(); row++) {
        QTableWidgetItem *item = tableWidget->item(row, 2);
        if (item) {
            const QString command = item->text();
            const bool match = searchText.isEmpty() ||
                               command.contains(searchText, Qt::CaseInsensitive);
            tableWidget->setRowHidden(row, !match);
        }
    }
}
