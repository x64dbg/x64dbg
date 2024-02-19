#include "BridgeResult.h"
#include "Bridge.h"
#include <QCoreApplication>

BridgeResult::BridgeResult(Type type)
    : mType(type)
{
    Bridge* bridge = Bridge::getBridge();
    EnterCriticalSection(&bridge->mCsBridge);
#ifdef DEBUG
    OutputDebugStringA(QString().sprintf("[x64dbg] [%u] BridgeResult(%d)\n", GetCurrentThreadId(), type).toUtf8().constData());
#endif //DEBUG
    ResetEvent(bridge->mResultEvents[type]);
}

BridgeResult::~BridgeResult()
{
#ifdef DEBUG
    OutputDebugStringA(QString().sprintf("[x64dbg] [%u] ~BridgeResult(%d)\n", GetCurrentThreadId(), mType).toUtf8().constData());
#endif //DEBUG
    LeaveCriticalSection(&Bridge::getBridge()->mCsBridge);
}

dsint BridgeResult::Wait()
{
#ifdef DEBUG
    OutputDebugStringA(QString().sprintf("[x64dbg] [%u] BridgeResult::Wait(%d)\n", GetCurrentThreadId(), mType).toUtf8().constData());
#endif //DEBUG
    Bridge* bridge = Bridge::getBridge();
    HANDLE hResultEvent = bridge->mResultEvents[mType];
    //Don't freeze when waiting on the main thread (https://github.com/x64dbg/x64dbg/issues/1716)
    if(GetCurrentThreadId() == bridge->mMainThreadId)
        while(WaitForSingleObject(hResultEvent, 10) == WAIT_TIMEOUT)
            QCoreApplication::processEvents();
    else
        WaitForSingleObject(hResultEvent, INFINITE);
#ifdef DEBUG
    OutputDebugStringA(QString().sprintf("[x64dbg] [%u] BridgeResult::~Wait(%d)\n", GetCurrentThreadId(), mType).toUtf8().constData());
#endif //DEBUG
    return bridge->mBridgeResults[mType];
}
