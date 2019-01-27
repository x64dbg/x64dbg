#ifndef _ADDRINFO_H
#define _ADDRINFO_H

#include "_global.h"
#include <functional>
#include <map>
#include <unordered_map>

//ranges
typedef std::pair<duint, duint> Range;
typedef std::pair<duint, Range> ModuleRange; //modhash + RVA range
typedef std::pair<int, ModuleRange> DepthModuleRange; //depth + modulerange

struct RangeCompare
{
    bool operator()(const Range & a, const Range & b) const //a before b?
    {
        return a.second < b.first;
    }
};

struct OverlappingRangeCompare
{
    bool operator()(const Range & a, const Range & b) const //a before b?
    {
        return a.second < b.first || a.second < b.second;
    }
};

struct ModuleRangeCompare
{
    bool operator()(const ModuleRange & a, const ModuleRange & b) const
    {
        if(a.first != b.first)
            return a.first < b.first; //module hash is smaller
        return a.second.second < b.second.first; //a.second is before b.second
    }
};

struct DepthModuleRangeCompare
{
    bool operator()(const DepthModuleRange & a, const DepthModuleRange & b) const
    {
        if(a.first != b.first)
            return a.first < b.first; //module depth is smaller
        if(a.second.first != b.second.first)
            return a.second.first < b.second.first; //module hash is smaller
        return a.second.second.second < b.second.second.first; //range is smaller
    }
};

#include "module.h"
#include <rapidjson/document.h>

struct AddrInfo
{
    duint modhash;
    duint addr;
    bool manual;

    std::string mod() const
    {
        return ModNameFromHash(modhash);
    }
};

//typedefs
typedef std::function<void (duint base, const char* mod, const char* name, duint addr)> EXPORTENUMCALLBACK;
typedef std::function<void (duint base, duint addr, const char* name, const char* moduleName)> IMPORTENUMCALLBACK;

bool apienumexports(duint base, const EXPORTENUMCALLBACK & cbEnum);
bool apienumimports(duint base, const IMPORTENUMCALLBACK & cbEnum);

#endif // _ADDRINFO_H
