#include "function.h"
#include "module.h"
#include "debugger.h"
#include "memory.h"
#include "threading.h"

typedef std::map<ModuleRange, FUNCTIONSINFO, ModuleRangeCompare> FunctionsInfo;

static FunctionsInfo functions;

bool functionadd(uint start, uint end, bool manual)
{
    if(!DbgIsDebugging() or end < start or !memisvalidreadptr(fdProcessInfo->hProcess, start))
        return false;
    const uint modbase = ModBaseFromAddr(start);
    if(modbase != ModBaseFromAddr(end)) //the function boundaries are not in the same module
        return false;
    if(functionoverlaps(start, end))
        return false;
    FUNCTIONSINFO function;
    ModNameFromAddr(start, function.mod, true);
    function.start = start - modbase;
    function.end = end - modbase;
    function.manual = manual;
    CriticalSectionLocker locker(LockFunctions);
    functions.insert(std::make_pair(ModuleRange(ModHashFromAddr(modbase), Range(function.start, function.end)), function));
    return true;
}

bool functionget(uint addr, uint* start, uint* end)
{
    if(!DbgIsDebugging())
        return false;
    uint modbase = ModBaseFromAddr(addr);
    CriticalSectionLocker locker(LockFunctions);
    const FunctionsInfo::iterator found = functions.find(ModuleRange(ModHashFromAddr(modbase), Range(addr - modbase, addr - modbase)));
    if(found == functions.end()) //not found
        return false;
    if(start)
        *start = found->second.start + modbase;
    if(end)
        *end = found->second.end + modbase;
    return true;
}

bool functionoverlaps(uint start, uint end)
{
    if(!DbgIsDebugging() or end < start)
        return false;
    const uint modbase = ModBaseFromAddr(start);
    CriticalSectionLocker locker(LockFunctions);
    return (functions.count(ModuleRange(ModHashFromAddr(modbase), Range(start - modbase, end - modbase))) > 0);
}

bool functiondel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    const uint modbase = ModBaseFromAddr(addr);
    CriticalSectionLocker locker(LockFunctions);
    return (functions.erase(ModuleRange(ModHashFromAddr(modbase), Range(addr - modbase, addr - modbase))) > 0);
}

void functiondelrange(uint start, uint end)
{
    if(!DbgIsDebugging())
        return;
    bool bDelAll = (start == 0 && end == ~0); //0x00000000-0xFFFFFFFF
    uint modbase = ModBaseFromAddr(start);
    if(modbase != ModBaseFromAddr(end))
        return;
    start -= modbase;
    end -= modbase;
    CriticalSectionLocker locker(LockFunctions);
    FunctionsInfo::iterator i = functions.begin();
    while(i != functions.end())
    {
        if(i->second.manual) //ignore manual
        {
            i++;
            continue;
        }
        if(bDelAll or !(i->second.start <= end and i->second.end >= start))
            functions.erase(i++);
        else
            i++;
    }
}

void functioncachesave(JSON root)
{
    CriticalSectionLocker locker(LockFunctions);
    const JSON jsonfunctions = json_array();
    const JSON jsonautofunctions = json_array();
    for(FunctionsInfo::iterator i = functions.begin(); i != functions.end(); ++i)
    {
        const FUNCTIONSINFO curFunction = i->second;
        JSON curjsonfunction = json_object();
        json_object_set_new(curjsonfunction, "module", json_string(curFunction.mod));
        json_object_set_new(curjsonfunction, "start", json_hex(curFunction.start));
        json_object_set_new(curjsonfunction, "end", json_hex(curFunction.end));
        if(curFunction.manual)
            json_array_append_new(jsonfunctions, curjsonfunction);
        else
            json_array_append_new(jsonautofunctions, curjsonfunction);
    }
    if(json_array_size(jsonfunctions))
        json_object_set(root, "functions", jsonfunctions);
    json_decref(jsonfunctions);
    if(json_array_size(jsonautofunctions))
        json_object_set(root, "autofunctions", jsonautofunctions);
    json_decref(jsonautofunctions);
}

void functioncacheload(JSON root)
{
    CriticalSectionLocker locker(LockFunctions);
    functions.clear();
    const JSON jsonfunctions = json_object_get(root, "functions");
    if(jsonfunctions)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonfunctions, i, value)
        {
            FUNCTIONSINFO curFunction;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(curFunction.mod, mod);
            else
                *curFunction.mod = '\0';
            curFunction.start = (uint)json_hex_value(json_object_get(value, "start"));
            curFunction.end = (uint)json_hex_value(json_object_get(value, "end"));
            if(curFunction.end < curFunction.start)
                continue; //invalid function
            curFunction.manual = true;
            const uint key = ModHashFromName(curFunction.mod);
            functions.insert(std::make_pair(ModuleRange(ModHashFromName(curFunction.mod), Range(curFunction.start, curFunction.end)), curFunction));
        }
    }
    JSON jsonautofunctions = json_object_get(root, "autofunctions");
    if(jsonautofunctions)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautofunctions, i, value)
        {
            FUNCTIONSINFO curFunction;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(curFunction.mod, mod);
            else
                *curFunction.mod = '\0';
            curFunction.start = (uint)json_hex_value(json_object_get(value, "start"));
            curFunction.end = (uint)json_hex_value(json_object_get(value, "end"));
            if(curFunction.end < curFunction.start)
                continue; //invalid function
            curFunction.manual = true;
            const uint key = ModHashFromName(curFunction.mod);
            functions.insert(std::make_pair(ModuleRange(ModHashFromName(curFunction.mod), Range(curFunction.start, curFunction.end)), curFunction));
        }
    }
}

bool functionenum(FUNCTIONSINFO* functionlist, size_t* cbsize)
{
    if(!DbgIsDebugging())
        return false;
    if(!functionlist && !cbsize)
        return false;
    CriticalSectionLocker locker(LockFunctions);
    if(!functionlist && cbsize)
    {
        *cbsize = functions.size() * sizeof(FUNCTIONSINFO);
        return true;
    }
    int j = 0;
    for(FunctionsInfo::iterator i = functions.begin(); i != functions.end(); ++i, j++)
    {
        functionlist[j] = i->second;
        uint modbase = ModBaseFromName(functionlist[j].mod);
        functionlist[j].start += modbase;
        functionlist[j].end += modbase;
    }
    return true;
}

void functionclear()
{
    CriticalSectionLocker locker(LockFunctions);
    FunctionsInfo().swap(functions);
}