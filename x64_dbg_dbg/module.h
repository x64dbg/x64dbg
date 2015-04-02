#ifndef _MODULE_H
#define _MODULE_H

#include "_global.h"
#include "addrinfo.h"

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

#endif //_MODULE_H