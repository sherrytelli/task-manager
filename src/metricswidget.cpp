#include "metricswidget.h"

#include <QDateTime>
#include <QFile>
#include <QHeaderView>
#include <QTextStream>
#include <unistd.h>

static QString formatMemorySize(quint64 bytes) {
    static const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        ++unitIndex;
    }
    return QString("%1 %2").arg(QString::number(size, 'f', 1), units[unitIndex]);
}

static QString formatBytesPerSecond(quint64 bytes) {
    static const char *units[] = {"B/s", "KB/s", "MB/s", "GB/s"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && unitIndex < 3) {
        size /= 1024.0;
        ++unitIndex;
    }
    return QString("%1 %2").arg(QString::number(size, 'f', 1), units[unitIndex]);
}

MetricsWidget::MetricsWidget(QWidget *parent)
    : QWidget(parent),
      refreshTimer(new QTimer(this)) {
    setupLayout();
    setRefreshInterval(refreshIntervalMs);
    updateCards();
    connect(refreshTimer, &QTimer::timeout, this, &MetricsWidget::updateCards);
}

void MetricsWidget::setRefreshInterval(int ms) {
    refreshIntervalMs = ms;
    refreshTimer->start(refreshIntervalMs);
}

void MetricsWidget::setupLayout() {
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QScrollArea::NoFrame);

    QWidget *scrollContent = new QWidget(scrollArea);
    QVBoxLayout *mainLayout = new QVBoxLayout(scrollContent);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // --- CPU Card ---
    QWidget *cpuCard = new QWidget(this);
    cpuCard->setStyleSheet(
        "QWidget {"
        "  background-color: #16213e;"
        "  border: 1px solid #0f3460;"
        "  border-radius: 6px;"
        "}");
    QVBoxLayout *cpuLayout = new QVBoxLayout(cpuCard);
    cpuLayout->setContentsMargins(12, 12, 12, 12);
    cpuLayout->setSpacing(8);

    QLabel *cpuTitle = new QLabel("CPU", cpuCard);
    cpuTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #e0e0e0;");
    cpuLayout->addWidget(cpuTitle);

    QHBoxLayout *cpuTopRow = new QHBoxLayout();
    cpuTopRow->setSpacing(8);
    QLabel *cpuUsageLabelTitle = new QLabel("Usage:", cpuCard);
    cpuUsageLabelTitle->setStyleSheet("color: #a0a0a0; font-size: 12px;");
    cpuUsageLabel = new QLabel("0.0%", cpuCard);
    cpuUsageLabel->setStyleSheet("color: #e0e0e0; font-size: 18px; font-weight: bold;");
    cpuUsageLabel->setMinimumWidth(80);
    cpuTopRow->addWidget(cpuUsageLabelTitle);
    cpuTopRow->addWidget(cpuUsageLabel);
    cpuTopRow->addStretch();
    cpuLayout->addLayout(cpuTopRow);

    QHBoxLayout *cpuLoadRow = new QHBoxLayout();
    cpuLoadRow->setSpacing(8);
    QLabel *cpuLoadLabel = new QLabel("Load Avg:", cpuCard);
    cpuLoadLabel->setStyleSheet("color: #a0a0a0; font-size: 12px;");
    cpuLoadAvgLabel = new QLabel("N/A, N/A, N/A", cpuCard);
    cpuLoadAvgLabel->setStyleSheet("color: #e0e0e0; font-size: 12px;");
    cpuLoadAvgLabel->setMinimumWidth(120);
    cpuLoadRow->addWidget(cpuLoadLabel);
    cpuLoadRow->addWidget(cpuLoadAvgLabel);
    cpuLoadRow->addStretch();
    cpuLayout->addLayout(cpuLoadRow);

    cpuScrollArea = new QScrollArea(cpuCard);
    cpuScrollArea->setFrameShape(QScrollArea::NoFrame);
    cpuScrollArea->setMaximumHeight(120);
    cpuCoreTable = new QTableWidget(cpuScrollArea);
    cpuCoreTable->setColumnCount(2);
    cpuCoreTable->setHorizontalHeaderLabels({"Core", "Usage"});
    cpuCoreTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    cpuCoreTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    cpuCoreTable->horizontalHeader()->resizeSection(1, 80);
    cpuCoreTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    cpuCoreTable->verticalHeader()->setVisible(false);
    cpuCoreTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #1a2744;"
        "  border: none;"
        "  font-size: 11px;"
        "  color: #e0e0e0;"
        "} "
        "QTableWidget::item {"
        "  padding: 2px;"
        "}");
    cpuScrollArea->setWidget(cpuCoreTable);
    cpuLayout->addWidget(cpuScrollArea);

    // --- Memory Card ---
    QWidget *memCard = new QWidget(this);
    memCard->setStyleSheet(
        "QWidget {"
        "  background-color: #16213e;"
        "  border: 1px solid #0f3460;"
        "  border-radius: 6px;"
        "}");
    QVBoxLayout *memLayout = new QVBoxLayout(memCard);
    memLayout->setContentsMargins(12, 12, 12, 12);
    memLayout->setSpacing(8);

    QLabel *memTitle = new QLabel("Memory", memCard);
    memTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #e0e0e0;");
    memLayout->addWidget(memTitle);

    QHBoxLayout *memTotalRow = new QHBoxLayout();
    memTotalRow->setSpacing(8);
    QLabel *memTotalLabelTitle = new QLabel("Total:", memCard);
    memTotalLabelTitle->setStyleSheet("color: #a0a0a0; font-size: 12px;");
    memTotalLabel = new QLabel("0 B", memCard);
    memTotalLabel->setStyleSheet("color: #e0e0e0; font-size: 12px;");
    memTotalLabel->setMinimumWidth(80);
    memTotalRow->addWidget(memTotalLabelTitle);
    memTotalRow->addWidget(memTotalLabel);
    memTotalRow->addStretch();
    memLayout->addLayout(memTotalRow);

    QHBoxLayout *memUsedRow = new QHBoxLayout();
    memUsedRow->setSpacing(8);
    QLabel *memUsedLabelTitle = new QLabel("Used:", memCard);
    memUsedLabelTitle->setStyleSheet("color: #a0a0a0; font-size: 12px;");
    memUsedLabel = new QLabel("0 B", memCard);
    memUsedLabel->setStyleSheet("color: #e0e0e0; font-size: 12px;");
    memUsedLabel->setMinimumWidth(80);
    memUsedRow->addWidget(memUsedLabelTitle);
    memUsedRow->addWidget(memUsedLabel);
    memUsedRow->addStretch();
    memLayout->addLayout(memUsedRow);

    QHBoxLayout *memPctRow = new QHBoxLayout();
    memPctRow->setSpacing(8);
    QLabel *memPctLabelTitle = new QLabel("Usage:", memCard);
    memPctLabelTitle->setStyleSheet("color: #a0a0a0; font-size: 12px;");
    memProgressBar = new QProgressBar(memCard);
    memProgressBar->setRange(0, 100);
    memProgressBar->setValue(0);
    memProgressBar->setStyleSheet(
        "QProgressBar {"
        "  background-color: #1a2744;"
        "  border: 1px solid #0f3460;"
        "  border-radius: 3px;"
        "  text-align: center;"
        "  height: 16px;"
        "  font-size: 11px;"
        "  color: #e0e0e0;"
        "} "
        "QProgressBar::chunk {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 #4ade80, stop:1 #22c55e);"
        "  border-radius: 2px;"
        "}");
    memProgressBar->setFixedWidth(150);
    memPercentLabel = new QLabel("0%", memCard);
    memPercentLabel->setStyleSheet("color: #e0e0e0; font-size: 12px; font-weight: bold;");
    memPctRow->addWidget(memPctLabelTitle);
    memPctRow->addWidget(memProgressBar);
    memPctRow->addWidget(memPercentLabel);
    memPctRow->addStretch();
    memLayout->addLayout(memPctRow);

    QHBoxLayout *memBufRow = new QHBoxLayout();
    memBufRow->setSpacing(8);
    QLabel *memBufLabelTitle = new QLabel("Buffers/Cache:", memCard);
    memBufLabelTitle->setStyleSheet("color: #a0a0a0; font-size: 12px;");
    memBuffersLabel = new QLabel("0 B / 0 B", memCard);
    memBuffersLabel->setStyleSheet("color: #e0e0e0; font-size: 12px;");
    memBufRow->addWidget(memBufLabelTitle);
    memBufRow->addWidget(memBuffersLabel);
    memBufRow->addStretch();
    memLayout->addLayout(memBufRow);

    QHBoxLayout *memSwapRow = new QHBoxLayout();
    memSwapRow->setSpacing(8);
    QLabel *memSwapLabelTitle = new QLabel("Swap:", memCard);
    memSwapLabelTitle->setStyleSheet("color: #a0a0a0; font-size: 12px;");
    memSwapLabel = new QLabel("0 B / 0 B", memCard);
    memSwapLabel->setStyleSheet("color: #e0e0e0; font-size: 12px;");
    memSwapLabel->setMinimumWidth(80);
    memSwapRow->addWidget(memSwapLabelTitle);
    memSwapRow->addWidget(memSwapLabel);
    memSwapRow->addStretch();
    memLayout->addLayout(memSwapRow);

    // --- Network Card ---
    QWidget *netCard = new QWidget(this);
    netCard->setStyleSheet(
        "QWidget {"
        "  background-color: #16213e;"
        "  border: 1px solid #0f3460;"
        "  border-radius: 6px;"
        "}");
    QVBoxLayout *netLayout = new QVBoxLayout(netCard);
    netLayout->setContentsMargins(12, 12, 12, 12);
    netLayout->setSpacing(8);

    QLabel *netTitle = new QLabel("Network", netCard);
    netTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #e0e0e0;");
    netLayout->addWidget(netTitle);

    networkScrollArea = new QScrollArea(netCard);
    networkScrollArea->setFrameShape(QScrollArea::NoFrame);
    networkScrollArea->setWidgetResizable(true);
    networkTable = new QTableWidget(networkScrollArea);
    networkTable->setColumnCount(6);
    networkTable->setHorizontalHeaderLabels({"Interface", "RX", "TX", "RX Pkts", "TX Pkts", "Err"});
    networkTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    networkTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    networkTable->horizontalHeader()->resizeSection(1, 100);
    networkTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    networkTable->horizontalHeader()->resizeSection(2, 100);
    networkTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    networkTable->horizontalHeader()->resizeSection(3, 80);
    networkTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    networkTable->horizontalHeader()->resizeSection(4, 80);
    networkTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Fixed);
    networkTable->horizontalHeader()->resizeSection(5, 50);
    networkTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    networkTable->verticalHeader()->setVisible(false);
    networkTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #1a2744;"
        "  border: none;"
        "  font-size: 11px;"
        "  color: #e0e0e0;"
        "} "
        "QTableWidget::item {"
        "  padding: 2px;"
        "}");
    networkScrollArea->setWidget(networkTable);
    netLayout->addWidget(networkScrollArea);

    // --- Uptime Card ---
    QWidget *uptimeCard = new QWidget(this);
    uptimeCard->setStyleSheet(
        "QWidget {"
        "  background-color: #16213e;"
        "  border: 1px solid #0f3460;"
        "  border-radius: 6px;"
        "}");
    QVBoxLayout *uptimeLayout = new QVBoxLayout(uptimeCard);
    uptimeLayout->setContentsMargins(12, 12, 12, 12);
    uptimeLayout->setSpacing(8);

    QLabel *uptimeTitle = new QLabel("System Uptime", uptimeCard);
    uptimeTitle->setStyleSheet("font-size: 14px; font-weight: bold; color: #e0e0e0;");
    uptimeLayout->addWidget(uptimeTitle);

    QHBoxLayout *uptimeRow = new QHBoxLayout();
    uptimeRow->setSpacing(8);
    QLabel *uptimeLabelTitle = new QLabel("Uptime:", uptimeCard);
    uptimeLabelTitle->setStyleSheet("color: #a0a0a0; font-size: 12px;");
    uptimeLabel = new QLabel("N/A", uptimeCard);
    uptimeLabel->setStyleSheet("color: #e0e0e0; font-size: 12px;");
    uptimeLabel->setMinimumWidth(100);
    uptimeRow->addWidget(uptimeLabelTitle);
    uptimeRow->addWidget(uptimeLabel);
    uptimeRow->addStretch();
    uptimeLayout->addLayout(uptimeRow);

    QHBoxLayout *procRow = new QHBoxLayout();
    procRow->setSpacing(8);
    QLabel *procLabelTitle = new QLabel("Processes:", uptimeCard);
    procLabelTitle->setStyleSheet("color: #a0a0a0; font-size: 12px;");
    processCountLabel = new QLabel("0", uptimeCard);
    processCountLabel->setStyleSheet("color: #e0e0e0; font-size: 12px;");
    processCountLabel->setMinimumWidth(40);
    procRow->addWidget(procLabelTitle);
    procRow->addWidget(processCountLabel);
    procRow->addStretch();
    uptimeLayout->addLayout(procRow);

    QHBoxLayout *threadRow = new QHBoxLayout();
    threadRow->setSpacing(8);
    QLabel *threadLabelTitle = new QLabel("Threads:", uptimeCard);
    threadLabelTitle->setStyleSheet("color: #a0a0a0; font-size: 12px;");
    threadCountLabel = new QLabel("0", uptimeCard);
    threadCountLabel->setStyleSheet("color: #e0e0e0; font-size: 12px;");
    threadCountLabel->setMinimumWidth(40);
    threadRow->addWidget(threadLabelTitle);
    threadRow->addWidget(threadCountLabel);
    threadRow->addStretch();
    uptimeLayout->addLayout(threadRow);

    // --- Assemble layout ---
    QGridLayout *topGrid = new QGridLayout();
    topGrid->setSpacing(12);
    topGrid->addWidget(cpuCard, 0, 0, 2, 1);
    topGrid->addWidget(memCard, 0, 1, 1, 1);
    topGrid->addWidget(netCard, 1, 1, 1, 1);

    mainLayout->addLayout(topGrid);
    mainLayout->addWidget(uptimeCard);
    mainLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(scrollArea);
}

