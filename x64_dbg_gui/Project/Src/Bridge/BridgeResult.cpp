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

int_t BridgeResult::Wait()
{
    while(!Bridge::getBridge()->hasBridgeResult) //wait for thread completion
        Sleep(100);
    return Bridge::getBridge()->bridgeResult;
}
