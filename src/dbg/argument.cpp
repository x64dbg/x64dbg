#include "argument.h"
#include "module.h"
#include "memory.h"
#include "threading.h"
#include "database_cb_batcher.h"

struct ArgumentSerializer : RangeInfoSerializer<ARGUMENTSINFO>
{
    bool Save(const ARGUMENTSINFO & value) override
    {
        RangeInfoSerializer::Save(value);
        setHex("icount", value.instructioncount);
        return true;
    }

    bool Load(ARGUMENTSINFO & value) override
    {
        if(!getBool("manual", value.manual) || !RangeInfoSerializer::Load(value))
            return false;
        return getHex("icount", value.instructioncount);
    }
};

struct Arguments : RangeInfoMap<LockArguments, ARGUMENTSINFO, ArgumentSerializer>
{
protected:
    const char* jsonKey() const override
    {
        return "arguments";
    }

    bool populateDbOperation(DbOperation & op, const ARGUMENTSINFO & value) const override
    {
        op.itemType = DbItemTypeArgument;
        op.manual = value.manual;
        op.modhash = value.modhash;
        op.address = value.start;
        op.argument.end = value.end;
        op.argument.icount = value.instructioncount;
        return true;
    }
};

static Arguments arguments;

bool ArgumentAdd(duint Start, duint End, bool Manual, duint InstructionCount)
{
    ARGUMENTSINFO argument;
    if(!arguments.PrepareValue(argument, Start, End, Manual) || ArgumentOverlaps(Start, End))
        return false;

    argument.instructioncount = InstructionCount;

    return arguments.Add(argument);
}

bool ArgumentGet(duint Address, duint* Start, duint* End, duint* InstrCount)
{
    ARGUMENTSINFO argument;
    if(!arguments.Get(Arguments::VaKey(Address, Address), argument))
        return false;
    arguments.AdjustValue(argument);
    if(Start)
        *Start = argument.start;
    if(End)
        *End = argument.end;
    if(InstrCount)
        *InstrCount = argument.instructioncount;
    return true;
}

bool ArgumentOverlaps(duint Start, duint End)
{
    // A argument can't end before it begins
    if(Start > End)
        return false;
    return arguments.Contains(Arguments::VaKey(Start, End));
}

bool ArgumentDelete(duint Address)
{
    return arguments.Delete(Arguments::VaKey(Address, Address));
}

void ArgumentDelRange(duint Start, duint End, bool DeleteManual)
{
    // Should all arguments be deleted?
    // 0x00000000 - 0xFFFFFFFF
    if(Start == 0 && End == ~0)
    {
        ArgumentClear(false);
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

        arguments.DeleteWhere([ = ](const ARGUMENTSINFO & value)
        {
            if(!DeleteManual && value.manual)
                return false;
            return value.end >= Start && value.start <= End;
        });
    }
}

void ArgumentCacheSave(JSON Root)
{
    arguments.CacheSave(Root);
}

void ArgumentCacheLoad(JSON Root)
{
    DbCallbackBatcher batcher(true);
    arguments.CacheLoad(Root);
}

void ArgumentClear(bool Terminating)
{
    DbCallbackBatcher batcher;
    arguments.Clear(Terminating);
}

void ArgumentGetList(std::vector<ARGUMENTSINFO> & list)
{
    arguments.GetList(list);
}

bool ArgumentGetInfo(duint Address, ARGUMENTSINFO & info)
{
    return arguments.Get(Arguments::VaKey(Address, Address), info);
}

bool ArgumentEnum(ARGUMENTSINFO* List, size_t* Size)
{
    return arguments.Enum(List, Size);
}
