#pragma once

#include <QObject>
#include "TableRpcData.h"

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)

class TableServer : public QObject
{
    Q_OBJECT

public:
    TableServer(QObject* parent = nullptr);

private:
    bool table(const TableRequest & request, TableResponse & response);

private:
    QWebSocketServer* mServer = nullptr;
    QList<QWebSocket*> mClients;
};
