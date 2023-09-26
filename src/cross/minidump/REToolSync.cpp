#include "REToolSync.h"
#include <httplib.h>
#include <QDebug>
#include <QJsonDocument>

REToolSync::REToolSync(QObject* parent)
{
    mServer = new httplib::Server();
    mPingTimer = new QTimer(parent);
    QObject::connect(mPingTimer, &QTimer::timeout, mPingTimer, [this]()
    {
        if(mToken.isEmpty())
            return;

        httplib::Client client(mEndpoint.toUtf8().data());
        httplib::Params params =
        {
            { "token", mToken.toUtf8().constData() },
        };
        auto response = client.Get("/api/clients", params, httplib::Headers());
        if(!response || response->status != 200)
        {
            emit error(tr("Client no longer registered at: %1 (token: %2)").arg(mEndpoint, mToken));
            mToken.clear();
            disconnect();
        }
    });
}

REToolSync::~REToolSync()
{
    disconnect();

    delete mServer;
    mServer = nullptr;
}

bool REToolSync::connect(const QString & endpoint)
{
    if(mServer->is_running())
    {
        emit error(tr("Already connected"));
        return false;
    }

    // Ping the server
    mEndpoint = endpoint;
    httplib::Client client(mEndpoint.toUtf8().data());
    {
        auto response = client.Get("/api/ping");
        if(!response || response->status != 200)
        {
            emit error(tr("Pinging failed: %1/api/ping").arg(mEndpoint));
            return false;
        }
        emit info(tr("Endpoint working: %1").arg(mEndpoint));
    }

    // Start our server
    if(isRunning())
    {
        emit error(tr("Thread already running (bad)"));
        terminate();
    }
    start();

    // Wait for our server to initialize
    for(int i = 0; i < 10; i++)
    {
        if(mServer->is_running())
            break;
        QThread::msleep(100);
    }
    if(!mServer->is_running())
    {
        emit error(tr("Server not running after starting thread"));
        return false;
    }

    // Register ourselves
    auto json = QString("{\"endpoint\":\"http://%1:%2\"}").arg(mHost).arg(mPort);
    auto response = client.Post("/api/clients", json.toUtf8().constData(), "application/json");
    if(!response)
    {
        emit error(tr("Client registration failed (no response, error: %1)").arg(httplib::to_string(response.error()).c_str()));
        return false;
    }
    else if(response->status != 200)
    {
        emit error(tr("Client registration failed (status: %1, body: %2)").arg(response->status).arg(response->body.c_str()));
        return false;
    }
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(response->body));
    if(!doc.isObject())
    {
        emit error(tr("Client registration failed (JSON error: %1)").arg(response->body.c_str()));
        return false;
    }
    mToken = doc["token"].toString();
    if(mToken.isEmpty())
    {
        emit error(tr("Client registration failed (no token in body: %1)").arg(response->body.c_str()));
        return false;
    }
    emit info(tr("Client registration success (token: %1)").arg(mToken));

    // Periodically check if the server is still alive
    mPingTimer->start(3000);

    return true;
}

void REToolSync::disconnect()
{
    if(!mServer->is_running())
    {
        return;
    }

    // Delete our client from the server
    if(!mToken.isEmpty())
    {
        httplib::Client client(mEndpoint.toUtf8().constData());
        httplib::Params params =
        {
            { "token", mToken.toUtf8().constData() },
        };
        auto response = client.Delete("/api/clients", params);
        if(!response)
        {
            emit error(tr("Failed to unregister (no response, error: %1)").arg(httplib::to_string(response.error()).c_str()));
        }
        else if(response->status != 200)
        {
            emit error(tr("Failed to unregister (token: %1, body: %2)").arg(mToken).arg(response->body.c_str()));
        }
        else
        {
            emit info(tr("Unregistered client (token: %1)").arg(mToken));
            mToken.clear();
        }
    }

    // Stop the timer
    mPingTimer->stop();

    // Wait for the server to stop
    mServer->stop();
    while(mServer->is_running())
    {
        QThread::msleep(100);
    }

    // Wait for the thread to finish
    if(isRunning())
    {
        wait();
    }
}

void REToolSync::run()
{
    using namespace httplib;
    mServer->Get("/ping", [](const Request & request, Response & response)
    {
        response.set_content("pong", "text/plain");
    });
    mServer->Post("/goto", [this](const Request & request, Response & response)
    {
        auto addressText = QString::fromStdString(request.get_param_value("address"));
        bool ok = false;
        auto address = addressText.toULongLong(&ok, 16);
        if(!ok)
        {
            response.set_content("Invalid address", "text/plain");
            response.status = 400;
            return;
        }
        emit gotoAddress(address);
    });
    mPort = mServer->bind_to_any_port(mHost);
    if(mPort == -1)
    {
        emit error(tr("Failed to bind to %1").arg(mHost));
        return;
    }
    emit info(tr("Listening on http://%1:%2").arg(mHost).arg(mPort));
    if(!mServer->listen_after_bind())
    {
        emit error(tr("Error listening on %1:%2").arg(mHost).arg(mPort));
        return;
    }
    emit info(tr("Server stopped"));
}
