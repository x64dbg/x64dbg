#include "watch.h"
#include "console.h"
#include "threading.h"
#include "debugger.h"
#include "taskthread.h"
#include <Windows.h>

std::map<unsigned int, WatchExpr*> watchexpr;
unsigned int idCounter = 1;

WatchExpr::WatchExpr(const char* name, const char* expression, WATCHVARTYPE type) :
    expr(expression),
    haveCurrValue(false),
    varType(type), currValue(0),
    watchdogTriggered(false),
    watchWindow(0),
    watchdogMode(MODE_DISABLED)
{
    if(!expr.IsValidExpression())
        varType = WATCHVARTYPE::TYPE_INVALID;
    strcpy_s(this->WatchName, name);
}

duint WatchExpr::getIntValue()
{
    duint origVal = currValue;
    if(varType == WATCHVARTYPE::TYPE_UINT || varType == WATCHVARTYPE::TYPE_INT || varType == WATCHVARTYPE::TYPE_ASCII || varType == WATCHVARTYPE::TYPE_UNICODE)
    {
        duint val;
        bool ok = expr.Calculate(val, varType == WATCHVARTYPE::TYPE_INT, false);
        if(ok)
        {
            currValue = val;
            haveCurrValue = true;
            if(getType() != WATCHVARTYPE::TYPE_INVALID)
            {
                switch(getWatchdogMode())
                {
                default:
                case WATCHDOGMODE::MODE_DISABLED:
                    break;
                case WATCHDOGMODE::MODE_ISTRUE:
                    if(currValue != 0)
                    {
                        duint cip = GetContextDataEx(hActiveThread, UE_CIP);
                        dprintf(QT_TRANSLATE_NOOP("DBG", "Watchdog %s (expression \"%s\") is triggered at %p ! Original value: %p, New value: %p\n"), WatchName, getExpr().c_str(), cip, origVal, currValue);
                        watchdogTriggered = 1;
                    }
                    break;
                case WATCHDOGMODE::MODE_ISFALSE:
                    if(currValue == 0)
                    {
                        duint cip = GetContextDataEx(hActiveThread, UE_CIP);
                        dprintf(QT_TRANSLATE_NOOP("DBG", "Watchdog %s (expression \"%s\") is triggered at %p ! Original value: %p, New value: %p\n"), WatchName, getExpr().c_str(), cip, origVal, currValue);
                        watchdogTriggered = 1;
                    }
                    break;
                case WATCHDOGMODE::MODE_CHANGED:
                    if(currValue != origVal || !haveCurrValue)
                    {
                        duint cip = GetContextDataEx(hActiveThread, UE_CIP);
                        dprintf(QT_TRANSLATE_NOOP("DBG", "Watchdog %s (expression \"%s\") is triggered at %p ! Original value: %p, New value: %p\n"), WatchName, getExpr().c_str(), cip, origVal, currValue);
                        watchdogTriggered = 1;
                    }
                    break;
                case WATCHDOGMODE::MODE_UNCHANGED:
                    if(currValue == origVal || !haveCurrValue)
                    {
                        duint cip = GetContextDataEx(hActiveThread, UE_CIP);
                        dprintf(QT_TRANSLATE_NOOP("DBG", "Watchdog %s (expression \"%s\") is triggered at %p ! Original value: %p, New value: %p\n"), WatchName, getExpr().c_str(), cip, origVal, currValue);
                        watchdogTriggered = 1;
                    }
                    break;
                }
            }
            return val;
        }
    }
    currValue = 0;
    haveCurrValue = false;
    return 0;
}

bool WatchExpr::modifyExpr(const char* expression, WATCHVARTYPE type)
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

void WatchExpr::modifyName(const char* newName)
{
    strcpy_s(WatchName, newName);
}

// Update GUI

