#pragma once

#include <QDialog>
#include "dbg_types.h"

namespace Ui
{
    class PageMemoryRights;
}

class PageMemoryRights : public QDialog
{
    Q_OBJECT

public:
    explicit PageMemoryRights(QWidget* parent = 0);
    void RunAddrSize(duint, duint, QString);
    ~PageMemoryRights();

private slots:
    void on_btnSelectall_clicked();
    void on_btnDeselectall_clicked();
    void on_btnSetrights_clicked();

signals:
    void refreshMemoryMap();

private:
    Ui::PageMemoryRights* ui;
    duint addr;
    duint size;
    QString pagetype;
};
