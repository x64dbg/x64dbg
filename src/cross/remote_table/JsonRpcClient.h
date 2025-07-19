#pragma once

#include <QObject>
#include <QWebSocket>

#include <unordered_map>
#include <functional>
#include <string>
#include "json.hpp"

// https://www.jsonrpc.org/specification
class JsonRpcClient : public QObject
{
    Q_OBJECT

public:
    using Callback = std::function<void(const nlohmann::json &)>;

    JsonRpcClient(QObject* parent = nullptr);
    void open(const QUrl & url);
    bool isConnected();
    void call(const QString & method, nlohmann::json params, Callback finished);

    template<class Response, class Request>
    void call(const Request & params, const std::function<void(Response)> & finished)
    {
        if(strcmp(Request::method, Response::method) != 0)
            throw std::runtime_error("Mismatching method name");
        nlohmann::json jparams;
        nlohmann::to_json(jparams, params);
        call(Request::method, std::move(jparams), [finished](const nlohmann::json & response)
        {
            Response r;
            nlohmann::from_json(response, r);
            finished(std::move(r));
        });
    }

signals:
    void connected();
    void disconnected();

private slots:
    void textMessageReceivedSlot(const QString & message);

private:
    uint64_t mRequestId = 0;
    QWebSocket mSocket;
    std::unordered_map<int, Callback> mCalls;
};
