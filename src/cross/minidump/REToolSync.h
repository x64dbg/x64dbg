#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <atomic>
#include <memory>
#include "Types.h"

namespace httplib
{
    class Server;
    class Client;
}

class REToolSync : public QThread
{
    Q_OBJECT
public:
    explicit REToolSync(QObject* parent = nullptr);
    REToolSync(const REToolSync &) = delete;
    REToolSync(REToolSync &&) = delete;
    ~REToolSync();

    bool connect(const QString & endpoint);
    void disconnect();

signals:
    void error(QString message);
    void info(QString message);
    void gotoAddress(duint address);

protected:
    void run() override;

private:
    QString mEndpoint;
    httplib::Server* mServer = nullptr;
    const char* mHost = "127.0.0.1";
    int mPort = -1;
    QString mToken;
    QTimer* mPingTimer = nullptr;
};