void GuiUpdateWatchViewAsync()
{
    static TaskThread_<decltype(&GuiUpdateWatchView)> task(&GuiUpdateWatchView);
    task.WakeUp();
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

unsigned int WatchAddExprUnlocked(const char* expr, WATCHVARTYPE type)
{
    unsigned int newid = InterlockedExchangeAdd((volatile long*)&idCounter, 1);
    char DefaultName[MAX_WATCH_NAME_SIZE];
    sprintf_s(DefaultName, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Watch %u")), newid);
    WatchExpr* newWatch = new WatchExpr(DefaultName, expr, type);
    auto temp = watchexpr.emplace(std::make_pair(newid, newWatch));
    if(temp.second)
    {
        newWatch->getIntValue();
        return newid;
    }
    else
    {
        delete newWatch;
        return 0;
    }
}

unsigned int WatchAddExpr(const char* expr, WATCHVARTYPE type)
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    unsigned int id = WatchAddExprUnlocked(expr, type);
    EXCLUSIVE_RELEASE();
    GuiUpdateWatchView();
    return id;
}

bool WatchModifyExpr(unsigned int id, const char* expr, WATCHVARTYPE type)
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    auto obj = watchexpr.find(id);
    if(obj != watchexpr.end())
    {
        bool success = obj->second->modifyExpr(expr, type);
        EXCLUSIVE_RELEASE();
        GuiUpdateWatchView();
        return success;
    }
    else
        return false;
}

void WatchModifyNameUnlocked(unsigned int id, const char* newName)
{
    auto obj = watchexpr.find(id);
    if(obj != watchexpr.end())
    {
        obj->second->modifyName(newName);
    }
}

void WatchModifyName(unsigned int id, const char* newName)
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    WatchModifyNameUnlocked(id, newName);
    EXCLUSIVE_RELEASE();
    GuiUpdateWatchViewAsync();
}

void WatchDelete(unsigned int id)
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    auto x = watchexpr.find(id);
    if(x != watchexpr.end())
    {
        delete x->second;
        watchexpr.erase(x);
        EXCLUSIVE_RELEASE();
        GuiUpdateWatchViewAsync();
    }
}

void WatchSetWatchdogModeUnlocked(unsigned int id, WATCHDOGMODE mode)
{
    auto obj = watchexpr.find(id);
    if(obj != watchexpr.end())
        obj->second->setWatchdogMode(mode);
}

void WatchSetWatchdogMode(unsigned int id, WATCHDOGMODE mode)
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    WatchSetWatchdogModeUnlocked(id, mode);
    EXCLUSIVE_RELEASE();
    GuiUpdateWatchViewAsync();
}

WATCHDOGMODE WatchGetWatchdogEnabled(unsigned int id)
{
    SHARED_ACQUIRE(LockWatch);
    auto obj = watchexpr.find(id);
    if(obj != watchexpr.end())
        return obj->second->getWatchdogMode();
    else
        return WATCHDOGMODE::MODE_DISABLED;
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

WATCHVARTYPE WatchGetType(unsigned int id)
{
    SHARED_ACQUIRE(LockWatch);
    auto obj = watchexpr.find(id);
    if(obj != watchexpr.end())
        return obj->second->getType();
    else
        return WATCHVARTYPE::TYPE_INVALID;
}

unsigned int WatchGetWindow(unsigned int id)
{
    SHARED_ACQUIRE(LockWatch);
    auto obj = watchexpr.find(id);
    if(obj != watchexpr.end())
        return obj->second->watchWindow;
    else
        return WATCHVARTYPE::TYPE_INVALID;
}

bool WatchIsWatchdogTriggered(unsigned int id)
{
    SHARED_ACQUIRE(LockWatch);
    auto obj = watchexpr.find(id);
    if(obj != watchexpr.end())
        return obj->second->watchdogTriggered;
    else
        return false;
}

void WatchSetWindow(unsigned int id, unsigned int window)
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    auto obj = watchexpr.find(id);
    if(obj != watchexpr.end())
        obj->second->watchWindow = window;
    EXCLUSIVE_RELEASE();
    GuiUpdateWatchViewAsync();
}

