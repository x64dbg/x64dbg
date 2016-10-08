#include "cmd-watch-control.h"
#include "variable.h"
#include "watch.h"
#include "console.h"
#include "threading.h"

CMDRESULT cbAddWatch(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "No enough arguments for addwatch\n"));
        return STATUS_ERROR;
    }
    WATCHVARTYPE newtype = WATCHVARTYPE::TYPE_UINT;
    if(argc > 2)
    {
        if(_stricmp(argv[2], "uint") == 0)
            newtype = WATCHVARTYPE::TYPE_UINT;
        else if(_stricmp(argv[2], "int") == 0)
            newtype = WATCHVARTYPE::TYPE_INT;
        else if(_stricmp(argv[2], "float") == 0)
            newtype = WATCHVARTYPE::TYPE_FLOAT;
        else if(_stricmp(argv[2], "ascii") == 0)
            newtype = WATCHVARTYPE::TYPE_ASCII;
        else if(_stricmp(argv[2], "unicode") == 0)
            newtype = WATCHVARTYPE::TYPE_UNICODE;
    }
    unsigned int newid = WatchAddExpr(argv[1], newtype);
    varset("$result", newid, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbDelWatch(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "No enough arguments for delwatch\n"));
        return STATUS_ERROR;
    }
    duint id;
    bool ok = valfromstring(argv[1], &id);
    if(!ok)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error expression in argument 1.\n"));
        return STATUS_ERROR;
    }
    WatchDelete((unsigned int)id);
    return STATUS_CONTINUE;
}

CMDRESULT cbSetWatchdog(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "No enough arguments for delwatch\n"));
        return STATUS_ERROR;
    }
    duint id;
    bool ok = valfromstring(argv[1], &id);
    if(!ok)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error expression in argument 1.\n"));
        return STATUS_ERROR;
    }
    WATCHDOGMODE mode;
    if(argc > 2)
    {
        if(_stricmp(argv[2], "disabled") == 0)
            mode = WATCHDOGMODE::MODE_DISABLED;
        else if(_stricmp(argv[2], "changed") == 0)
            mode = WATCHDOGMODE::MODE_CHANGED;
        else if(_stricmp(argv[2], "unchanged") == 0)
            mode = WATCHDOGMODE::MODE_UNCHANGED;
        else if(_stricmp(argv[2], "istrue") == 0)
            mode = WATCHDOGMODE::MODE_ISTRUE;
        else if(_stricmp(argv[2], "isfalse") == 0)
            mode = WATCHDOGMODE::MODE_ISFALSE;
        else
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Unknown watchdog mode.\n"));
            return STATUS_ERROR;
        }
    }
    else
        mode = (WatchGetWatchdogEnabled((unsigned int)id) == WATCHDOGMODE::MODE_DISABLED) ? WATCHDOGMODE::MODE_CHANGED : WATCHDOGMODE::MODE_DISABLED;
    WatchSetWatchdogMode((unsigned int)id, mode);
    return STATUS_CONTINUE;
}

CMDRESULT cbSetWatchExpression(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "No enough arguments for SetWatchExpression"));
        return STATUS_ERROR;
    }
    duint id;
    bool ok = valfromstring(argv[1], &id);
    if(ok)
    {
        WATCHVARTYPE varType;
        if(argc > 3)
        {
            if(_stricmp(argv[3], "uint") == 0)
                varType = WATCHVARTYPE::TYPE_UINT;
            else if(_stricmp(argv[3], "int") == 0)
                varType = WATCHVARTYPE::TYPE_INT;
            else if(_stricmp(argv[3], "float") == 0)
                varType = WATCHVARTYPE::TYPE_FLOAT;
            else if(_stricmp(argv[3], "ascii") == 0)
                varType = WATCHVARTYPE::TYPE_ASCII;
            else if(_stricmp(argv[3], "unicode") == 0)
                varType = WATCHVARTYPE::TYPE_UNICODE;
            else
                varType = WATCHVARTYPE::TYPE_UINT;
        }
        else
            varType = WATCHVARTYPE::TYPE_UINT;
        WatchModifyExpr((unsigned int)id, argv[2], varType);
        return STATUS_CONTINUE;
    }
    else
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error expression in argument 1.\n"));
        return STATUS_ERROR;
    }
}

CMDRESULT cbSetWatchName(int argc, char* argv[])
{
    if(argc < 3)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "No enough arguments for SetWatchName"));
        return STATUS_ERROR;
    }
    duint id;
    bool ok = valfromstring(argv[1], &id);
    if(ok)
    {
        WatchModifyName((unsigned int)id, argv[2]);
        return STATUS_CONTINUE;
    }
    else
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error expression in argument 1.\n"));
        return STATUS_ERROR;
    }
}

CMDRESULT cbCheckWatchdog(int argc, char* argv[])
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    bool watchdogTriggered = false;
    for(auto j = watchexpr.begin(); j != watchexpr.end(); ++j)
    {
        std::pair<unsigned int, WatchExpr*> i = *j;
        i.second->watchdogTriggered = false;
        duint intVal = i.second->getIntValue();
        watchdogTriggered |= i.second->watchdogTriggered;
    }
    EXCLUSIVE_RELEASE();
    if(watchdogTriggered)
        GuiUpdateWatchViewAsync();
    varset("$result", watchdogTriggered ? 1 : 0, false);
    return STATUS_CONTINUE;
}