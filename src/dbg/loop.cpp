#include "loop.h"
#include "memory.h"
#include "threading.h"
#include "module.h"

std::map<DepthModuleRange, LOOPSINFO, DepthModuleRangeCompare> loops;

bool LoopAdd(duint Start, duint End, bool Manual, duint instructionCount)
{
    ASSERT_DEBUGGING("Export call");

    // Loop must begin before it ends
    if(Start > End)
        return false;

    // Memory addresses must be valid
    if(!MemIsValidReadPtr(Start) || !MemIsValidReadPtr(End))
        return false;

    // Check if loop boundaries are in the same module range
    const duint moduleBase = ModBaseFromAddr(Start);

    if(moduleBase != ModBaseFromAddr(End))
        return false;

    // Loops cannot overlap other loops
    int finalDepth = 0;

    if(LoopOverlaps(0, Start, End, &finalDepth))
        return false;

    // Fill out loop information structure
    LOOPSINFO loopInfo;
    loopInfo.start = Start - moduleBase;
    loopInfo.end = End - moduleBase;
    loopInfo.depth = finalDepth;
    loopInfo.manual = Manual;
    loopInfo.instructioncount = instructionCount;
    loopInfo.modhash = ModHashFromAddr(moduleBase);

    // Link this to a parent loop if one does exist
    if(finalDepth)
        LoopGet(finalDepth - 1, Start, &loopInfo.parent, 0);
    else
        loopInfo.parent = 0;
    if(loopInfo.parent)
        loopInfo.parent -= moduleBase;

    EXCLUSIVE_ACQUIRE(LockLoops);

    // Insert into list
    loops.insert(std::make_pair(DepthModuleRange(finalDepth, ModuleRange(loopInfo.modhash, Range(loopInfo.start, loopInfo.end))), loopInfo));
    return true;
}

// Get the start/end of a loop at a certain depth and address
bool LoopGet(int Depth, duint Address, duint* Start, duint* End, duint* InstructionCount)
{
    ASSERT_DEBUGGING("Export call");

    // Get the virtual address module
    const duint moduleBase = ModBaseFromAddr(Address);

    // Virtual address to relative address
    Address -= moduleBase;

    SHARED_ACQUIRE(LockLoops);

    // Search with this address range
    auto found = loops.find(DepthModuleRange(Depth, ModuleRange(ModHashFromAddr(moduleBase), Range(Address, Address))));

    if(found == loops.end())
        return false;

    // Return the loop start and end
    if(Start)
        *Start = found->second.start + moduleBase;

    if(End)
        *End = found->second.end + moduleBase;

    if(InstructionCount)
        *InstructionCount = found->second.instructioncount;

    return true;
}

// Check if a loop overlaps a range, inside is not overlapping
bool LoopOverlaps(int Depth, duint Start, duint End, int* FinalDepth)
{
    ASSERT_DEBUGGING("Export call");

    // Determine module addresses and lookup keys
    const duint moduleBase = ModBaseFromAddr(Start);

    // Virtual addresses to relative addresses
    Start -= moduleBase;
    End -= moduleBase;

    // The last recursive call will set FinalDepth last
    if(FinalDepth)
        *FinalDepth = Depth;

    SHARED_ACQUIRE(LockLoops);

    auto found = loops.find(DepthModuleRange(Depth, ModuleRange(ModHashFromAddr(moduleBase), Range(Start, End))));
    if(found == loops.end())
        return false;

    if(found->second.start <= Start && found->second.end >= End)
        return LoopOverlaps(Depth + 1, Start + moduleBase, End + moduleBase, FinalDepth);

    return true;
}

static bool LoopDeleteAllRange(DepthModuleRange range)
{
    auto erased = 0;
    for(auto found = loops.find(range); found != loops.end(); found = loops.find(range), erased++)
        loops.erase(found);
    return erased > 0;
}

// This should delete a loop and all sub-loops that matches a certain addr
bool LoopDelete(int Depth, duint Address)
{
    // Get the virtual address module
    const duint moduleBase = ModBaseFromAddr(Address);

    // Virtual address to relative address
    Address -= moduleBase;

    SHARED_ACQUIRE(LockLoops);

    // Search with this address range
    auto found = loops.find(DepthModuleRange(Depth, ModuleRange(ModHashFromAddr(moduleBase), Range(Address, Address))));
    if(found == loops.end())
        return false;

    // Delete all loops in the given range increasing the depth until nothing is left to delete
    auto range = found->first;
    while(LoopDeleteAllRange(range))
        range.first++;

    return true;
}

