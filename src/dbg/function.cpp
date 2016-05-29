#include "function.h"
#include "module.h"
#include "memory.h"
#include "threading.h"

std::map<ModuleRange, FUNCTIONSINFO, ModuleRangeCompare> functions;

bool FunctionAdd(duint Start, duint End, bool Manual, duint InstructionCount)
{
    ASSERT_DEBUGGING("Export call");

    // Make sure memory is readable
    if(!MemIsValidReadPtr(Start))
        return false;

    // Fail if boundary exceeds module size
    const duint moduleBase = ModBaseFromAddr(Start);

    if(moduleBase != ModBaseFromAddr(End))
        return false;

    // Fail if 'Start' and 'End' are incompatible
    if(Start > End || FunctionOverlaps(Start, End))
        return false;

    FUNCTIONSINFO function;
    ModNameFromAddr(Start, function.mod, true);
    function.start = Start - moduleBase;
    function.end = End - moduleBase;
    function.manual = Manual;
    function.instructioncount = InstructionCount;

    // Insert to global table
    EXCLUSIVE_ACQUIRE(LockFunctions);

    functions.insert(std::make_pair(ModuleRange(ModHashFromAddr(moduleBase), Range(function.start, function.end)), function));
    return true;
}

bool FunctionGet(duint Address, duint* Start, duint* End, duint* InstrCount)
{
    ASSERT_DEBUGGING("Export call");

    const duint moduleBase = ModBaseFromAddr(Address);

    // Lookup by module hash, then function range
    SHARED_ACQUIRE(LockFunctions);

    auto found = functions.find(ModuleRange(ModHashFromAddr(moduleBase), Range(Address - moduleBase, Address - moduleBase)));

    // Was this range found?
    if(found == functions.end())
        return false;

    if(Start)
        *Start = found->second.start + moduleBase;

    if(End)
        *End = found->second.end + moduleBase;

    if(InstrCount)
        *InstrCount = found->second.instructioncount;

    return true;
}

bool FunctionOverlaps(duint Start, duint End)
{
    ASSERT_DEBUGGING("Export call");

    // A function can't end before it begins
    if(Start > End)
        return false;

    const duint moduleBase = ModBaseFromAddr(Start);

    SHARED_ACQUIRE(LockFunctions);
    return (functions.count(ModuleRange(ModHashFromAddr(moduleBase), Range(Start - moduleBase, End - moduleBase))) > 0);
}

bool FunctionDelete(duint Address)
{
    ASSERT_DEBUGGING("Export call");

    const duint moduleBase = ModBaseFromAddr(Address);

    EXCLUSIVE_ACQUIRE(LockFunctions);
    return (functions.erase(ModuleRange(ModHashFromAddr(moduleBase), Range(Address - moduleBase, Address - moduleBase))) > 0);
}

void FunctionDelRange(duint Start, duint End, bool DeleteManual)
{
    ASSERT_DEBUGGING("Export call");

    // Should all functions be deleted?
    // 0x00000000 - 0xFFFFFFFF
    if(Start == 0 && End == ~0)
    {
        FunctionClear();
    }
    else
    {
        // The start and end address must be in the same module
        duint moduleBase = ModBaseFromAddr(Start);

        if(moduleBase != ModBaseFromAddr(End))
            return;

        // Convert these to a relative offset
        Start -= moduleBase;
        End -= moduleBase;

        EXCLUSIVE_ACQUIRE(LockFunctions);
        for(auto itr = functions.begin(); itr != functions.end();)
        {
            const auto & currentFunction = itr->second;

            // Ignore manually set entries
            if(!DeleteManual && currentFunction.manual)
            {
                ++itr;
                continue;
            }

            // [Start, End]
            if(currentFunction.end >= Start && currentFunction.start <= End)
                itr = functions.erase(itr);
            else
                ++itr;
        }
    }
}

