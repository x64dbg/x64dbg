#ifndef _ADDRINFO_H
#define _ADDRINFO_H

#include "_global.h"

//ranges
typedef std::pair<uint, uint> Range;
typedef std::pair<uint, Range> ModuleRange; //modhash + RVA range
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
        if(a.first < b.first) //module hash is smaller
            return true;
        if(a.first != b.first) //module hashes are not equal
            return false;
        return a.second.second < b.second.first; //a.second is before b.second
    }
};

struct DepthModuleRangeCompare
{
    bool operator()(const DepthModuleRange & a, const DepthModuleRange & b) const
    {
        if(a.first < b.first) //module depth is smaller
            return true;
        if(a.first != b.first) //module depths are not equal
            return false;
        if(a.second.first < b.second.first) //module hash is smaller
            return true;
        if(a.second.first != b.second.first) //module hashes are not equal
            return false;
        return a.second.second.second < b.second.second.first; //a.second.second is before b.second.second
    }
};

//typedefs
typedef void (*EXPORTENUMCALLBACK)(uint base, const char* mod, const char* name, uint addr);

void dbsave();
void dbload();
void dbclose();

bool apienumexports(uint base, EXPORTENUMCALLBACK cbEnum);

#endif // _ADDRINFO_H