void MetricsWidget::updateCards() {
    currentCpu = readCpuMetrics();
    currentMemory = readMemoryMetrics();
    currentSystem = readSystemMetrics();

    // Collect all interfaces from /proc/net/dev
    currentNetwork.clear();
    QFile netDevFile("/proc/net/dev");
    if (netDevFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream netDevStream(&netDevFile);
        QString line;
        bool skipHeader = true;
        while (netDevStream.readLineInto(&line)) {
            if (skipHeader) {
                skipHeader = false;
                continue;
            }
            if (line.trimmed().isEmpty()) continue;

            int colonPos = line.indexOf(':');
            if (colonPos < 0) continue;

            QString iface = line.left(colonPos).trimmed();
            if (iface.isEmpty()) continue;

            QStringList fields = line.mid(colonPos + 1).split(' ', Qt::SkipEmptyParts);
            if (fields.size() < 16) continue;

            NetworkMetrics nm;
            nm.interface = iface;
            nm.bytesReceived = fields[0].toULongLong();
            nm.packetsReceived = fields[1].toULongLong();
            nm.errorsIn = fields[7].toULongLong();
            nm.bytesSent = fields[8].toULongLong();
            nm.packetsSent = fields[9].toULongLong();
            nm.errorsOut = fields[15].toULongLong();

            // Calculate delta from previous reading
            for (int i = 0; i < previousNetwork.size(); ++i) {
                if (previousNetwork[i].interface == iface) {
                    nm.bytesReceived = fields[0].toULongLong() - previousNetwork[i].bytesReceived;
                    nm.bytesSent = fields[8].toULongLong() - previousNetwork[i].bytesSent;
                    break;
                }
            }

            currentNetwork.append(nm);
        }
        netDevFile.close();
    }

    // CPU card
    cpuUsageLabel->setText(
        QString("%1%").arg(QString::number(currentCpu.totalUsagePercent, 'f', 1)));

    cpuLoadAvgLabel->setText(
        QString("%1, %2, %3")
            .arg(currentCpu.loadAvg1m)
            .arg(currentCpu.loadAvg5m)
            .arg(currentCpu.loadAvg15m));

    cpuCoreTable->setRowCount(currentCpu.perCoreUsage.size());
    for (int i = 0; i < currentCpu.perCoreUsage.size(); ++i) {
        QTableWidgetItem *coreItem = new QTableWidgetItem(
            QString("Core %1").arg(i));
        coreItem->setFlags(coreItem->flags() & ~Qt::ItemIsEditable);
        cpuCoreTable->setItem(i, 0, coreItem);

        QTableWidgetItem *usageItem = new QTableWidgetItem(
            QString("%1%").arg(QString::number(currentCpu.perCoreUsage[i], 'f', 1)));
        usageItem->setFlags(usageItem->flags() & ~Qt::ItemIsEditable);
        usageItem->setTextAlignment(Qt::AlignCenter);
        cpuCoreTable->setItem(i, 1, usageItem);
    }

    // Memory card
    memTotalLabel->setText(formatMemorySize(currentMemory.totalBytes));
    memUsedLabel->setText(formatMemorySize(currentMemory.usedBytes));
    memBuffersLabel->setText(
        QString("%1 / %2")
            .arg(formatMemorySize(currentMemory.buffersBytes))
            .arg(formatMemorySize(currentMemory.cachedBytes)));

    if (currentMemory.totalBytes > 0) {
        double memPercent =
            (static_cast<double>(currentMemory.usedBytes) / currentMemory.totalBytes) * 100.0;
        memPercent = qMin(memPercent, 100.0);
        memPercentLabel->setText(QString("%1%").arg(QString::number(memPercent, 'f', 1)));
        memProgressBar->setValue(static_cast<int>(memPercent));
    } else {
        memPercentLabel->setText("0%");
        memProgressBar->setValue(0);
    }

    memSwapLabel->setText(
        QString("%1 / %2")
            .arg(formatMemorySize(currentMemory.swapUsedBytes))
            .arg(formatMemorySize(currentMemory.swapTotalBytes)));

    // Network card
    networkTable->setRowCount(currentNetwork.size());
    for (int i = 0; i < currentNetwork.size(); ++i) {
        const NetworkMetrics &net = currentNetwork[i];

        QTableWidgetItem *ifaceItem = new QTableWidgetItem(net.interface);
        ifaceItem->setFlags(ifaceItem->flags() & ~Qt::ItemIsEditable);
        networkTable->setItem(i, 0, ifaceItem);

        QTableWidgetItem *rxItem = new QTableWidgetItem(
            formatBytesPerSecond(net.bytesReceived));
        rxItem->setFlags(rxItem->flags() & ~Qt::ItemIsEditable);
        rxItem->setTextAlignment(Qt::AlignRight);
        networkTable->setItem(i, 1, rxItem);

        QTableWidgetItem *txItem = new QTableWidgetItem(
            formatBytesPerSecond(net.bytesSent));
        txItem->setFlags(txItem->flags() & ~Qt::ItemIsEditable);
        txItem->setTextAlignment(Qt::AlignRight);
        networkTable->setItem(i, 2, txItem);

        QTableWidgetItem *rxPktItem = new QTableWidgetItem(
            QString("%1").arg(net.packetsReceived));
        rxPktItem->setFlags(rxPktItem->flags() & ~Qt::ItemIsEditable);
        rxPktItem->setTextAlignment(Qt::AlignRight);
        networkTable->setItem(i, 3, rxPktItem);

        QTableWidgetItem *txPktItem = new QTableWidgetItem(
            QString("%1").arg(net.packetsSent));
        txPktItem->setFlags(txPktItem->flags() & ~Qt::ItemIsEditable);
        txPktItem->setTextAlignment(Qt::AlignRight);
        networkTable->setItem(i, 4, txPktItem);

        QTableWidgetItem *errItem = new QTableWidgetItem(
            QString("%1/%2")
                .arg(net.errorsIn)
                .arg(net.errorsOut));
        errItem->setFlags(errItem->flags() & ~Qt::ItemIsEditable);
        errItem->setTextAlignment(Qt::AlignCenter);
        networkTable->setItem(i, 5, errItem);
    }

    // Uptime card
    uptimeLabel->setText(currentSystem.uptime);
    processCountLabel->setText(QString::number(currentSystem.processCount));
    threadCountLabel->setText(QString::number(currentSystem.threadCount));

    emit metricsRefreshed(currentCpu, currentMemory, currentSystem);
}

