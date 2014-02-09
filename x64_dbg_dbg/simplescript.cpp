#include "simplescript.h"
#include "value.h"
#include "console.h"

bool scriptload(const char* filename)
{
    dprintf("scriptload(%s)\n", filename);
    return true;
}

void scriptunload()
{
    dputs("scriptunload");
}

void scriptrun(int destline)
{
    dprintf("scriptrun(%d)\n", destline);
}

void scriptstep()
{
    dputs("scriptstep");
}

bool scriptbptoggle(int line)
{
    dprintf("scriptbptoggle(%d)\n", line);
    return true;
}

bool scriptbpget(int line)
{
    if(line==5)
        return true;
    return false;
}

bool scriptcmdexec(const char* command)
{
    dprintf("scriptcmdexec(%s)\n", command);
    return true;
}

void scriptabort()
{
    dputs("scriptabort");
}

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

CMDRESULT cbScriptSetIp(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint line=0;
    if(!valfromstring(argv[1], &line, 0, 0, false, 0))
        return STATUS_ERROR;
    GuiScriptSetIp((int)line);
    return STATUS_CONTINUE;
}

CMDRESULT cbScriptError(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint line=0;
    if(argc>=3)
        if(!valfromstring(argv[2], &line, 0, 0, false, 0))
            return STATUS_ERROR;
    GuiScriptError((int)line, argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbScriptSetTitle(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    GuiScriptSetTitle(argv[1]);
    return STATUS_CONTINUE;
}

CMDRESULT cbScriptSetInfoLine(int argc, char* argv[])
{
    if(argc<2)
    {
        dputs("not enough arguments!");
        return STATUS_ERROR;
    }
    duint line=0;
    if(!valfromstring(argv[1], &line, 0, 0, false, 0))
        return STATUS_ERROR;
    if(argc<3)
        GuiScriptSetInfoLine((int)line, "");
    else
        GuiScriptSetInfoLine((int)line, argv[2]);
    return STATUS_CONTINUE;
}
