#include "JsonRpcClient.h"

#include <QDebug>

JsonRpcClient::JsonRpcClient(QObject* parent)
    : QObject(parent)
{
    connect(&mSocket, &QWebSocket::connected, this, &JsonRpcClient::connected);
    connect(&mSocket, &QWebSocket::disconnected, this, &JsonRpcClient::disconnected);
    connect(&mSocket, &QWebSocket::textMessageReceived, this, &JsonRpcClient::textMessageReceivedSlot);
}

void JsonRpcClient::open(const QUrl & url)
{
    mSocket.open(url);
}

bool JsonRpcClient::isConnected()
{
    return mSocket.state() == QAbstractSocket::ConnectedState;
}

void JsonRpcClient::call(const QString & method, nlohmann::json params, Callback finished)
{
    auto id = mRequestId++;
    nlohmann::json request;
    request["jsonrpc"] = "2.0";
    request["method"] = method.toStdString();
    request["params"] = std::move(params);
    request["id"] = id;
    // TODO: timeout
    mCalls.emplace(id, std::move(finished));
    mSocket.sendTextMessage(QString::fromStdString(request.dump()));
}

void JsonRpcClient::textMessageReceivedSlot(const QString & message)
{
    //qDebug() << "[client] textMessageReceivedSlot" << message;
    // This is fine 🔥
    auto response = nlohmann::json::parse(message.toStdString());
    // TODO: rewrite to not use exceptions for WASM
    try
    {
        auto magic = response.at("jsonrpc").get<std::string>();
        if(magic != "2.0")
        {
            qDebug() << "[client] Invalid magic:" << QString::fromStdString(magic);
            return;
        }
        auto result = response.at("result");
        auto id = response.at("id").get<uint64_t>();
        auto itr = mCalls.find(id);
        if(itr == mCalls.end())
        {
            qDebug() << "[client] Unknown response id:" << id;
            return;
        }
        itr->second(result);
    }
    catch(std::exception & x)
    {
        qDebug() << "[client] JSON exception:" << x.what();
    }
}
