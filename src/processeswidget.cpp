#include "processeswidget.h"
#include "processdetailsdialog.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QInputDialog>
#include <QMessageBox>
#include <QScreen>

#include <csignal>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

static QString formatMemorySize(qint64 bytes) {
    static const char *units[] = {"KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes) / 1024.0;
    while (size >= 1024.0 && unitIndex < 3) {
        size /= 1024.0;
        ++unitIndex;
    }
    return QString("%1 %2").arg(QString::number(size, 'f', 1), units[unitIndex]);
}

ProcessWidget::ProcessWidget(QWidget *parent)
    : QWidget(parent),
      procDir(new QDir("/proc/")),
      refreshTimer(new QTimer(this)) {
    setupTable();
    setupContextMenu();

    procDir->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    updateProcessesList();

    connect(refreshTimer, &QTimer::timeout, this, &ProcessWidget::updateProcessesList);
    refreshTimer->start(refreshIntervalMs);
}

void ProcessWidget::setRefreshInterval(int ms) {
    refreshIntervalMs = ms;
    refreshTimer->start(refreshIntervalMs);
}

void ProcessWidget::setupTable() {
    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(COLUMN_COUNT);
    tableWidget->setHorizontalHeaderLabels(
        {"PID", "USER", "STATE", "CPU%", "MEMORY", "THREADS", "START TIME", "COMMAND LINE"});
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    tableWidget->setSortingEnabled(true);
    tableWidget->verticalHeader()->setVisible(false);

    for (int i = 0; i < COLUMN_COUNT; ++i) {
        if (kColumnWidths[i] > 0) {
            tableWidget->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Fixed);
            tableWidget->horizontalHeader()->resizeSection(i, kColumnWidths[i]);
        } else {
            tableWidget->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
        }
    }

    QVBoxLayout *tableLayout = new QVBoxLayout(this);
    tableLayout->addWidget(tableWidget);

    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText("Search processes...");
    tableLayout->addWidget(searchLineEdit);

    connect(searchLineEdit, &QLineEdit::textChanged, this, &ProcessWidget::filterProcesses);
    connect(tableWidget, &QTableWidget::itemDoubleClicked, this,
            [this](QTableWidgetItem *item) {
                if (item->row() < static_cast<int>(processCache.size())) {
                    showProcessDetails(processCache[item->row()]);
                }
            });
    connect(tableWidget, &QTableWidget::customContextMenuRequested, this,
            [this](const QPoint &pos) { killSelectedProcess(false); });

    lastSearchText = "";
}

void ProcessWidget::setupContextMenu() {
    // Context menu is handled via customContextMenuRequested signal
}

void ProcessWidget::killSelectedProcess(bool force) {
    QList<QTableWidgetSelectionRange> selections = tableWidget->selectedRanges();
    if (selections.isEmpty()) return;

    int row = selections.first().topRow();
    if (row >= static_cast<int>(processCache.size())) return;

    const ProcessInfo &info = processCache[row];

    QString actionText = force ? "Kill Process (Force)" : "Kill Process";
    QString message = force
        ? QString("Are you sure you want to force kill '%1' (PID: %2)?\nThis action cannot be undone.")
              .arg(info.name)
              .arg(info.pid)
        : QString("Are you sure you want to send SIGTERM to '%1' (PID: %2)?")
              .arg(info.name)
              .arg(info.pid);

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Kill", message,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    int signalToSend = force ? 9 : 15;
    int result = kill(static_cast<pid_t>(info.pid), signalToSend);

    if (result != 0) {
        QMessageBox::warning(
            this, "Kill Failed",
            QString("Failed to send signal to process '%1' (PID: %2).\n\n"
                    "This may be due to insufficient permissions or the process ending.")
                .arg(info.name)
                .arg(info.pid));
    }
}

void ProcessWidget::showProcessDetails(const ProcessInfo &info) {
    ProcessDetailsDialog dialog(this);
    dialog.setProcessInfo(info);
    dialog.exec();
}

