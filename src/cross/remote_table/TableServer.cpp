#include "TableServer.h"
#include "TableRpcData.h"

#include <QWebSocketServer>
#include <QWebSocket>
#include <QDebug>
#include <QThread>
#include <QApplication>

#include <chrono>

TableServer::TableServer(QObject* parent)
    : QObject(parent)
    , mServer(new QWebSocketServer("TableServer", QWebSocketServer::NonSecureMode))
{
    if(mServer->listen(QHostAddress::LocalHost, 42069))
    {
        qDebug() << "[server] Listening on ws://127.0.0.1:42069";
        connect(mServer, &QWebSocketServer::newConnection, this, [this]()
        {
            auto socket = mServer->nextPendingConnection();
            connect(socket, &QWebSocket::textMessageReceived, this, [this](QString message)
            {
                //qDebug() << "[server] textMessageReceived:" << message;

                auto client = qobject_cast<QWebSocket*>(sender());

                try
                {
                    auto request = nlohmann::json::parse(message.toStdString());
                    auto magic = request["jsonrpc"].get<std::string>();
                    if(magic != "2.0")
                    {
                        qDebug() << "[server] Invalid magic:" << QString::fromStdString(magic);
                        return;
                    }
                    auto method = request["method"].get<std::string>();
                    auto id = request["id"].get<uint64_t>();
                    auto params = request["params"];
                    nlohmann::json response;
                    response["jsonrpc"] = "2.0";
                    response["id"] = id;
                    nlohmann::json & result = response["result"];
                    if(method == "table")
                    {
                        TableRequest request;
                        nlohmann::from_json(params, request);

                        TableResponse response{};

                        if(!table(request, response))
                        {
                            // TODO: write error response
                            qDebug() << "[server] bad fail";
                        }

                        nlohmann::to_json(result, response);
                    }
                    else
                    {
                        qDebug() << "[server] unknown method:" << method.c_str();
                        return;
                    }

                    client->sendTextMessage(QString::fromStdString(response.dump()));
                }
                catch(std::exception & x)
                {
                    qDebug() << "[server] JSON error:" << x.what();
                }

            });

            mClients << socket;
        });
    }
    else
    {
        qDebug() << "[server] Error listening";
    }
}

bool TableServer::table(const TableRequest & request, TableResponse & response)
{
    response.rows.resize(request.lines);
    for(uint64_t line = 0; line < request.lines; line++)
    {
        auto & row = response.rows[line];
        row.resize(2);
        row[0] = "address: " + std::to_string(line + request.offset);
        row[1] = "data: " + std::to_string(line + request.offset);
    }

    // Sleep without hanging event loop
    auto start = std::chrono::system_clock::now();
    while(true)
    {
        auto now = std::chrono::system_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if(elapsedMs >= 500)
            break;
        QApplication::processEvents();
    }

    return true;
}
