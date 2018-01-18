#include "BridgeResult.h"
#include "Bridge.h"

BridgeResult::BridgeResult()
{
    EnterCriticalSection(&Bridge::getBridge()->csBridge);
    ResetEvent(Bridge::getBridge()->hResultEvent);
}

BridgeResult::~BridgeResult()
{
    LeaveCriticalSection(&Bridge::getBridge()->csBridge);
}

dsint BridgeResult::Wait()
{
    //Don't freeze when waiting on the main thread (https://github.com/x64dbg/x64dbg/issues/1716)
    if(GetCurrentThreadId() == Bridge::getBridge()->dwMainThreadId)
        while(WaitForSingleObject(Bridge::getBridge()->hResultEvent, 10) == WAIT_TIMEOUT)
            QCoreApplication::processEvents();
    else
        WaitForSingleObject(Bridge::getBridge()->hResultEvent, INFINITE);
    return Bridge::getBridge()->bridgeResult;
}
