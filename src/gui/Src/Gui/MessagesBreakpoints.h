#ifndef MESSAGESBREAKPOINTS_H
#define MESSAGESBREAKPOINTS_H

#include <QDialog>
#include <QLayout>
#include <QMap>
#include <QVector>
#include "Imports.h"

namespace Ui
{
    class MessagesBreakpoints;
}

class MessagesBreakpoints : public QDialog
{
    Q_OBJECT

public:
    struct MsgBreakpointData
    {
        QString procVA;
        QString wndHandle;
    };
    explicit MessagesBreakpoints(MsgBreakpointData pbpData, QWidget* parent = 0);
    ~MessagesBreakpoints();
    MsgBreakpointData bpData;

private slots:
    void on_btnOk_clicked();

private:
    Ui::MessagesBreakpoints* ui;
    QMap<duint, QString> messages;
    QVector<QString> filterMessages;
};

#endif // MESSAGESBREAKPOINTS_H
