#ifndef _ADDRINFO_H
#define _ADDRINFO_H

#include "_global.h"

//superglobal variables
extern sqlite3* userdb;

//typedefs
typedef void (*EXPORTENUMCALLBACK)(uint base, const char* mod, const char* name, uint addr);

//structures
struct MODINFO
{
    uint base;
    uint size;
    char name[MAX_MODULE_SIZE];
    char extension[MAX_MODULE_SIZE];
};

void dbinit();
bool dbsave();
bool dbload();
void dbclose();
bool modload(uint base, uint size, const char* fullpath);
bool modunload(uint base);
void modclear();
bool modnamefromaddr(uint addr, char* modname, bool extension);
uint modbasefromaddr(uint addr);
uint modbasefromname(const char* modname);
bool apienumexports(uint base, EXPORTENUMCALLBACK cbEnum);
bool commentset(uint addr, const char* text);
bool commentget(uint addr, char* text);
bool commentdel(uint addr);
bool labelset(uint addr, const char* text);
bool labelfromstring(const char* text, uint* addr);
bool labelget(uint addr, char* text);
bool labeldel(uint addr);
bool bookmarkset(uint addr);
bool bookmarkget(uint addr);
bool bookmarkdel(uint addr);
bool functionget(duint addr, duint* start, duint* end);
bool functionoverlaps(uint start, uint end);
bool functionadd(uint start, uint end, bool manual);
bool functiondel(uint addr);

#endif // _ADDRINFO_H
