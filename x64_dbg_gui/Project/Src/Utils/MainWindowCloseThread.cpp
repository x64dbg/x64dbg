#include "MainWindowCloseThread.h"
#include "NewTypes.h"

void MainWindowCloseThread::run()
{
    DbgExit();
    emit canClose();
}