void ProcessWidget::updateProcessesList() {
    totalMemoryBytes = readTotalMemory();

    procDir->refresh();
    const QFileInfoList pidDirs = procDir->entryInfoList();

    // Build a map of current PIDs for CPU delta calculation
    QMap<qint64, qulonglong> currentCpuTicks;
    QVector<ProcessInfo> newCache;

    for (const auto &pidDir : pidDirs) {
        bool ok = false;
        pidDir.fileName().toInt(&ok);
        if (!ok) continue;

        int pid = pidDir.fileName().toInt();
        ProcessInfo info = readProcessInfo(pid);
        if (info.name.isEmpty()) continue;

        currentCpuTicks[pid] = info.cumulativeCpuTicks;
        newCache.append(info);
    }

    // Calculate CPU% from deltas
    for (int i = 0; i < newCache.size(); ++i) {
        qint64 pid = newCache[i].pid;
        qulonglong currentTicks = currentCpuTicks.value(pid, 0);
        qulonglong prevTicks = previousCpuTimes.value(pid, 0);

        if (prevTicks > 0) {
            qulonglong tickDelta = currentTicks - prevTicks;
            // refreshIntervalMs is in milliseconds, convert to seconds for percentage
            double cpuPercent = (static_cast<double>(tickDelta) / (refreshIntervalMs * 10.0));
            // Cap at number of CPU cores
            int numCores = sysconf(_SC_NPROCESSORS_ONLN);
            if (cpuPercent > numCores) cpuPercent = static_cast<double>(numCores);
            newCache[i].cpuPercent = cpuPercent;
        }

        // Calculate memory percentage using RSS + Unshared
        if (totalMemoryBytes > 0) {
            newCache[i].memoryPercent = (static_cast<double>(newCache[i].rssBytes + newCache[i].unsharedRssBytes) / totalMemoryBytes) * 100.0;
        } else {
            newCache[i].memoryPercent = 0.0;
        }
    }

    // Store current CPU ticks for next iteration
    previousCpuTimes = currentCpuTicks;

    // Update cache
    processCache = newCache;

    // Rebuild table
    tableWidget->setRowCount(0);

    for (int i = 0; i < processCache.size(); ++i) {
        const ProcessInfo &info = processCache[i];
        const int row = tableWidget->rowCount();
        tableWidget->insertRow(row);

        QTableWidgetItem *pidItem = new QTableWidgetItem(QString::number(info.pid));
        pidItem->setData(Qt::DisplayRole, info.pid);
        pidItem->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(row, COL_PID, pidItem);

        QTableWidgetItem *userItem = new QTableWidgetItem(info.user);
        userItem->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(row, COL_USER, userItem);

        QTableWidgetItem *stateItem = new QTableWidgetItem(info.state);
        stateItem->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(row, COL_STATE, stateItem);

        QTableWidgetItem *cpuItem = new QTableWidgetItem(QString("%1%").arg(QString::number(info.cpuPercent, 'f', 1)));
        cpuItem->setData(Qt::DisplayRole, info.cpuPercent);
        cpuItem->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(row, COL_CPU_PERCENT, cpuItem);

        QTableWidgetItem *memItem = new QTableWidgetItem(formatMemorySize(info.rssBytes + info.unsharedRssBytes));
        memItem->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(row, COL_MEMORY, memItem);

        QTableWidgetItem *threadsItem = new QTableWidgetItem(QString::number(info.threadCount));
        threadsItem->setData(Qt::DisplayRole, info.threadCount);
        threadsItem->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(row, COL_THREADS, threadsItem);

        QTableWidgetItem *timeItem = new QTableWidgetItem(info.startTime);
        timeItem->setTextAlignment(Qt::AlignCenter);
        tableWidget->setItem(row, COL_START_TIME, timeItem);

        QTableWidgetItem *cmdItem = new QTableWidgetItem(info.commandLine);
        tableWidget->setItem(row, COL_COMMAND_LINE, cmdItem);
    }

    filterProcesses(lastSearchText);

    quint64 usedMemory = 0;
    for (const auto &info : processCache) {
        usedMemory += static_cast<quint64>(info.rssBytes + info.unsharedRssBytes);
    }
    emit refreshComplete(processCache.size(), totalMemoryBytes, usedMemory);
}

