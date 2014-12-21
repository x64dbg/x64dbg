#ifndef _MEMORY_H
#define _MEMORY_H

#include "_global.h"
#include "addrinfo.h"

typedef std::map<Range, MEMPAGE, RangeCompare> MemoryMap;

extern MemoryMap memoryPages;
extern bool bListAllPages;

struct PATTERNNIBBLE
{
    unsigned char n;
    bool all;
};

struct PATTERNBYTE
{
    PATTERNNIBBLE n[2];
};

void memupdatemap(HANDLE hProcess);
uint memfindbaseaddr(uint addr, uint* size, bool refresh = false);
bool memread(HANDLE hProcess, const void* lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead);
bool memwrite(HANDLE hProcess, void* lpBaseAddress, const void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten);
bool mempatch(HANDLE hProcess, void* lpBaseAddress, const void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten);
bool memisvalidreadptr(HANDLE hProcess, uint addr);
void* memalloc(HANDLE hProcess, uint addr, SIZE_T size, DWORD fdProtect);
void memfree(HANDLE hProcess, uint addr);
uint memfindpattern(unsigned char* data, uint size, const char* pattern, int* patternsize = 0);

#endif // _MEMORY_H