void LoopCacheSave(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockLoops);

    // Create the root JSON objects
    const JSON jsonLoops = json_array();
    const JSON jsonAutoLoops = json_array();

    // Write all entries
    for(auto & itr : loops)
    {
        const LOOPSINFO & currentLoop = itr.second;
        JSON currentJson = json_object();

        json_object_set_new(currentJson, "module", json_string(currentLoop.mod().c_str()));
        json_object_set_new(currentJson, "start", json_hex(currentLoop.start));
        json_object_set_new(currentJson, "end", json_hex(currentLoop.end));
        json_object_set_new(currentJson, "depth", json_integer(currentLoop.depth));
        json_object_set_new(currentJson, "parent", json_hex(currentLoop.parent));
        json_object_set_new(currentJson, "icount", json_hex(currentLoop.instructioncount));

        if(currentLoop.manual)
            json_array_append_new(jsonLoops, currentJson);
        else
            json_array_append_new(jsonAutoLoops, currentJson);
    }

    // Append a link to the global root
    if(json_array_size(jsonLoops))
        json_object_set(Root, "loops", jsonLoops);

    if(json_array_size(jsonAutoLoops))
        json_object_set(Root, "autoloops", jsonAutoLoops);

    // Release memory/references
    json_decref(jsonLoops);
    json_decref(jsonAutoLoops);
}

void LoopCacheLoad(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockLoops);

    // Inline lambda to parse each JSON entry
    auto AddLoops = [](const JSON Object, bool Manual)
    {
        size_t i;
        JSON value;

        json_array_foreach(Object, i, value)
        {
            LOOPSINFO loopInfo;

            // Module name
            const char* mod = json_string_value(json_object_get(value, "module"));

            if(mod && strlen(mod) < MAX_MODULE_SIZE)
                loopInfo.modhash = ModHashFromName(mod);

            // All other variables
            loopInfo.start = (duint)json_hex_value(json_object_get(value, "start"));
            loopInfo.end = (duint)json_hex_value(json_object_get(value, "end"));
            loopInfo.depth = (int)json_integer_value(json_object_get(value, "depth"));
            loopInfo.parent = (duint)json_hex_value(json_object_get(value, "parent"));
            loopInfo.instructioncount = (duint)json_hex_value(json_object_get(value, "icount"));
            loopInfo.manual = Manual;

            // Sanity check: Make sure the loop starts before it ends
            if(loopInfo.end < loopInfo.start)
                continue;

            // Insert into global list
            loops[DepthModuleRange(loopInfo.depth, ModuleRange(loopInfo.modhash, Range(loopInfo.start, loopInfo.end)))] = loopInfo;
        }
    };

    const JSON jsonLoops = json_object_get(Root, "loops");
    const JSON jsonAutoLoops = json_object_get(Root, "autoloops");

    // Load user-set loops
    if(jsonLoops)
        AddLoops(jsonLoops, true);

    // Load auto-set loops
    if(jsonAutoLoops)
        AddLoops(jsonAutoLoops, false);
}

bool LoopEnum(LOOPSINFO* List, size_t* Size)
{
    // If list or size is not requested, fail
    ASSERT_FALSE(!List && !Size);
    SHARED_ACQUIRE(LockLoops);

    // See if the caller requested an output size
    if(Size)
    {
        *Size = loops.size() * sizeof(LOOPSINFO);

        if(!List)
            return true;
    }

    for(auto & itr : loops)
    {
        *List = itr.second;

        // Adjust the offset to a real virtual address
        duint modbase = ModBaseFromName(List->mod().c_str());
        List->start += modbase;
        List->end += modbase;

        List++;
    }

    return true;
}

void LoopClear()
{
    EXCLUSIVE_ACQUIRE(LockLoops);
    std::map<DepthModuleRange, LOOPSINFO, DepthModuleRangeCompare> empty;
    std::swap(loops, empty);
}