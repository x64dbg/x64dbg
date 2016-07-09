#include "exprfunc.h"
#include "symbolinfo.h"

namespace Exprfunc
{
    duint srcline(duint addr)
    {
        int line = 0;
        DWORD displacement = 0;
        if(!SymGetSourceLine(addr, nullptr, &line, &displacement) || displacement)
            return 0;
        return line;
    }
}