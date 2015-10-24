#include "_global.h"
#include "TitanEngine\TitanEngine.h"
#include "IEngine.h"

IEngine temp_Engine;
IEngine *Engine = &temp_Engine;

bool IEngine::Initialize()
{
    if (!EngineCheckStructAlignment(UE_STRUCT_TITAN_ENGINE_CONTEXT, sizeof(TITAN_ENGINE_CONTEXT_t)))
        return false;

    if (sizeof(TITAN_ENGINE_CONTEXT_t) != sizeof(REGISTERCONTEXT))
        return false;

    return true;
}