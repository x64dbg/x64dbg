#include "BridgeResult.h"
#include "Bridge.h"
#include <QCoreApplication>

BridgeResult::BridgeResult(Type type)
    : mType(type)
{
    Bridge* bridge = Bridge::getBridge();
    EnterCriticalSection(&bridge->csBridge);
#ifdef DEBUG
    OutputDebugStringA(QString().sprintf("[x64dbg] [%u] BridgeResult(%d)\n", GetCurrentThreadId(), type).toUtf8().constData());
#endif //DEBUG
    ResetEvent(bridge->resultEvents[type]);
}

BridgeResult::~BridgeResult()
{
#ifdef DEBUG
    OutputDebugStringA(QString().sprintf("[x64dbg] [%u] ~BridgeResult(%d)\n", GetCurrentThreadId(), mType).toUtf8().constData());
#endif //DEBUG
    LeaveCriticalSection(&Bridge::getBridge()->csBridge);
}

dsint BridgeResult::Wait()
{
#ifdef DEBUG
    OutputDebugStringA(QString().sprintf("[x64dbg] [%u] BridgeResult::Wait(%d)\n", GetCurrentThreadId(), mType).toUtf8().constData());
#endif //DEBUG
    Bridge* bridge = Bridge::getBridge();
    HANDLE hResultEvent = bridge->resultEvents[mType];
    //Don't freeze when waiting on the main thread (https://github.com/x64dbg/x64dbg/issues/1716)
    if(GetCurrentThreadId() == bridge->dwMainThreadId)
        while(WaitForSingleObject(hResultEvent, 10) == WAIT_TIMEOUT)
            QCoreApplication::processEvents();
    else
        WaitForSingleObject(hResultEvent, INFINITE);
#ifdef DEBUG
    OutputDebugStringA(QString().sprintf("[x64dbg] [%u] BridgeResult::~Wait(%d)\n", GetCurrentThreadId(), mType).toUtf8().constData());
#endif //DEBUG
    return bridge->bridgeResults[mType];
}
