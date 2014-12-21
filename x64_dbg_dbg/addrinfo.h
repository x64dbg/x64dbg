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

//structures
struct MODSECTIONINFO
{
    uint addr; //va
    uint size; //virtual size
    char name[50];
};

struct MODINFO
{
    uint base; //module base
    uint size; //module size
    uint hash; //full module name hash
    uint entry; //entry point
    char name[MAX_MODULE_SIZE]; //module name (without extension)
    char extension[MAX_MODULE_SIZE]; //file extension
    std::vector<MODSECTIONINFO> sections;
};
typedef std::map<Range, MODINFO, RangeCompare> ModulesInfo;

struct COMMENTSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    char text[MAX_COMMENT_SIZE];
    bool manual;
};
typedef std::map<uint, COMMENTSINFO> CommentsInfo;

struct LABELSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    char text[MAX_LABEL_SIZE];
    bool manual;
};
typedef std::map<uint, LABELSINFO> LabelsInfo;

struct BOOKMARKSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    bool manual;
};
typedef std::map<uint, BOOKMARKSINFO> BookmarksInfo;

struct FUNCTIONSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint start;
    uint end;
    bool manual;
};
typedef std::map<ModuleRange, FUNCTIONSINFO, ModuleRangeCompare> FunctionsInfo;

struct LOOPSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint start;
    uint end;
    uint parent;
    int depth;
    bool manual;
};
typedef std::map<DepthModuleRange, LOOPSINFO, DepthModuleRangeCompare> LoopsInfo;

//typedefs
typedef void (*EXPORTENUMCALLBACK)(uint base, const char* mod, const char* name, uint addr);

void dbsave();
void dbload();
void dbclose();

bool modload(uint base, uint size, const char* fullpath);
bool modunload(uint base);
void modclear();
bool modnamefromaddr(uint addr, char* modname, bool extension);
uint modbasefromaddr(uint addr);
uint modhashfromva(uint va);
uint modhashfromname(const char* mod);
uint modbasefromname(const char* modname);
uint modsizefromaddr(uint addr);
bool modsectionsfromaddr(uint addr, std::vector<MODSECTIONINFO>* sections);
uint modentryfromaddr(uint addr);
int modpathfromaddr(duint addr, char* path, int size);
int modpathfromname(const char* modname, char* path, int size);

bool apienumexports(uint base, EXPORTENUMCALLBACK cbEnum);

bool commentset(uint addr, const char* text, bool manual);
bool commentget(uint addr, char* text);
bool commentdel(uint addr);
void commentdelrange(uint start, uint end);
void commentcachesave(JSON root);
void commentcacheload(JSON root);
bool commentenum(COMMENTSINFO* commentlist, size_t* cbsize);

bool labelset(uint addr, const char* text, bool manual);
bool labelfromstring(const char* text, uint* addr);
bool labelget(uint addr, char* text);
bool labeldel(uint addr);
void labeldelrange(uint start, uint end);
void labelcachesave(JSON root);
void labelcacheload(JSON root);
bool labelenum(LABELSINFO* labellist, size_t* cbsize);

bool bookmarkset(uint addr, bool manual);
bool bookmarkget(uint addr);
bool bookmarkdel(uint addr);
void bookmarkdelrange(uint start, uint end);
void bookmarkcachesave(JSON root);
void bookmarkcacheload(JSON root);
bool bookmarkenum(BOOKMARKSINFO* bookmarklist, size_t* cbsize);

bool functionadd(uint start, uint end, bool manual);
bool functionget(uint addr, uint* start, uint* end);
bool functionoverlaps(uint start, uint end);
bool functiondel(uint addr);
void functiondelrange(uint start, uint end);
void functioncachesave(JSON root);
void functioncacheload(JSON root);
bool functionenum(FUNCTIONSINFO* functionlist, size_t* cbsize);

bool loopadd(uint start, uint end, bool manual);
bool loopget(int depth, uint addr, uint* start, uint* end);
bool loopoverlaps(int depth, uint start, uint end, int* finaldepth);
bool loopdel(int depth, uint addr);
void loopcachesave(JSON root);
void loopcacheload(JSON root);
bool loopenum(LOOPSINFO* looplist, size_t* cbsize);

#endif // _ADDRINFO_H
