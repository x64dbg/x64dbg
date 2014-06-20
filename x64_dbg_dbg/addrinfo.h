#ifndef _ADDRINFO_H
#define _ADDRINFO_H

#include "_global.h"

//ranges
typedef std::pair<uint, uint> Range;

struct RangeCompare
{
    bool operator()(const Range& a, const Range& b) //a before b?
    {
        return a.second < b.first;
    }
};

struct OverlappingRangeCompare
{
    bool operator()(const Range& a, const Range& b) //a before b?
    {
        return a.second < b.first || a.second < b.second;
    }
};

//structures
struct MODINFO
{
    uint base; //module base
    uint size; //module size
    uint hash; //full module name hash
    char name[MAX_MODULE_SIZE]; //module name (without extension)
    char extension[MAX_MODULE_SIZE]; //file extension
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
    uint modbase;
    uint start;
    uint end;
    bool manual;
};

struct LOOPSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint modbase;
    uint start;
    uint end;
    int parent;
    int depth;
    bool manual;
};

//typedefs
typedef void (*EXPORTENUMCALLBACK)(uint base, const char* mod, const char* name, uint addr);

typedef std::vector<FUNCTIONSINFO> FunctionsInfo;
typedef std::vector<LOOPSINFO> LoopsInfo;

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

bool apienumexports(uint base, EXPORTENUMCALLBACK cbEnum);

bool commentset(uint addr, const char* text, bool manual);
bool commentget(uint addr, char* text);
bool commentdel(uint addr);
void commentcachesave(JSON root);
void commentcacheload(JSON root);

bool labelset(uint addr, const char* text, bool manual);
bool labelfromstring(const char* text, uint* addr);
bool labelget(uint addr, char* text);
bool labeldel(uint addr);
void labelcachesave(JSON root);
void labelcacheload(JSON root);

bool bookmarkset(uint addr, bool manual);
bool bookmarkget(uint addr);
bool bookmarkdel(uint addr);
void bookmarkcachesave(JSON root);
void bookmarkcacheload(JSON root);


bool functionadd(uint start, uint end, bool manual);
bool functionget(uint addr, uint* start, uint* end);
bool functionoverlaps(uint start, uint end);
bool functiondel(uint addr);

bool loopadd(uint start, uint end, bool manual);
bool loopget(int depth, uint addr, uint* start, uint* end);
bool loopoverlaps(int depth, uint start, uint end, int* finaldepth);
bool loopdel(int depth, uint addr);

#endif // _ADDRINFO_H