ProcessInfo ProcessWidget::readProcessInfo(int pid) {
    ProcessInfo info;
    info.pid = pid;

    // Read /proc/[pid]/status
    QFile statusFile(QString("/proc/%1/status").arg(pid));
    if (!statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return info;
    }

    QTextStream statusStream(&statusFile);
    QString line;
    while (statusStream.readLineInto(&line)) {
        if (line.startsWith("Name:")) {
            info.name = line.mid(5).trimmed();
        } else if (line.startsWith("Uid:")) {
            QStringList uidParts = line.split('\t', Qt::SkipEmptyParts);
            if (uidParts.size() >= 2) {
                bool uidOk = false;
                int uidVal = uidParts[1].trimmed().toInt(&uidOk);
                if (uidOk) {
                    struct passwd *pw = getpwuid(uidVal);
                    if (pw) {
                        info.user = QString::fromLocal8Bit(pw->pw_name);
                    } else {
                        info.user = QString::number(uidVal);
                    }
                }
            }
        } else if (line.startsWith("State:")) {
            QString stateStr = line.mid(6).trimmed();
            if (!stateStr.isEmpty()) {
                info.state = stateStr;
            }
        } else if (line.startsWith("Threads:")) {
            QString threadsStr = line.mid(8).trimmed();
            bool threadsOk = false;
            info.threadCount = threadsStr.toInt(&threadsOk);
            if (!threadsOk) info.threadCount = 0;
        } else if (line.startsWith("VmRSS:")) {
            QString rssStr = line.split(':').last().trimmed();
            bool rssOk = false;
            qint64 rssKb = rssStr.split(' ', Qt::SkipEmptyParts).first().toLongLong(&rssOk);
            if (rssOk) {
                info.rssBytes = rssKb * 1024;
            }
        }
    }
    statusFile.close();

    // Read /proc/[pid]/statm for shared pages (KDE ksysguardd approach)
    // statm format: size resident shared text lib data dt
    // Fields are in pages. Shared = shared pages (shared libraries, mmapped files, page table entries)
    QFile statmFile(QString("/proc/%1/statm").arg(pid));
    if (statmFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream statmStream(&statmFile);
        QString statmLine = statmStream.readLine();
        statmFile.close();

        QStringList statmFields = statmLine.split(' ', Qt::SkipEmptyParts);
        if (statmFields.size() >= 3) {
            bool sharedOk = false;
            qint64 sharedPages = statmFields[2].toLongLong(&sharedOk);
            if (sharedOk) {
                info.sharedPagesKb = sharedPages * (qint64)sysconf(_SC_PAGESIZE) / 1024;
            }
        }
    }

    // Calculate unshared RSS (VmURss) - KDE ksysguardd approach
    // Unshared RSS = RSS - shared pages from statm
    info.unsharedRssBytes = info.rssBytes - info.sharedPagesKb * 1024;
    if (info.unsharedRssBytes < 0) {
        info.unsharedRssBytes = 0;
    }

    if (info.name.isEmpty()) return info;

    // Read /proc/[pid]/stat
    QFile statFile(QString("/proc/%1/stat").arg(pid));
    if (statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream statStream(&statFile);
        QString statLine = statStream.readLine();
        statFile.close();

        // Parse /proc/[pid]/stat carefully - comm field may contain spaces and parentheses
        // Format: pid (comm) state fields...
        int commEnd = statLine.lastIndexOf(')');
        if (commEnd > 0) {
            QString remaining = statLine.mid(commEnd + 2);  // skip ") "
            QStringList statFields = remaining.split(' ', Qt::SkipEmptyParts);
            // statFields[0] = state (field 3)
            // statFields[1] = ppid (field 4)
            // statFields[11] = utime (field 13)
            // statFields[12] = stime (field 14)
            // statFields[17] = starttime (field 22)
            // statFields[18] = num_threads (field 20)

            if (statFields.size() >= 19) {
                // Parent PID
                bool ppidOk = false;
                info.parentPid = statFields[1].toLongLong(&ppidOk);

                // CPU times (in clock ticks)
                static long ticksPerSecond = sysconf(_SC_CLK_TCK);
                if (ticksPerSecond <= 0) ticksPerSecond = 100;

                bool utimeOk = false, stimeOk = false;
                qulonglong utime = statFields[11].toULongLong(&utimeOk);
                qulonglong stime = statFields[12].toULongLong(&stimeOk);
                if (utimeOk && stimeOk) {
                    info.cumulativeCpuTicks = utime + stime;
                }

                // Start time (convert from clock ticks to human readable)
                bool startOk = false;
                qulonglong startTimeTicks = statFields[17].toULongLong(&startOk);
                if (startOk && ticksPerSecond > 0) {
                    info.startTime = formatUptime(startTimeTicks, ticksPerSecond);
                }

                // Num threads (use stat value if status didn't have one)
                if (info.threadCount <= 0) {
                    bool threadsOk = false;
                    int statThreads = statFields[18].toInt(&threadsOk);
                    if (threadsOk && statThreads > 0) {
                        info.threadCount = statThreads;
                    }
                }
            }
        }
    }

    // Read /proc/[pid]/cmdline
    QFile cmdlineFile(QString("/proc/%1/cmdline").arg(pid));
    if (cmdlineFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray cmdlineData = cmdlineFile.readAll();
        cmdlineFile.close();

        if (!cmdlineData.isEmpty()) {
            cmdlineData.replace('\0', ' ');
            cmdlineData = cmdlineData.trimmed();
            info.commandLine = QString::fromLocal8Bit(cmdlineData);
        }
    }

    // Fallback: if command line is empty, use the name
    if (info.commandLine.isEmpty()) {
        info.commandLine = info.name;
    }

    return info;
}