void FunctionCacheSave(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockFunctions);

    // Allocate JSON object array
    const JSON jsonFunctions = json_array();
    const JSON jsonAutoFunctions = json_array();

    for(auto & i : functions)
    {
        JSON currentFunction = json_object();

        json_object_set_new(currentFunction, "module", json_string(i.second.mod));
        json_object_set_new(currentFunction, "start", json_hex(i.second.start));
        json_object_set_new(currentFunction, "end", json_hex(i.second.end));
        json_object_set_new(currentFunction, "icount", json_hex(i.second.instructioncount));

        if(i.second.manual)
            json_array_append_new(jsonFunctions, currentFunction);
        else
            json_array_append_new(jsonAutoFunctions, currentFunction);
    }

    if(json_array_size(jsonFunctions))
        json_object_set(Root, "functions", jsonFunctions);

    if(json_array_size(jsonAutoFunctions))
        json_object_set(Root, "autofunctions", jsonAutoFunctions);

    // Decrease reference count to avoid leaking memory
    json_decref(jsonFunctions);
    json_decref(jsonAutoFunctions);
}

void FunctionCacheLoad(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockFunctions);

    // Delete existing entries
    functions.clear();

    // Inline lambda to enumerate all JSON array indices
    auto InsertFunctions = [](const JSON Object, bool Manual)
    {
        size_t i;
        JSON value;
        json_array_foreach(Object, i, value)
        {
            FUNCTIONSINFO functionInfo;
            memset(&functionInfo, 0, sizeof(FUNCTIONSINFO));

            // Copy module name
            const char* mod = json_string_value(json_object_get(value, "module"));

            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(functionInfo.mod, mod);

            // Function address
            functionInfo.start = (duint)json_hex_value(json_object_get(value, "start"));
            functionInfo.end = (duint)json_hex_value(json_object_get(value, "end"));
            functionInfo.manual = Manual;
            functionInfo.instructioncount = (duint)json_hex_value(json_object_get(value, "icount"));

            // Sanity check
            if(functionInfo.end < functionInfo.start)
                continue;

            const duint key = ModHashFromName(functionInfo.mod);
            functions.insert(std::make_pair(ModuleRange(key, Range(functionInfo.start, functionInfo.end)), functionInfo));
        }
    };

    const JSON jsonFunctions = json_object_get(Root, "functions");
    const JSON jsonAutoFunctions = json_object_get(Root, "autofunctions");

    // Manual
    if(jsonFunctions)
        InsertFunctions(jsonFunctions, true);

    // Auto
    if(jsonAutoFunctions)
        InsertFunctions(jsonAutoFunctions, false);
}

bool FunctionEnum(FUNCTIONSINFO* List, size_t* Size)
{
    ASSERT_DEBUGGING("Export call");

    // If a list isn't passed and the size not requested, fail
    ASSERT_FALSE(!List && !Size);
    SHARED_ACQUIRE(LockFunctions);

    // Did the caller request the buffer size needed?
    if(Size)
    {
        *Size = functions.size() * sizeof(FUNCTIONSINFO);

        if(!List)
            return true;
    }

    // Fill out the buffer
    for(auto & itr : functions)
    {
        // Adjust for relative to virtual addresses
        duint moduleBase = ModBaseFromName(itr.second.mod);

        *List = itr.second;
        List->start += moduleBase;
        List->end += moduleBase;

        List++;
    }

    return true;
}

void FunctionClear()
{
    EXCLUSIVE_ACQUIRE(LockFunctions);
    functions.clear();
}

void FunctionGetList(std::vector<FUNCTIONSINFO> & list)
{
    SHARED_ACQUIRE(LockFunctions);
    list.clear();
    list.reserve(functions.size());
    for(const auto & itr : functions)
        list.push_back(itr.second);
}

bool FunctionGetInfo(duint Address, FUNCTIONSINFO* info)
{
    auto moduleBase = ModBaseFromAddr(Address);

    // Lookup by module hash, then function range
    SHARED_ACQUIRE(LockFunctions);

    auto found = functions.find(ModuleRange(ModHashFromAddr(moduleBase), Range(Address - moduleBase, Address - moduleBase)));

    // Was this range found?
    if(found == functions.end())
        return false;

    if(info)
        memcpy(info, &found->second, sizeof(FUNCTIONSINFO));
    return true;
}
