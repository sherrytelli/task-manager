#ifndef PROCESSESWIDGET_H
#define PROCESSESWIDGET_H

#include <QAbstractItemView>
#include <QDir>
#include <QFileInfoList>
#include <QHeaderView>
#include <QLineEdit>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

class ProcessWidget : public QWidget {
    Q_OBJECT

    public:
    explicit ProcessWidget(QWidget *parent = nullptr);

    private:
    QTableWidget *tableWidget;
    QDir *procDir;
    void updateProcessesList();
    QLineEdit *searchLineEdit;
    QString lastSearchText;
    void filterProcesses(const QString &filterText);
};

#endif // !PROCESSESWIDGET_H