std::vector<WATCHINFO> WatchGetList()
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    std::vector<WATCHINFO> watchList;
    for(auto & i : watchexpr)
    {
        WATCHINFO info;
        info.value = i.second->getCurrIntValue();
        strcpy_s(info.WatchName, i.second->getName());
        strcpy_s(info.Expression, i.second->getExpr().c_str());
        info.varType = i.second->getType();
        info.id = i.first;
        info.watchdogMode = i.second->getWatchdogMode();
        info.watchdogTriggered = i.second->watchdogTriggered;
        info.window = i.second->watchWindow;
        watchList.push_back(info);
    }
    return watchList;
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
        if(i.second->getType() == WATCHVARTYPE::TYPE_INVALID)
            continue;
        JSON watchitem = json_object();
        json_object_set_new(watchitem, "Expression", json_string(i.second->getExpr().c_str()));
        json_object_set_new(watchitem, "Name", json_string(i.second->getName()));
        switch(i.second->getType())
        {
        case WATCHVARTYPE::TYPE_INT:
            json_object_set_new(watchitem, "DataType", json_string("int"));
            break;
        case WATCHVARTYPE::TYPE_UINT:
            json_object_set_new(watchitem, "DataType", json_string("uint"));
            break;
        case WATCHVARTYPE::TYPE_FLOAT:
            json_object_set_new(watchitem, "DataType", json_string("float"));
            break;
        case WATCHVARTYPE::TYPE_ASCII:
            json_object_set_new(watchitem, "DataType", json_string("ascii"));
            break;
        case WATCHVARTYPE::TYPE_UNICODE:
            json_object_set_new(watchitem, "DataType", json_string("unicode"));
            break;
        default:
            break;
        }
        switch(i.second->getWatchdogMode())
        {
        case WATCHDOGMODE::MODE_DISABLED:
            json_object_set_new(watchitem, "WatchdogMode", json_string("Disabled"));
            break;
        case WATCHDOGMODE::MODE_CHANGED:
            json_object_set_new(watchitem, "WatchdogMode", json_string("Changed"));
            break;
        case WATCHDOGMODE::MODE_UNCHANGED:
            json_object_set_new(watchitem, "WatchdogMode", json_string("Unchanged"));
            break;
        case WATCHDOGMODE::MODE_ISTRUE:
            json_object_set_new(watchitem, "WatchdogMode", json_string("IsTrue"));
            break;
        case WATCHDOGMODE::MODE_ISFALSE:
            json_object_set_new(watchitem, "WatchdogMode", json_string("IsFalse"));
            break;
        default:
            break;
        }
        json_array_append_new(watchroot, watchitem);
    }
    if(json_array_size(watchroot))
        json_object_set(root, "watch", watchroot);

    json_decref(watchroot);
}

void WatchCacheLoad(JSON root)
{
    EXCLUSIVE_ACQUIRE(LockWatch);
    JSON watchroot = json_object_get(root, "watch");
    if(!watchroot)
        watchroot = json_object_get(root, "Watch");
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
            datatype = "uint";
        const char* WatchdogMode = json_string_value(json_object_get(val, "WatchdogMode"));
        if(!WatchdogMode)
            WatchdogMode = "Disabled";
        const char* watchname = json_string_value(json_object_get(val, "Name"));
        if(!watchname)
            watchname = "Watch";
        WATCHVARTYPE varType;
        WATCHDOGMODE watchdog_mode;
        if(strcmp(datatype, "int") == 0)
            varType = WATCHVARTYPE::TYPE_INT;
        else if(strcmp(datatype, "uint") == 0)
            varType = WATCHVARTYPE::TYPE_UINT;
        else if(strcmp(datatype, "float") == 0)
            varType = WATCHVARTYPE::TYPE_FLOAT;
        else if(strcmp(datatype, "ascii") == 0)
            varType = WATCHVARTYPE::TYPE_ASCII;
        else if(strcmp(datatype, "unicode") == 0)
            varType = WATCHVARTYPE::TYPE_UNICODE;
        else
            continue;
        if(strcmp(WatchdogMode, "Disabled") == 0)
            watchdog_mode = WATCHDOGMODE::MODE_DISABLED;
        else if(strcmp(WatchdogMode, "Changed") == 0)
            watchdog_mode = WATCHDOGMODE::MODE_CHANGED;
        else if(strcmp(WatchdogMode, "Unchanged") == 0)
            watchdog_mode = WATCHDOGMODE::MODE_UNCHANGED;
        else if(strcmp(WatchdogMode, "IsTrue") == 0)
            watchdog_mode = WATCHDOGMODE::MODE_ISTRUE;
        else if(strcmp(WatchdogMode, "IsFalse") == 0)
            watchdog_mode = WATCHDOGMODE::MODE_ISFALSE;
        else
            continue;
        unsigned int id = WatchAddExprUnlocked(expr, varType);
        WatchModifyNameUnlocked(id, watchname);
        WatchSetWatchdogModeUnlocked(id, watchdog_mode);
    }
    WatchInitIdCounter(); // Initialize id counter so that it begin with a unused value
}