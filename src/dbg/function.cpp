#include "function.h"
#include "module.h"
#include "memory.h"
#include "threading.h"
#include "database_cb_batcher.h"

struct FunctionSerializer : RangeInfoSerializer<FUNCTIONSINFO>
{
    bool Save(const FUNCTIONSINFO & value) override
    {
        RangeInfoSerializer::Save(value);
        setHex("icount", value.instructioncount);
        setHex("parent", value.parent);
        return true;
    }

    bool Load(FUNCTIONSINFO & value) override
    {
        if(!RangeInfoSerializer::Load(value))
            return false;
        value.parent = 0;
        getHex("parent", value.parent);
        return getHex("icount", value.instructioncount);
    }
};

struct Functions : RangeInfoMap<LockFunctions, FUNCTIONSINFO, FunctionSerializer>
{
    void AdjustValue(FUNCTIONSINFO & value) const override
    {
        auto base = ModBaseFromName(value.mod().c_str());
        RangeInfoMap::AdjustValue(value);
        value.parent += base;
    }

protected:
    const char* jsonKey() const override
    {
        return "functions";
    }

    bool populateDbOperation(DbOperation & op, const FUNCTIONSINFO & value) const override
    {
        op.itemType = DbItemTypeFunction;
        op.manual = value.manual;
        op.modhash = value.modhash;
        op.address = value.start;
        op.function.end = value.end;
        op.function.parent = value.parent;
        op.function.icount = value.instructioncount;
        return true;
    }
};

static Functions functions;

bool FunctionAdd(duint Start, duint End, bool Manual, duint InstructionCount, duint Parent)
{
    FUNCTIONSINFO function;
    if(!functions.PrepareValue(function, Start, End, Manual) || FunctionOverlaps(Start, End))
        return false;

    function.instructioncount = InstructionCount;
    function.parent = (Parent ? Parent : Start) - ModBaseFromAddr(Start);

    return functions.Add(function);
}

bool FunctionGet(duint Address, duint* Start, duint* End, duint* InstrCount, duint* Parent)
{
    FUNCTIONSINFO function;
    if(!functions.Get(Functions::VaKey(Address, Address), function))
        return false;
    functions.AdjustValue(function);
    if(Start)
        *Start = function.start;
    if(End)
        *End = function.end;
    if(InstrCount)
        *InstrCount = function.instructioncount;
    if(Parent)
        *Parent = function.parent;
    return true;
}

bool FunctionOverlaps(duint Start, duint End)
{
    // A function can't end before it begins
    if(Start > End)
        return false;
    return functions.Contains(Functions::VaKey(Start, End));
}

bool FunctionDelete(duint Address)
{
    return functions.Delete(Functions::VaKey(Address, Address));
}

void FunctionDelRange(duint Start, duint End, bool DeleteManual)
{
    // Should all functions be deleted?
    // 0x00000000 - 0xFFFFFFFF
    if(Start == 0 && End == ~0)
    {
        FunctionClear(false);
    }
    else
    {
        // The start and end address must be in the same module
        auto moduleBase = ModBaseFromAddr(Start);

        if(moduleBase != ModBaseFromAddr(End))
            return;

        // Convert these to a relative offset
        Start -= moduleBase;
        End -= moduleBase;

        DbCallbackBatcher batcher;

        functions.DeleteWhere([ = ](const FUNCTIONSINFO & value)
        {
            if(!DeleteManual && value.manual)
                return false;
            return value.end >= Start && value.start <= End;
        });
    }
}

void FunctionCacheSave(JSON Root)
{
    functions.CacheSave(Root);
}

void FunctionCacheLoad(JSON Root)
{
    DbCallbackBatcher batcher(true);
    functions.CacheLoad(Root);
    functions.CacheLoad(Root, "auto"); //legacy support
}

bool FunctionEnum(FUNCTIONSINFO* List, size_t* Size)
{
    return functions.Enum(List, Size);
}

void FunctionClear(bool Terminating)
{
    DbCallbackBatcher batcher;
    functions.Clear(Terminating);
}

void FunctionGetList(std::vector<FUNCTIONSINFO> & list)
{
    functions.GetList(list);
}

bool FunctionGetInfo(duint Address, FUNCTIONSINFO & info)
{
    return functions.Get(Functions::VaKey(Address, Address), info);
}
