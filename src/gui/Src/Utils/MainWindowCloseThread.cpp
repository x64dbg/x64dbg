#include "MainWindowCloseThread.h"
#include "Imports.h"
#include "Bridge.h"

MainWindowCloseThread::MainWindowCloseThread(QObject* parent)
    : QThread(parent)
{
}

void MainWindowCloseThread::run()
{
    DbgExit();
    Bridge::getBridge()->setDbgStopped();
    emit canClose();
}
