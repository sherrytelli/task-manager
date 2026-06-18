#ifndef METRICSWIDGET_H
#define METRICSWIDGET_H

#include <QDir>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QScrollArea>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

struct CpuMetrics {
    double totalUsagePercent = 0.0;
    int coreCount = 0;
    QVector<double> perCoreUsage;
    QString loadAvg1m = "N/A";
    QString loadAvg5m = "N/A";
    QString loadAvg15m = "N/A";
};

struct MemoryMetrics {
    quint64 totalBytes = 0;
    quint64 usedBytes = 0;
    quint64 availableBytes = 0;
    quint64 swapTotalBytes = 0;
    quint64 swapUsedBytes = 0;
    quint64 buffersBytes = 0;
    quint64 cachedBytes = 0;
};

struct NetworkMetrics {
    QString interface;
    quint64 bytesReceived = 0;
    quint64 bytesSent = 0;
    quint64 packetsReceived = 0;
    quint64 packetsSent = 0;
    quint64 errorsIn = 0;
    quint64 errorsOut = 0;
};

struct SystemMetrics {
    QString uptime = "N/A";
    int processCount = 0;
    int threadCount = 0;
};

class MetricsWidget : public QWidget {
    Q_OBJECT

    public:
    explicit MetricsWidget(QWidget *parent = nullptr);
    void setRefreshInterval(int ms);

    signals:
    void metricsRefreshed(const CpuMetrics &cpu, const MemoryMetrics &memory,
                          const SystemMetrics &system);

    private:
    QTimer *refreshTimer;
    int refreshIntervalMs = 2000;

    CpuMetrics currentCpu;
    MemoryMetrics currentMemory;
    SystemMetrics currentSystem;
    QVector<NetworkMetrics> currentNetwork;
    QVector<NetworkMetrics> previousNetwork;

    QLabel *cpuUsageLabel;
    QLabel *cpuLoadAvgLabel;
    QTableWidget *cpuCoreTable;
    QLabel *memTotalLabel;
    QLabel *memUsedLabel;
    QLabel *memPercentLabel;
    QLabel *memSwapLabel;
    QLabel *memBuffersLabel;
    QProgressBar *memProgressBar;
    QTableWidget *networkTable;
    QLabel *uptimeLabel;
    QLabel *processCountLabel;
    QLabel *threadCountLabel;
    QScrollArea *cpuScrollArea;
    QScrollArea *networkScrollArea;

    void setupLayout();
    QWidget *createCard(const QString &title);
    void addCardRow(QWidget *card, const QString &label, const QString &value);
    void addCardRow(QWidget *card, const QString &label, double value, const QString &unit);
    void addCardRow(QWidget *card, const QString &label, quint64 value, const QString &unit);
    void updateCards();

    CpuMetrics readCpuMetrics();
    MemoryMetrics readMemoryMetrics();
    NetworkMetrics readNetworkMetrics();
    SystemMetrics readSystemMetrics();
};

#endif // METRICSWIDGET_H
