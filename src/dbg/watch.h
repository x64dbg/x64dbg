#include "_global.h"
#include "command.h"
#include "expressionparser.h"
#include "jansson\jansson.h"

enum WatchVarType
{
    TYPE_UINT, // unsigned integer
    TYPE_INT,  // signed integer
    TYPE_FLOAT,// single precision floating point value
    TYPE_INVALID // invalid watch expression or data type
};

enum WatchdogMode
{
    MODE_DISABLED, // watchdog is disabled
    MODE_ISTRUE,   // alert if expression is not 0
    MODE_ISFALSE,  // alert if expression is 0
    MODE_CHANGED,  // alert if expression is changed
    MODE_UNCHANGED // alert if expression is not changed
};

class WatchExpr
{
protected:
    ExpressionParser expr;
    WatchdogMode watchdogMode;
    bool haveCurrValue;
    WatchVarType varType;
    duint currValue; // last result of getIntValue()

public:
    WatchExpr(const char* expression, WatchVarType type);
    ~WatchExpr() {};
    duint getIntValue(); // evaluate the expression as integer
    bool modifyExpr(const char* expression, WatchVarType type); // modify the expression and data type

    inline WatchdogMode getWatchdogMode()
    {
        return watchdogMode;
    };
    inline void setWatchdogMode(WatchdogMode mode)
    {
        watchdogMode = mode;
    };
    inline WatchVarType getType()
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
    }
    inline const bool HaveCurrentValue()
    {
        return haveCurrValue;
    };
};

extern std::map<unsigned int, WatchExpr*> watchexpr;

void WatchClear();
unsigned int WatchAddExpr(const char* expr, WatchVarType type);
bool WatchModifyExpr(unsigned int id, const char* expr, WatchVarType type);
void WatchDelete(unsigned int id);
void WatchSetWatchdogMode(unsigned int id, bool isEnabled);
WatchdogMode WatchGetWatchdogMode(unsigned int id);
duint WatchGetUnsignedValue(unsigned int id);
WatchVarType WatchGetType(unsigned int id);
void WatchCacheSave(JSON root); // Save watch data to database
void WatchCacheLoad(JSON root); // Load watch data from database

CMDRESULT cbWatchdog(int argc, char* argv[]);
CMDRESULT cbAddWatch(int argc, char* argv[]);
CMDRESULT cbDelWatch(int argc, char* argv[]);
CMDRESULT cbSetWatchdog(int argc, char* argv[]);

