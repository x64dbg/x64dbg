#ifndef BRIDGERESULT_H
#define BRIDGERESULT_H

#include "Imports.h"

class BridgeResult
{
public:
    enum Type
    {
        ScriptAdd,
        ScriptMessage,
        RefInitialize,
        MenuAddToList,
        MenuAdd,
        MenuAddEntry,
        MenuAddSeparator,
        MenuClear,
        MenuRemove,
        SelectionGet,
        SelectionSet,
        GetlineWindow,
        MenuSetIcon,
        MenuSetEntryIcon,
        MenuSetEntryChecked,
        MenuSetVisible,
        MenuSetEntryVisible,
        MenuSetName,
        MenuSetEntryName,
        GetGlobalNotes,
        GetDebuggeeNotes,
        RegisterScriptLang,
        LoadGraph,
        GraphAt,
        GetActiveView,
        TypeAddNode,
        TypeClear,
        MenuSetEntryHotkey,
        GraphCurrent,
        Last,
    };
    explicit BridgeResult(Type type);
    ~BridgeResult();
    dsint Wait();
private:
    Type mType;
};

#endif // BRIDGERESULT_H
