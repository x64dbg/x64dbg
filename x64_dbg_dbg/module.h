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