#include "MainWindowCloseThread.h"
#include "Imports.h"
#include "Bridge.h"

void MainWindowCloseThread::run()
{
    DbgExit();
    Bridge::getBridge()->setDbgStopped();
    emit canClose();
}
