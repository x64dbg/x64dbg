#include "loop.h"
#include "debugger.h"
#include "memory.h"
#include "threading.h"
#include "module.h"

std::map<DepthModuleRange, LOOPSINFO, DepthModuleRangeCompare> loops;

bool LoopAdd(uint Start, uint End, bool Manual)
{
	// CHECK: Export function
    if(!DbgIsDebugging())
        return false;

	// Loop must begin before it ends
	if (Start > End)
		return false;

	// Memory addresses must be valid
	if (!MemIsValidReadPtr(Start) || !MemIsValidReadPtr(End))
		return false;

	// Check if loop boundaries are in the same module range
    const uint moduleBase = ModBaseFromAddr(Start);

    if(moduleBase != ModBaseFromAddr(End))
        return false;
    
	// Loops cannot overlap other loops
	int finalDepth = 0;

    if(LoopOverlaps(0, Start, End, &finalDepth))
        return false;

	// Fill out loop information structure
    LOOPSINFO loopInfo;

	loopInfo.start	= Start - moduleBase;
	loopInfo.end	= End - moduleBase;
	loopInfo.depth	= finalDepth;
	loopInfo.manual = Manual;
    ModNameFromAddr(Start, loopInfo.mod, true);
    
	// Link this to a parent loop if one does exist
	if(finalDepth)
        LoopGet(finalDepth - 1, Start, &loopInfo.parent, 0);
    else
        loopInfo.parent = 0;

	EXCLUSIVE_ACQUIRE(LockLoops);

	// Insert into list
    loops.insert(std::make_pair(DepthModuleRange(finalDepth, ModuleRange(ModHashFromAddr(moduleBase), Range(loopInfo.start, loopInfo.end))), loopInfo));
    return true;
}

// Get the start/end of a loop at a certain depth and address
bool LoopGet(int Depth, uint Address, uint* Start, uint* End)
{
	// CHECK: Exported function
    if(!DbgIsDebugging())
        return false;

	// Get the virtual address module
    const uint moduleBase = ModBaseFromAddr(Address);

	// Virtual address to relative address
	Address -= moduleBase;

	SHARED_ACQUIRE(LockLoops);

	// Search with this address range
    auto found = loops.find(DepthModuleRange(Depth, ModuleRange(ModHashFromAddr(moduleBase), Range(Address, Address))));

    if(found == loops.end())
        return false;

	// Return the loop start
    if(Start)
        *Start = found->second.start + moduleBase;

	// Also the loop end
    if(End)
        *End = found->second.end + moduleBase;

    return true;
}

//check if a loop overlaps a range, inside is not overlapping
bool LoopOverlaps(int Depth, uint Start, uint End, int* FinalDepth)
{
	// CHECK: Export function
    if(!DbgIsDebugging())
        return false;

	// Determine module addresses and lookup keys
    const uint moduleBase	= ModBaseFromAddr(Start);
	const uint key			= ModHashFromAddr(moduleBase);

	uint curStart	= Start - moduleBase;
    uint curEnd		= End - moduleBase;

	SHARED_ACQUIRE(LockLoops);

    // Check if the new loop fits in the old loop
    for(auto& itr : loops)
    {
		// Only look in the current module
        if(itr.first.second.first != key)
            continue;

		// Loop must be at this recursive depth
		if (itr.second.depth != Depth)
			continue;

        if(itr.second.start < curStart && itr.second.end > curEnd)
            return LoopOverlaps(Depth + 1, curStart, curEnd, FinalDepth);
    }

	// Did the user request t the loop depth?
    if(FinalDepth)
        *FinalDepth = Depth;

    // Check for loop overlaps
	for (auto& itr : loops)
    {
		// Only look in the current module
		if (itr.first.second.first != key)
			continue;

		// Loop must be at this recursive depth
		if (itr.second.depth != Depth)
			continue;

        if(itr.second.start <= curEnd && itr.second.end >= curStart)
            return true;
    }

    return false;
}

