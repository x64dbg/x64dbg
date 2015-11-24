#include "_scriptapi_misc.h"
#include "value.h"

SCRIPT_EXPORT bool Script::Misc::ParseExpression(const char* expression, duint* value)
{
    return valfromstring(expression, value);
}

SCRIPT_EXPORT duint Script::Misc::RemoteGetProcAddress(const char* module, const char* api)
{
    duint value;
    if(!ParseExpression(StringUtils::sprintf("%s:%s", module, api).c_str(), &value))
        return 0;
    return value;
}

SCRIPT_EXPORT duint Script::Misc::ResolveLabel(const char* label)
{
    duint value;
    if(!ParseExpression(label, &value))
        return 0;
    return value;
}

SCRIPT_EXPORT void* Script::Misc::Alloc(duint size)
{
    return BridgeAlloc(size);
}

SCRIPT_EXPORT void Script::Misc::Free(void* ptr)
{
    return BridgeFree(ptr);
}