CpuMetrics MetricsWidget::readCpuMetrics() {
    CpuMetrics metrics;

    static QVector<qulonglong> prevSystem;
    static QVector<qulonglong> prevUser;
    static QVector<qulonglong> prevIdle;
    static QVector<qulonglong> prevIowait;
    static bool firstRead = true;

    QFile statFile("/proc/stat");
    if (!statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return metrics;
    }

    QTextStream statStream(&statFile);
    QString line;
    int lineIndex = 0;

    QVector<double> perCoreUsageList;

    while (statStream.readLineInto(&line)) {
        if (!line.startsWith("cpu")) break;

        QStringList fields = line.split(' ', Qt::SkipEmptyParts);
        if (fields.size() < 8) {
            ++lineIndex;
            continue;
        }

        qulonglong user = fields[1].toULongLong();
        qulonglong nice = fields[2].toULongLong();
        qulonglong system = fields[3].toULongLong();
        qulonglong idle = fields[4].toULongLong();
        qulonglong iowait = fields[5].toULongLong();
        qulonglong irq = fields[6].toULongLong();
        qulonglong softirq = fields[7].toULongLong();

        qulonglong totalTick = user + nice + system + idle + iowait + irq + softirq;

        if (lineIndex == 0) {
            if (!firstRead && prevSystem.size() > 0) {
                auto totalDiff = qMax(static_cast<long long>(totalTick - prevSystem[0]), 0LL);
                auto idleDiff = qMax(static_cast<long long>(idle + iowait - prevIdle[0]), 0LL);

                if (totalDiff > 0) {
                    double usage = (1.0 - static_cast<double>(idleDiff) / totalDiff) * 100.0;
                    metrics.totalUsagePercent = qMax(0.0, qMin(100.0, usage));
                }
            }
            while (prevSystem.size() < 1) {
                prevSystem.append(0);
                prevUser.append(0);
                prevIdle.append(0);
                prevIowait.append(0);
            }
            prevSystem[0] = totalTick;
            prevUser[0] = user + nice;
            prevIdle[0] = idle + iowait;
            prevIowait[0] = iowait;
        } else {
            int coreIndex = lineIndex - 1;
            metrics.coreCount = qMax(metrics.coreCount, coreIndex + 1);

            while (prevSystem.size() <= static_cast<size_t>(coreIndex)) {
                prevSystem.append(0);
                prevUser.append(0);
                prevIdle.append(0);
                prevIowait.append(0);
            }

            if (!firstRead && prevSystem[coreIndex] > 0) {
                auto totalDiff = static_cast<long long>(totalTick - prevSystem[coreIndex]);
                auto idleDiff = qMax(static_cast<long long>(idle + iowait - prevIdle[coreIndex]), 0LL);

                if (totalDiff > 0) {
                    double usage = (1.0 - static_cast<double>(idleDiff) / totalDiff) * 100.0;
                    perCoreUsageList.append(qMax(0.0, qMin(100.0, usage)));
                } else {
                    perCoreUsageList.append(0.0);
                }
            } else {
                perCoreUsageList.append(0.0);
            }

            prevSystem[coreIndex] = totalTick;
            prevUser[coreIndex] = user + nice;
            prevIdle[coreIndex] = idle + iowait;
            prevIowait[coreIndex] = iowait;
        }

        ++lineIndex;
    }
    statFile.close();

    firstRead = false;
    metrics.perCoreUsage = perCoreUsageList;

    QFile loadavgFile("/proc/loadavg");
    if (loadavgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream loadavgStream(&loadavgFile);
        QString loadavgLine = loadavgStream.readLine();
        loadavgFile.close();

        QStringList loadavgFields = loadavgLine.split(' ', Qt::SkipEmptyParts);
        if (loadavgFields.size() >= 3) {
            metrics.loadAvg1m = loadavgFields[0];
            metrics.loadAvg5m = loadavgFields[1];
            metrics.loadAvg15m = loadavgFields[2];
        }
    }

    return metrics;
}

