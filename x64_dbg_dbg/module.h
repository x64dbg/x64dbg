#pragma once

#include "_global.h"
#include "addrinfo.h"

struct MODSECTIONINFO
{
    uint addr;      // Virtual address
    uint size;      // Virtual size
    char name[50];  // Escaped section name
};

struct MODINFO
{
    uint base;  // Module base
    uint size;  // Module size
    uint hash;  // Full module name hash
    uint entry; // Entry point

    char name[MAX_MODULE_SIZE];         // Module name (without extension)
    char extension[MAX_MODULE_SIZE];    // File extension
    char path[MAX_PATH];                // File path (in UTF8)

    HANDLE Handle;          // Handle to the file opened by TitanEngine
    HANDLE MapHandle;       // Handle to the memory map
    ULONG_PTR FileMapVA;    // File map virtual address (Debugger local)
    DWORD FileMapSize;      // File map virtual size

    std::vector<MODSECTIONINFO> sections;
};

typedef std::map<Range, MODINFO, RangeCompare> ModulesInfo;

bool ModLoad(uint base, uint size, const char* fullpath);
bool ModUnload(uint base);
void ModClear();
MODINFO* ModInfoFromAddr(uint addr);
bool ModNameFromAddr(uint addr, char* modname, bool extension);
uint ModBaseFromAddr(uint addr);
uint ModHashFromAddr(uint addr);
uint ModHashFromName(const char* mod);
uint ModBaseFromName(const char* modname);
uint ModSizeFromAddr(uint addr);
bool ModSectionsFromAddr(uint addr, std::vector<MODSECTIONINFO>* sections);
uint ModEntryFromAddr(uint addr);
int ModPathFromAddr(duint addr, char* path, int size);
int ModPathFromName(const char* modname, char* path, int size);