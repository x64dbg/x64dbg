#include "function.h"
#include "module.h"
#include "memory.h"
#include "threading.h"

struct FunctionSerializer : JSONWrapper<FUNCTIONSINFO>
{
    bool Save(const FUNCTIONSINFO & value) override
    {
        setString("module", value.mod());
        setHex("start", value.start);
        setHex("end", value.end);
        setHex("icount", value.instructioncount);
        setBool("manual", value.manual);
        return true;
    }

    bool Load(FUNCTIONSINFO & value) override
    {
        //legacy support
        value.manual = true;
        getBool("manual", value.manual);
        std::string mod;
        if(!getString("module", mod))
            return false;
        value.modhash = ModHashFromName(mod.c_str());
        return getHex("start", value.start) &&
               getHex("end", value.end) &&
               getHex("icount", value.instructioncount) &&
               value.end >= value.start;
    }
};

struct Functions : SerializableModuleRangeMap<LockFunctions, FUNCTIONSINFO, FunctionSerializer>
{
    void AdjustValue(FUNCTIONSINFO & value) const override
    {
        auto base = ModBaseFromName(value.mod().c_str());
        value.start += base;
        value.end += base;
    }

protected:
    const char* jsonKey() const override
    {
        return "functions";
    }

    ModuleRange makeKey(const FUNCTIONSINFO & value) const override
    {
        return ModuleRange(value.modhash, Range(value.start, value.end));
    }
};

static Functions functions;

bool FunctionAdd(duint Start, duint End, bool Manual, duint InstructionCount)
{
    // Make sure memory is readable
    if(!MemIsValidReadPtr(Start))
        return false;

    // Fail if boundary exceeds module size
    auto moduleBase = ModBaseFromAddr(Start);

    if(moduleBase != ModBaseFromAddr(End))
        return false;

    // Fail if 'Start' and 'End' are incompatible
    if(Start > End || FunctionOverlaps(Start, End))
        return false;

    FUNCTIONSINFO function;
    function.modhash = ModHashFromAddr(moduleBase);
    function.start = Start - moduleBase;
    function.end = End - moduleBase;
    function.manual = Manual;
    function.instructioncount = InstructionCount;

    return functions.Add(function);
}

bool FunctionGet(duint Address, duint* Start, duint* End, duint* InstrCount)
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
        FunctionClear();
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
    functions.CacheLoad(Root);
    functions.CacheLoad(Root, "auto"); //legacy support
}

bool FunctionEnum(FUNCTIONSINFO* List, size_t* Size)
{
    return functions.Enum(List, Size);
}

void FunctionClear()
{
    functions.Clear();
}

void FunctionGetList(std::vector<FUNCTIONSINFO> & list)
{
    functions.GetList(list);
}

bool FunctionGetInfo(duint Address, FUNCTIONSINFO & info)
{
    return functions.Get(Functions::VaKey(Address, Address), info);
}
