#include "exprfunc.h"
#include "symbolinfo.h"
#include "module.h"

namespace Exprfunc
{
    duint srcline(duint addr)
    {
        int line = 0;
        if(!SymGetSourceLine(addr, nullptr, &line, nullptr))
            return 0;
        return line;
    }

    duint srcdisp(duint addr)
    {
        DWORD disp;
        if(!SymGetSourceLine(addr, nullptr, nullptr, &disp))
            return 0;
        return disp;
    }

    duint modparty(duint addr)
    {
        return ModGetParty(addr);
    }
}
