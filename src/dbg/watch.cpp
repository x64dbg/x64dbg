#include "watch.h"
#include "console.h"
#include "variable.h"
#include "value.h"
#include "threading.h"
#include "debugger.h"
#include "symbolinfo.h"
#include <Windows.h>

std::map<unsigned int, WatchExpr*> watchexpr;
unsigned int idCounter = 1;

WatchExpr::WatchExpr(const char* expression, WatchVarType type) : expr(expression), varType(type), currValue(0), haveCurrValue(false)
{
    if(!expr.IsValidExpression())
        varType = WatchVarType::TYPE_INVALID;
}

duint WatchExpr::getIntValue()
{
    if(varType == WatchVarType::TYPE_UINT || varType == WatchVarType::TYPE_INT)
    {
        duint val;
        bool ok = expr.Calculate(val, varType == WatchVarType::TYPE_INT);
        if(ok)
        {
            currValue = val;
            haveCurrValue = true;
            return val;
        }
    }
    currValue = 0;
    haveCurrValue = false;
    return 0;
}

bool WatchExpr::modifyExpr(const char* expression, WatchVarType type)
{
    ExpressionParser b(expression);
    if(b.IsValidExpression())
    {
        expr = b;
        varType = type;
        currValue = 0;
        haveCurrValue = false;
        return true;
    }
    else
        return false;
}

// Global functions
// Clear all watch
void WatchClear()
{
    if(!watchexpr.empty())
    {
        for(auto i : watchexpr)
            delete i.second;
        watchexpr.clear();
    }
}

unsigned int WatchAddExprUnlocked(const char* expr, WatchVarType type)
{
    unsigned int newid = InterlockedExchangeAdd(&idCounter, 1);
    auto temp = watchexpr.emplace(std::make_pair(newid, new WatchExpr(expr, type)));
    return temp.second ? newid : 0;
}

unsigned int WatchAddExpr(const char* expr, WatchVarType type)
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    return WatchAddExprUnlocked(expr, type);
}

bool WatchModifyExpr(unsigned int id, const char* expr, WatchVarType type)
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    auto obj = watchexpr.find(id);
    if(obj != watchexpr.end())
    {
        return obj->second->modifyExpr(expr, type);
    }
    else
        return false;
}

void WatchDelete(unsigned int id)
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    auto x = watchexpr.find(id);
    if(x != watchexpr.end())
    {
        delete x->second;
        watchexpr.erase(x);
    }
}

void WatchSetWatchdogModeUnlocked(unsigned int id, WatchdogMode mode)
{
    auto obj = watchexpr.find(id);
    if(obj != watchexpr.end())
        obj->second->setWatchdogMode(mode);
}

void WatchSetWatchdogMode(unsigned int id, WatchdogMode mode)
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    WatchSetWatchdogModeUnlocked(id, mode);
}

WatchdogMode WatchGetWatchdogEnabled(unsigned int id)
{
    SHARED_ACQUIRE(LockWatch);
    auto obj = watchexpr.find(id);
    if(obj != watchexpr.end())
        return obj->second->getWatchdogMode();
    else
        return WatchdogMode::MODE_DISABLED;
}

duint WatchGetUnsignedValue(unsigned int id)
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    auto obj = watchexpr.find(id);
    if(obj != watchexpr.end())
        return obj->second->getIntValue();
    else
        return 0;
}

WatchVarType WatchGetType(unsigned int id)
{
    SHARED_ACQUIRE(LockWatch);
    auto obj = watchexpr.find(id);
    if(obj != watchexpr.end())
        return obj->second->getType();
    else
        return WatchVarType::TYPE_INVALID;
}

// Initialize id counter so that it begin with a unused value
void WatchInitIdCounter()
{
    idCounter = 1;
    for(auto i : watchexpr)
        if(i.first > idCounter)
            idCounter = i.first + 1;
}

