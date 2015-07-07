#include "_scriptapi_misc.h"
#include "value.h"

SCRIPT_EXPORT bool Script::Misc::ParseExpression(const char* expression, duint* value)
{
    return valfromstring(expression, value);
}