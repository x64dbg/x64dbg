#pragma once

#include "_global.h"
#include "jansson/jansson_x64dbg.h"
#include "expressionparser.h"
#include <map>

class WatchExpr
{
protected:
    char WatchName[MAX_WATCH_NAME_SIZE];
    ExpressionParser expr;
    WATCHDOGMODE watchdogMode;
    bool haveCurrValue;
    WATCHVARTYPE varType;
    duint currValue; // last result of getIntValue()

public:
    bool watchdogTriggered;
    unsigned int watchWindow;

    WatchExpr(const char* name, const char* expression, WATCHVARTYPE type);
    ~WatchExpr() {};
    duint getIntValue(); // evaluate the expression as integer
    bool modifyExpr(const char* expression, WATCHVARTYPE type); // modify the expression and data type
    void modifyName(const char* newName);

    inline WATCHDOGMODE getWatchdogMode()
    {
        return watchdogMode;
    };
    inline char* getName()
    {
        return WatchName;
    };
    inline void setWatchdogMode(WATCHDOGMODE mode)
    {
        watchdogMode = mode;
    };
    inline WATCHVARTYPE getType()
    {
        return varType;
    };
    inline duint getCurrIntValue()
    {
        return currValue;
    };
    inline const String & getExpr()
    {
        return expr.GetExpression();
    };
    inline const bool HaveCurrentValue()
    {
        return haveCurrValue;
    };
    inline void setType(WATCHVARTYPE type)
    {
        varType = type;
    };
};

extern std::map<unsigned int, WatchExpr*> watchexpr;

void GuiUpdateWatchViewAsync();
void WatchClear();
unsigned int WatchAddExpr(const char* expr, WATCHVARTYPE type);
bool WatchModifyExpr(unsigned int id, const char* expr, WATCHVARTYPE type);
void WatchModifyName(unsigned int id, const char* newName);
void WatchDelete(unsigned int id);
void WatchSetWatchdogMode(unsigned int id, WATCHDOGMODE mode);
bool WatchIsWatchdogTriggered(unsigned int id);
WATCHDOGMODE WatchGetWatchdogMode(unsigned int id);
WATCHDOGMODE WatchGetWatchdogEnabled(unsigned int id);
duint WatchGetUnsignedValue(unsigned int id);
WATCHVARTYPE WatchGetType(unsigned int id);
void WatchSetType(unsigned int id, WATCHVARTYPE type);
std::vector<WATCHINFO> WatchGetList();

void WatchCacheSave(JSON root); // Save watch data to database
void WatchCacheLoad(JSON root); // Load watch data from database