void WatchCacheSave(JSON root)
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    JSON watchroot = json_array();
    for(auto i : watchexpr)
    {
        if(i.second->getType() == WatchVarType::TYPE_INVALID)
            continue;
        JSON watchitem = json_object();
        json_object_set_new(watchitem, "Expression", json_string(i.second->getExpr().c_str()));
        switch(i.second->getType())
        {
        case WatchVarType::TYPE_INT:
            json_object_set_new(watchitem, "DataType", json_string("int"));
            break;
        case WatchVarType::TYPE_UINT:
            json_object_set_new(watchitem, "DataType", json_string("uint"));
            break;
        case WatchVarType::TYPE_FLOAT:
            json_object_set_new(watchitem, "DataType", json_string("float"));
            break;
        default:
            break;
        }
        switch(i.second->getWatchdogMode())
        {
        case WatchdogMode::MODE_DISABLED:
            json_object_set_new(watchitem, "WatchdogMode", json_string("Disabled"));
            break;
        case WatchdogMode::MODE_CHANGED:
            json_object_set_new(watchitem, "WatchdogMode", json_string("Changed"));
            break;
        case WatchdogMode::MODE_UNCHANGED:
            json_object_set_new(watchitem, "WatchdogMode", json_string("Unchanged"));
            break;
        case WatchdogMode::MODE_ISTRUE:
            json_object_set_new(watchitem, "WatchdogMode", json_string("IsTrue"));
            break;
        case WatchdogMode::MODE_ISFALSE:
            json_object_set_new(watchitem, "WatchdogMode", json_string("IsFalse"));
            break;
        default:
            break;
        }
        json_array_append_new(watchroot, watchitem);
    }
    json_object_set_new(root, "Watch", watchroot);
}

void WatchCacheLoad(JSON root)
{
    WatchClear();
    EXCLUSIVE_ACQUIRE(LockWatch);
    JSON watchroot = json_object_get(root, "Watch");
    unsigned int i;
    JSON val;
    if(!watchroot)
        return;

    json_array_foreach(watchroot, i, val)
    {
        const char* expr = json_string_value(json_object_get(val, "Expression"));
        if(!expr)
            continue;
        const char* datatype = json_string_value(json_object_get(val, "DataType"));
        if(!datatype)
            continue;
        const char* watchdogmode = json_string_value(json_object_get(val, "WatchdogMode"));
        if(!watchdogmode)
            continue;
        WatchVarType varType;
        WatchdogMode watchdogMode;
        if(strcmp(datatype, "int") == 0)
            varType = WatchVarType::TYPE_INT;
        else if(strcmp(datatype, "uint") == 0)
            varType = WatchVarType::TYPE_UINT;
        else if(strcmp(datatype, "float") == 0)
            varType = WatchVarType::TYPE_FLOAT;
        else
            continue;
        if(strcmp(watchdogmode, "Disabled") == 0)
            watchdogMode = WatchdogMode::MODE_DISABLED;
        else if(strcmp(watchdogmode, "Changed") == 0)
            watchdogMode = WatchdogMode::MODE_CHANGED;
        else if(strcmp(watchdogmode, "Unchanged") == 0)
            watchdogMode = WatchdogMode::MODE_UNCHANGED;
        else if(strcmp(watchdogmode, "IsTrue") == 0)
            watchdogMode = WatchdogMode::MODE_ISTRUE;
        else if(strcmp(watchdogmode, "IsFalse") == 0)
            watchdogMode = WatchdogMode::MODE_ISFALSE;
        else
            continue;
        unsigned int id = WatchAddExprUnlocked(expr, varType);
        WatchSetWatchdogModeUnlocked(id, watchdogMode);
    }
    WatchInitIdCounter(); // Initialize id counter so that it begin with a unused value
}