MemoryMetrics MetricsWidget::readMemoryMetrics() {
    MemoryMetrics metrics;

    QFile meminfoFile("/proc/meminfo");
    if (!meminfoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return metrics;
    }

    QTextStream meminfoStream(&meminfoFile);
    QString line;
    while (meminfoStream.readLineInto(&line)) {
        if (line.startsWith("MemTotal:")) {
            QString valueStr = line.split(':').last().trimmed();
            bool ok = false;
            quint64 kb = valueStr.split(' ', Qt::SkipEmptyParts).first().toULongLong(&ok);
            if (ok) metrics.totalBytes = kb * 1024;
        } else if (line.startsWith("MemAvailable:")) {
            QString valueStr = line.split(':').last().trimmed();
            bool ok = false;
            quint64 kb = valueStr.split(' ', Qt::SkipEmptyParts).first().toULongLong(&ok);
            if (ok) metrics.availableBytes = kb * 1024;
        } else if (line.startsWith("SwapTotal:")) {
            QString valueStr = line.split(':').last().trimmed();
            bool ok = false;
            quint64 kb = valueStr.split(' ', Qt::SkipEmptyParts).first().toULongLong(&ok);
            if (ok) metrics.swapTotalBytes = kb * 1024;
        } else if (line.startsWith("SwapFree:")) {
            QString valueStr = line.split(':').last().trimmed();
            bool ok = false;
            quint64 kb = valueStr.split(' ', Qt::SkipEmptyParts).first().toULongLong(&ok);
            if (ok) metrics.swapUsedBytes = metrics.swapTotalBytes - (kb * 1024);
        } else if (line.startsWith("Buffers:")) {
            QString valueStr = line.split(':').last().trimmed();
            bool ok = false;
            quint64 kb = valueStr.split(' ', Qt::SkipEmptyParts).first().toULongLong(&ok);
            if (ok) metrics.buffersBytes = kb * 1024;
        } else if (line.startsWith("Cached:")) {
            QString valueStr = line.split(':').last().trimmed();
            bool ok = false;
            quint64 kb = valueStr.split(' ', Qt::SkipEmptyParts).first().toULongLong(&ok);
            if (ok) metrics.cachedBytes = kb * 1024;
        }
    }
    meminfoFile.close();

    if (metrics.availableBytes > 0 && metrics.totalBytes > metrics.availableBytes) {
        metrics.usedBytes = metrics.totalBytes - metrics.availableBytes;
    } else {
        quint64 freeKb = 0;
        QFile meminfoFile2("/proc/meminfo");
        if (meminfoFile2.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream meminfoStream2(&meminfoFile2);
            QString line2;
            while (meminfoStream2.readLineInto(&line2)) {
                if (line2.startsWith("MemFree:")) {
                    QString valueStr = line2.split(':').last().trimmed();
                    bool ok = false;
                    freeKb = valueStr.split(' ', Qt::SkipEmptyParts).first().toULongLong(&ok);
                    break;
                }
            }
            meminfoFile2.close();
        }
        if (metrics.totalBytes > 0 && freeKb > 0) {
            metrics.usedBytes = (metrics.totalBytes / 1024 - freeKb) * 1024;
        }
    }

    if (metrics.swapTotalBytes == 0) {
        metrics.swapUsedBytes = 0;
    }

    return metrics;
}

