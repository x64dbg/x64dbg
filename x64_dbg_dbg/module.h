#pragma once

#include "_global.h"

struct MODSECTIONINFO
{
    uint addr;      // Virtual address
    uint size;      // Virtual size
    char name[MAX_SECTION_SIZE * 5];  // Escaped section name
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

    std::vector<MODSECTIONINFO> sections;
};

bool ModLoad(uint Base, uint Size, const char* FullPath);
bool ModUnload(uint Base);
void ModClear();
MODINFO* ModInfoFromAddr(uint Address);
bool ModNameFromAddr(uint Address, char* Name, bool Extension);
uint ModBaseFromAddr(uint Address);
uint ModHashFromAddr(uint Address);
uint ModHashFromName(const char* Module);
uint ModBaseFromName(const char* Module);
uint ModSizeFromAddr(uint Address);
bool ModSectionsFromAddr(uint Address, std::vector<MODSECTIONINFO>* Sections);
uint ModEntryFromAddr(uint Address);
int ModPathFromAddr(duint Address, char* Path, int Size);
int ModPathFromName(const char* Module, char* Path, int Size);