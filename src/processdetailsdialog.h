#ifndef PROCESSDETAILSDIALOG_H
#define PROCESSDETAILSDIALOG_H

#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QGridLayout>
#include <QScrollArea>
#include <QVBoxLayout>
#include "processeswidget.h"

class ProcessDetailsDialog : public QDialog {
    Q_OBJECT

    public:
    explicit ProcessDetailsDialog(QWidget *parent = nullptr);

    void setProcessInfo(const ProcessInfo &info);

    private:
    QGroupBox *createInfoGroup(const QString &title, const QList<QPair<QString, QString>> &items);
};

#endif // !PROCESSDETAILSDIALOG_H