// This should delete a loop and all sub-loops that matches a certain addr
bool LoopDelete(int Depth, uint Address)
{
    return false;
}

void LoopCacheSave(JSON Root)
{
	EXCLUSIVE_ACQUIRE(LockLoops);
    const JSON jsonloops = json_array();
    const JSON jsonautoloops = json_array();
	for(auto& itr : loops)
    {
        const LOOPSINFO curLoop = itr.second;
        JSON curjsonloop = json_object();
        json_object_set_new(curjsonloop, "module", json_string(curLoop.mod));
        json_object_set_new(curjsonloop, "start", json_hex(curLoop.start));
        json_object_set_new(curjsonloop, "end", json_hex(curLoop.end));
        json_object_set_new(curjsonloop, "depth", json_integer(curLoop.depth));
        json_object_set_new(curjsonloop, "parent", json_hex(curLoop.parent));
        if(curLoop.manual)
            json_array_append_new(jsonloops, curjsonloop);
        else
            json_array_append_new(jsonautoloops, curjsonloop);
    }
    if(json_array_size(jsonloops))
        json_object_set(Root, "loops", jsonloops);
    json_decref(jsonloops);
    if(json_array_size(jsonautoloops))
        json_object_set(Root, "autoloops", jsonautoloops);
    json_decref(jsonautoloops);
}

void LoopCacheLoad(JSON Root)
{
	EXCLUSIVE_ACQUIRE(LockLoops);
    loops.clear();
    const JSON jsonloops = json_object_get(Root, "loops");
    if(jsonloops)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonloops, i, value)
        {
            LOOPSINFO curLoop;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(curLoop.mod, mod);
            else
                *curLoop.mod = '\0';
            curLoop.start = (uint)json_hex_value(json_object_get(value, "start"));
            curLoop.end = (uint)json_hex_value(json_object_get(value, "end"));
            curLoop.depth = (int)json_integer_value(json_object_get(value, "depth"));
            curLoop.parent = (uint)json_hex_value(json_object_get(value, "parent"));
            if(curLoop.end < curLoop.start)
                continue; //invalid loop
            curLoop.manual = true;
            loops.insert(std::make_pair(DepthModuleRange(curLoop.depth, ModuleRange(ModHashFromName(curLoop.mod), Range(curLoop.start, curLoop.end))), curLoop));
        }
    }
    JSON jsonautoloops = json_object_get(Root, "autoloops");
    if(jsonautoloops)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautoloops, i, value)
        {
            LOOPSINFO curLoop;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(curLoop.mod, mod);
            else
                *curLoop.mod = '\0';
            curLoop.start = (uint)json_hex_value(json_object_get(value, "start"));
            curLoop.end = (uint)json_hex_value(json_object_get(value, "end"));
            curLoop.depth = (int)json_integer_value(json_object_get(value, "depth"));
            curLoop.parent = (uint)json_hex_value(json_object_get(value, "parent"));
            if(curLoop.end < curLoop.start)
                continue; //invalid loop
            curLoop.manual = false;
            loops.insert(std::make_pair(DepthModuleRange(curLoop.depth, ModuleRange(ModHashFromName(curLoop.mod), Range(curLoop.start, curLoop.end))), curLoop));
        }
    }
}

bool LoopEnum(LOOPSINFO* List, size_t* Size)
{
    // If list or size is not requested, fail
    if(!List && !Size)
        return false;

    SHARED_ACQUIRE(LockLoops);

    // See if the caller requested an output size
    if(Size)
    {
        *Size = loops.size() * sizeof(LOOPSINFO);

        if(!List)
            return true;
    }

	for (auto& itr : loops)
    {
        *List = itr.second;

        // Adjust the offset to a real virtual address
        uint modbase    = ModBaseFromName(List->mod);
        List->start		+= modbase;
        List->end		+= modbase;

        List++;
    }

    return true;
}

void LoopClear()
{
    EXCLUSIVE_ACQUIRE(LockLoops);
    loops.clear();
}