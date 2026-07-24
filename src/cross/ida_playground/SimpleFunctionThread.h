#pragma once

#include <QThread>
#include <functional>

class FunctionExecutor : public QObject
{
    Q_OBJECT

public:
    explicit FunctionExecutor(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

public slots:
    void executeFunction(std::function<void()> func);
};

class SimpleFunctionThread : public QObject
{
    Q_OBJECT

public:
    explicit SimpleFunctionThread(QObject* parent = nullptr);
    ~SimpleFunctionThread() override;
    void addFunction(std::function<void()> func);

private:
    QThread* m_thread = nullptr;
    FunctionExecutor* m_executor = nullptr;
};
