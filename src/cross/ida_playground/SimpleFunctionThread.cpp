#include "SimpleFunctionThread.h"

void FunctionExecutor::executeFunction(std::function<void()> func)
{
    if(func)
    {
        func();
    }
}

SimpleFunctionThread::SimpleFunctionThread(QObject* parent) : QObject(parent)
{
    m_thread = new QThread(this);
    m_executor = new FunctionExecutor();
    m_executor->moveToThread(m_thread);
    m_thread->start();
}

SimpleFunctionThread::~SimpleFunctionThread()
{
    m_thread->quit();
    m_thread->wait();
    delete m_executor;
    delete m_thread;
}

void SimpleFunctionThread::addFunction(std::function<void()> func)
{
    QMetaObject::invokeMethod(m_executor, "executeFunction",
                              Qt::QueuedConnection,
                              Q_ARG(std::function<void()>, std::move(func)));
}