SystemMetrics MetricsWidget::readSystemMetrics() {
    SystemMetrics metrics;

    // Read /proc/uptime
    QFile uptimeFile("/proc/uptime");
    if (uptimeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream uptimeStream(&uptimeFile);
        QString uptimeLine = uptimeStream.readLine();
        uptimeFile.close();

        QStringList uptimeFields = uptimeLine.split(' ', Qt::SkipEmptyParts);
        if (uptimeFields.size() >= 1) {
            double uptimeSeconds = uptimeFields[0].toDouble();
            qint64 totalSeconds = static_cast<qint64>(uptimeSeconds);

            qint64 days = totalSeconds / 86400;
            qint64 hours = (totalSeconds % 86400) / 3600;
            qint64 minutes = (totalSeconds % 3600) / 60;

            if (days > 0) {
                metrics.uptime = QString("%1d %2h %3m")
                                     .arg(days)
                                     .arg(hours)
                                     .arg(minutes);
            } else if (hours > 0) {
                metrics.uptime = QString("%1h %2m")
                                     .arg(hours)
                                     .arg(minutes);
            } else {
                metrics.uptime = QString("%1m").arg(minutes);
            }
        }
    }

    // Count processes from /proc (directories with numeric names)
    QDir procDir("/proc");
    procDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfoList entries = procDir.entryInfoList();
    int procCount = 0;
    int threadCount = 0;
    for (const auto &entry : entries) {
        bool ok = false;
        entry.fileName().toInt(&ok);
        if (ok) {
            ++procCount;
            QFile statusFile(QString("/proc/%1/status").arg(entry.fileName()));
            if (statusFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream statusStream(&statusFile);
                QString statusLine;
                while (statusStream.readLineInto(&statusLine)) {
                    if (statusLine.startsWith("Threads:")) {
                        QString threadsStr = statusLine.mid(8).trimmed();
                        bool threadsOk = false;
                        int threads = threadsStr.toInt(&threadsOk);
                        if (threadsOk) {
                            threadCount += threads;
                        }
                        break;
                    }
                }
                statusFile.close();
            }
        }
    }

    metrics.processCount = procCount;
    metrics.threadCount = threadCount;

    return metrics;
}
