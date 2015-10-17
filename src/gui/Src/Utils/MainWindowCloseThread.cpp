#include "MainWindowCloseThread.h"
#include "NewTypes.h"
#include "Bridge.h"

void MainWindowCloseThread::run()
{
    DbgExit();
    Bridge::getBridge()->setDbgStopped();
    emit canClose();
}
