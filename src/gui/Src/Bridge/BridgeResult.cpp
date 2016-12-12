#include "BridgeResult.h"
#include "Bridge.h"

BridgeResult::BridgeResult()
{
    Bridge::getBridge()->mBridgeMutex->lock();
    ResetEvent(Bridge::getBridge()->hResultEvent);
}

BridgeResult::~BridgeResult()
{
    Bridge::getBridge()->mBridgeMutex->unlock();
}

dsint BridgeResult::Wait()
{
    WaitForSingleObject(Bridge::getBridge()->hResultEvent, INFINITE);
    return Bridge::getBridge()->bridgeResult;
}