quint64 ProcessWidget::readTotalMemory() {
    QFile meminfoFile("/proc/meminfo");
    if (!meminfoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }

    quint64 totalMemKb = 0;
    QTextStream meminfoStream(&meminfoFile);
    QString line;
    while (meminfoStream.readLineInto(&line)) {
        if (line.startsWith("MemTotal:")) {
            QString valueStr = line.split(':').last().trimmed();
            bool ok = false;
            totalMemKb = valueStr.split(' ', Qt::SkipEmptyParts).first().toULongLong(&ok);
            break;
        }
    }
    meminfoFile.close();
    return totalMemKb * 1024;
}

QString ProcessWidget::formatUptime(qint64 ticks, long ticksPerSecond) const {
    if (ticksPerSecond <= 0) ticksPerSecond = 100;

    qint64 seconds = ticks / ticksPerSecond;
    qint64 totalMinutes = seconds / 60;
    qint64 totalHours = totalMinutes / 60;
    qint64 days = totalHours / 24;

    if (days > 0) {
        return QString("%1d %2h").arg(days).arg(totalHours % 24);
    } else if (totalHours > 0) {
        return QString("%1h %2m").arg(totalHours).arg(totalMinutes % 60);
    } else {
        return QString("%1m %2s").arg(totalMinutes).arg(seconds % 60);
    }
}

void ProcessWidget::filterProcesses(const QString &filterText) {
    lastSearchText = filterText;
    const QString searchText = filterText.trimmed();

    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        bool match = searchText.isEmpty();
        if (!match) {
            for (int col = 0; col < COLUMN_COUNT; ++col) {
                QTableWidgetItem *item = tableWidget->item(row, col);
                if (item && item->text().contains(searchText, Qt::CaseInsensitive)) {
                    match = true;
                    break;
                }
            }
        }
        tableWidget->setRowHidden(row, !match);
    }
}
