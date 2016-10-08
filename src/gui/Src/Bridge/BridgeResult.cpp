#include "BridgeResult.h"
#include "Bridge.h"

BridgeResult::BridgeResult()
{
    Bridge::getBridge()->mBridgeMutex->lock();
    Bridge::getBridge()->hasBridgeResult = false;
}

BridgeResult::~BridgeResult()
{
    Bridge::getBridge()->mBridgeMutex->unlock();
}

dsint BridgeResult::Wait()
{
    while(!Bridge::getBridge()->hasBridgeResult) //wait for thread completion
        Sleep(1);
    return Bridge::getBridge()->bridgeResult;
}
