#ifndef _ADDRINFO_H
#define _ADDRINFO_H

#include "_global.h"

//structures
struct MODINFO
{
    uint base;
    uint size;
    char name[MAX_MODULE_SIZE];
    char extension[MAX_MODULE_SIZE];
};

struct COMMENTSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    char text[MAX_COMMENT_SIZE];
    bool manual;
};

struct LABELSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    char text[MAX_LABEL_SIZE];
    bool manual;
};

struct BOOKMARKSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    bool manual;
};

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

typedef std::vector<MODINFO> ModulesInfo;
typedef std::map<uint, COMMENTSINFO> CommentsInfo;
typedef std::map<uint, LABELSINFO> LabelsInfo;
typedef std::map<uint, BOOKMARKSINFO> BookmarksInfo;
typedef std::vector<FUNCTIONSINFO> FunctionsInfo;
typedef std::vector<LOOPSINFO> LoopsInfo;

void dbsave();
void dbload();
void dbupdate();
void dbclose();

bool modload(uint base, uint size, const char* fullpath);
bool modunload(uint base);
void modclear();
bool modnamefromaddr(uint addr, char* modname, bool extension);
uint modbasefromaddr(uint addr);
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

bool bookmarkset(uint addr, bool manual);
bool bookmarkget(uint addr);
bool bookmarkdel(uint addr);

bool functionadd(uint start, uint end, bool manual);
bool functionget(uint addr, uint* start, uint* end);
bool functionoverlaps(uint start, uint end);
bool functiondel(uint addr);

bool loopadd(uint start, uint end, bool manual);
bool loopget(int depth, uint addr, uint* start, uint* end);
bool loopoverlaps(int depth, uint start, uint end, int* finaldepth);
bool loopdel(int depth, uint addr);

#endif // _ADDRINFO_H
