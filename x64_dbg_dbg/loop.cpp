#include "loop.h"
#include "debugger.h"
#include "memory.h"
#include "threading.h"
#include "module.h"

typedef std::map<DepthModuleRange, LOOPSINFO, DepthModuleRangeCompare> LoopsInfo;

static LoopsInfo loops;

bool loopadd(uint start, uint end, bool manual)
{
    if(!DbgIsDebugging() or end < start or !MemIsValidReadPtr(start))
        return false;
    const uint modbase = ModBaseFromAddr(start);
    if(modbase != ModBaseFromAddr(end)) //the function boundaries are not in the same mem page
        return false;
    int finaldepth;
    if(loopoverlaps(0, start, end, &finaldepth)) //loop cannot overlap another loop
        return false;
    LOOPSINFO loop;
    ModNameFromAddr(start, loop.mod, true);
    loop.start = start - modbase;
    loop.end = end - modbase;
    loop.depth = finaldepth;
    if(finaldepth)
        loopget(finaldepth - 1, start, &loop.parent, 0);
    else
        loop.parent = 0;
    loop.manual = manual;
    CriticalSectionLocker locker(LockLoops);
    loops.insert(std::make_pair(DepthModuleRange(finaldepth, ModuleRange(ModHashFromAddr(modbase), Range(loop.start, loop.end))), loop));
    return true;
}

//get the start/end of a loop at a certain depth and addr
bool loopget(int depth, uint addr, uint* start, uint* end)
{
    if(!DbgIsDebugging())
        return false;
    const uint modbase = ModBaseFromAddr(addr);
    CriticalSectionLocker locker(LockLoops);
    LoopsInfo::iterator found = loops.find(DepthModuleRange(depth, ModuleRange(ModHashFromAddr(modbase), Range(addr - modbase, addr - modbase))));
    if(found == loops.end()) //not found
        return false;
    if(start)
        *start = found->second.start + modbase;
    if(end)
        *end = found->second.end + modbase;
    return true;
}

//check if a loop overlaps a range, inside is not overlapping
bool loopoverlaps(int depth, uint start, uint end, int* finaldepth)
{
    if(!DbgIsDebugging())
        return false;

    const uint modbase = ModBaseFromAddr(start);
    uint curStart = start - modbase;
    uint curEnd = end - modbase;
    const uint key = ModHashFromAddr(modbase);

    CriticalSectionLocker locker(LockLoops);

    //check if the new loop fits in the old loop
    for(LoopsInfo::iterator i = loops.begin(); i != loops.end(); ++i)
    {
        if(i->first.second.first != key) //only look in the current module
            continue;
        LOOPSINFO* curLoop = &i->second;
        if(curLoop->start < curStart and curLoop->end > curEnd and curLoop->depth == depth)
            return loopoverlaps(depth + 1, curStart, curEnd, finaldepth);
    }

    if(finaldepth)
        *finaldepth = depth;

    //check for loop overlaps
    for(LoopsInfo::iterator i = loops.begin(); i != loops.end(); ++i)
    {
        if(i->first.second.first != key) //only look in the current module
            continue;
        LOOPSINFO* curLoop = &i->second;
        if(curLoop->start <= curEnd and curLoop->end >= curStart and curLoop->depth == depth)
            return true;
    }
    return false;
}

//this should delete a loop and all sub-loops that matches a certain addr
bool loopdel(int depth, uint addr)
{
    return false;
}

void loopcachesave(JSON root)
{
    CriticalSectionLocker locker(LockLoops);
    const JSON jsonloops = json_array();
    const JSON jsonautoloops = json_array();
    for(LoopsInfo::iterator i = loops.begin(); i != loops.end(); ++i)
    {
        const LOOPSINFO curLoop = i->second;
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
        json_object_set(root, "loops", jsonloops);
    json_decref(jsonloops);
    if(json_array_size(jsonautoloops))
        json_object_set(root, "autoloops", jsonautoloops);
    json_decref(jsonautoloops);
}

void loopcacheload(JSON root)
{
    CriticalSectionLocker locker(LockLoops);
    loops.clear();
    const JSON jsonloops = json_object_get(root, "loops");
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
    JSON jsonautoloops = json_object_get(root, "autoloops");
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

bool loopenum(LOOPSINFO* looplist, size_t* cbsize)
{
    // If looplist or size is not requested, fail
    if(!looplist && !cbsize)
        return false;

    SHARED_ACQUIRE(LockLoops);

    // See if the caller requested an output size
    if(cbsize)
    {
        *cbsize = loops.size() * sizeof(LOOPSINFO);

        if(!looplist)
            return true;
    }

    for(auto itr = loops.begin(); itr != loops.end(); itr++)
    {
        *looplist = itr->second;

        // Adjust the offset to a real virtual address
        uint modbase    = ModBaseFromName(looplist->mod);
        looplist->start += modbase;
        looplist->end   += modbase;

        looplist++;
    }

    return true;
}

void loopclear()
{
    EXCLUSIVE_ACQUIRE(LockLoops);
    loops.clear();
    EXCLUSIVE_RELEASE();
}