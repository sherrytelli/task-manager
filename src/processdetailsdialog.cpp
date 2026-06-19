#include "processdetailsdialog.h"

#include <QDialogButtonBox>
#include <QScrollBar>

static QString formatBytes(qint64 bytes) {
    static const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        ++unitIndex;
    }
    return QString("%1 %2").arg(QString::number(size, 'f', 1), units[unitIndex]);
}

ProcessDetailsDialog::ProcessDetailsDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Process Details");
    setMinimumSize(500, 400);
    setStyleSheet(
        "QDialog {"
        "  background-color: #1a1a2e;"
        "}"
        "QGroupBox {"
        "  background-color: #16213e;"
        "  border: 1px solid #0f3460;"
        "  border-radius: 6px;"
        "  margin-top: 8px;"
        "  padding-top: 12px;"
        "  font-weight: bold;"
        "  font-size: 13px;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 12px;"
        "  padding: 0 4px;"
        "  color: #e0e0e0;"
        "}"
        "QLabel {"
        "  color: #e0e0e0;"
        "  font-size: 12px;"
        "}"
        "QScrollArea {"
        "  border: none;"
        "  background-color: transparent;"
        "}"
        "QScrollBar:vertical {"
        "  background-color: #1a2744;"
        "  width: 10px;"
        "  border: none;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background-color: #0f3460;"
        "  border-radius: 5px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background-color: #1a4a7a;"
        "}"
        "QScrollBar::add-line:vertical,"
        "QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
        "QScrollBar::add-page:vertical,"
        "QScrollBar::sub-page:vertical {"
        "  background: none;"
        "}"
        "QDialogButtonBox {"
        "  spacing: 8px;"
        "}"
        "QDialogButtonBox::button {"
        "  min-width: 80px;"
        "  padding: 6px 16px;"
        "  background-color: #16213e;"
        "  border: 1px solid #0f3460;"
        "  border-radius: 4px;"
        "  color: #e0e0e0;"
        "}"
        "QDialogButtonBox::button:hover {"
        "  background-color: #0f3460;"
        "}"
        "QDialogButtonBox::button:pressed {"
        "  background-color: #4a90d9;"
        "}");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *scrollContent = new QWidget(scrollArea);
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(8, 8, 8, 8);
    scrollLayout->setSpacing(8);
    scrollArea->setWidget(scrollContent);

    mainLayout->addWidget(scrollArea);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void ProcessDetailsDialog::setProcessInfo(const ProcessInfo &info) {
    QList<QPair<QString, QString>> basicInfo;
    basicInfo.append({"PID", QString::number(info.pid)});
    basicInfo.append({"Parent PID", QString::number(info.parentPid)});
    basicInfo.append({"Name", info.name});
    basicInfo.append({"User", info.user});
    basicInfo.append({"State", info.state});
    basicInfo.append({"Command Line", info.commandLine});

    QList<QPair<QString, QString>> resourceInfo;
    resourceInfo.append({"CPU%", QString("%1%").arg(QString::number(info.cpuPercent, 'f', 1))});
    resourceInfo.append({"Memory%", QString("%1%").arg(QString::number(info.memoryPercent, 'f', 1))});
    resourceInfo.append({"Memory (RSS)", formatBytes(info.rssBytes)});
    resourceInfo.append({"Memory (PSS)", formatBytes(info.pssKb * 1024)});
    resourceInfo.append({"Threads", QString::number(info.threadCount)});
    resourceInfo.append({"Start Time", info.startTime});

    QGroupBox *basicGroup = createInfoGroup("Basic Information", basicInfo);
    QGroupBox *resourceGroup = createInfoGroup("Resource Usage", resourceInfo);

    QScrollArea *scrollArea = qobject_cast<QScrollArea*>(this->layout()->itemAt(0)->widget());
    if (scrollArea) {
        QWidget *scrollContent = scrollArea->widget();
        QVBoxLayout *scrollLayout = qobject_cast<QVBoxLayout*>(scrollContent->layout());
        if (scrollLayout) {
            scrollLayout->addWidget(basicGroup);
            scrollLayout->addWidget(resourceGroup);
        }
    }
}

QGroupBox *ProcessDetailsDialog::createInfoGroup(const QString &title,
                                                  const QList<QPair<QString, QString>> &items) {
    QGroupBox *group = new QGroupBox(title);
    QGridLayout *grid = new QGridLayout(group);
    grid->setColumnStretch(1, 1);

    int row = 0;
    for (const auto &item : items) {
        QLabel *label = new QLabel(item.first, group);
        label->setStyleSheet("font-weight: bold;");
        QLabel *value = new QLabel(item.second, group);
        value->setWordWrap(true);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);

        grid->addWidget(label, row, 0);
        grid->addWidget(value, row, 1);
        ++row;
    }

    return group;
}
