#ifndef PROCESSESWIDGET_H
#define PROCESSESWIDGET_H

#include <QAbstractItemView>
#include <QDateTime>
#include <QDir>
#include <QHeaderView>
#include <QLineEdit>
#include <QMap>
#include <QMenu>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

struct ProcessInfo {
    qint64 pid = 0;
    QString name;
    QString user;
    double cpuPercent = 0.0;
    double memoryPercent = 0.0;
    QString state;
    QString startTime;
    int threadCount = 0;
    QString commandLine;
    qint64 parentPid = 0;
    qint64 rssBytes = 0;
    qint64 sharedPagesKb = 0;
    qint64 unsharedRssBytes = 0;
    qulonglong cumulativeCpuTicks = 0;
};

class ProcessWidget : public QWidget {
    Q_OBJECT

    public:
    explicit ProcessWidget(QWidget *parent = nullptr);

    void setRefreshInterval(int ms);

    signals:
    void refreshComplete(int processCount, quint64 totalMemory, quint64 usedMemory);

    private:
    QTableWidget *tableWidget;
    QDir *procDir;
    QLineEdit *searchLineEdit;
    QString lastSearchText;
    QTimer *refreshTimer;
    QVector<ProcessInfo> processCache;
    QMap<qint64, qulonglong> previousCpuTimes;
    qint64 totalSystemCpuTime = 0;
    quint64 totalMemoryBytes = 0;
    int refreshIntervalMs = 2000;

  enum Column {
         COL_PID = 0,
         COL_USER,
         COL_CPU_PERCENT,
         COL_MEMORY,
         COL_COMMAND_LINE,
         COLUMN_COUNT
     };

     static constexpr int kColumnWidths[COLUMN_COUNT] = {
         70,   // PID
         70,   // USER
         70,   // CPU%
         90,   // MEMORY
        0     // COMMAND LINE (stretch)
    };

    void updateProcessesList();
    void filterProcesses(const QString &filterText);
    void setupTable();
    void setupContextMenu();
    void killSelectedProcess(bool force);
    void showProcessDetails(const ProcessInfo &info);

    ProcessInfo readProcessInfo(int pid);
    quint64 readTotalMemory();
    quint64 readUsedMemory();
    qint64 readTotalCpuTime();
     QString formatUptime(qint64 ticks, long ticksPerSecond) const;
    QString getProcessStateSymbol(char stateChar) const;
    int findRowByPid(qint64 pid) const;
};

#endif // !PROCESSESWIDGET_H
