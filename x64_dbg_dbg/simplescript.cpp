#include "simplescript.h"
#include "command.h"
#include "console.h"
#include "instruction.h"
#include "x64_dbg.h"

CMDRESULT cbScriptAddLine(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    GuiScriptAddLine(argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbScriptClear(int argc, char* argv[])
{
    GuiScriptClear();
    return STATUS_CONTINUE;
}