CMDRESULT cbWatchdog(int argc, char* argv[])
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    duint watchdogTriggered = 0;
    auto lbl = SymGetSymbolicName(GetContextDataEx(hActiveThread, UE_CIP));
    for(auto j = watchexpr.begin(); j != watchexpr.end(); j++)
    {
        std::pair<unsigned int, WatchExpr*> i = *j ;
        if(i.second->getType() != WatchVarType::TYPE_INVALID)
        {
            duint origVal = i.second->getCurrIntValue();
            duint newVal = i.second->getIntValue();
            switch(i.second->getWatchdogMode())
            {
            default:
            case WatchdogMode::MODE_DISABLED:
                break;
            case WatchdogMode::MODE_ISTRUE:
                if(newVal != 0)
                {
                    dprintf("Watchdog " fhex " (expression \"%s\") is triggered at %s ! Original value: " fhex ", New value: " fhex "\n", i.first, i.second->getExpr().c_str(), lbl.c_str(), origVal, newVal);
                    watchdogTriggered = 1;
                }
                break;
            case WatchdogMode::MODE_ISFALSE:
                if(newVal == 0)
                {
                    dprintf("Watchdog " fhex " (expression \"%s\") is triggered at %s ! Original value: " fhex ", New value: " fhex "\n", i.first, i.second->getExpr().c_str(), lbl.c_str(), origVal, newVal);
                    watchdogTriggered = 1;
                }
                break;
            case WatchdogMode::MODE_CHANGED:
                if(newVal != origVal || !i.second->HaveCurrentValue())
                {
                    dprintf("Watchdog " fhex " (expression \"%s\") is triggered at %s ! Original value: " fhex ", New value: " fhex "\n", i.first, i.second->getExpr().c_str(), lbl.c_str(), origVal, newVal);
                    watchdogTriggered = 1;
                }
                break;
            case WatchdogMode::MODE_UNCHANGED:
                if(newVal == origVal || !i.second->HaveCurrentValue())
                {
                    dprintf("Watchdog " fhex " (expression \"%s\") is triggered at %s ! Original value: " fhex ", New value: " fhex "\n", i.first, i.second->getExpr().c_str(), lbl.c_str(), origVal, newVal);
                    watchdogTriggered = 1;
                }
                break;
            }
        }
    }
    varset("$result", watchdogTriggered, false);
    if(watchdogTriggered)
        varset("$breakcondition", 1, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbAddWatch(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("No enough arguments for addwatch\n");
        return STATUS_ERROR;
    }
    WatchVarType newtype = WatchVarType::TYPE_INT;
    if(argc > 2)
    {
        if(_stricmp(argv[2], "unsigned"))
            newtype = WatchVarType::TYPE_UINT;
    }
    unsigned int newid = WatchAddExpr(argv[1], newtype);
    varset("$result", newid, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbDelWatch(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("No enough arguments for delwatch\n");
        return STATUS_ERROR;
    }
    duint id;
    bool ok = valfromstring(argv[1], &id);
    if(!ok)
    {
        dputs("Error expression in argument 1.\n");
        return STATUS_ERROR;
    }
    WatchDelete(id);
    return STATUS_CONTINUE;
}

CMDRESULT cbSetWatchdog(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs("No enough arguments for delwatch\n");
        return STATUS_ERROR;
    }
    duint id;
    bool ok = valfromstring(argv[1], &id);
    if(!ok)
    {
        dputs("Error expression in argument 1.\n");
        return STATUS_ERROR;
    }
    WatchdogMode mode;
    if(argc > 2)
    {
        if(_stricmp(argv[2], "disabled") == 0)
            mode = WatchdogMode::MODE_DISABLED;
        else if(_stricmp(argv[2], "changed") == 0)
            mode = WatchdogMode::MODE_CHANGED;
        else if(_stricmp(argv[2], "unchanged") == 0)
            mode = WatchdogMode::MODE_UNCHANGED;
        else if(_stricmp(argv[2], "istrue") == 0)
            mode = WatchdogMode::MODE_ISTRUE;
        else if(_stricmp(argv[2], "isfalse") == 0)
            mode = WatchdogMode::MODE_ISFALSE;
        else
        {
            dputs("Unknown watchdog mode.\n");
            return STATUS_ERROR;
        }
    }
    else
        mode = (WatchGetWatchdogEnabled(id) == WatchdogMode::MODE_DISABLED) ? WatchdogMode::MODE_CHANGED : WatchdogMode::MODE_DISABLED;
    WatchSetWatchdogMode(id, mode);
    return STATUS_CONTINUE;
}
