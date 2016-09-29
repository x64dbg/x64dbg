#ifndef _MEMORY_H
#define _MEMORY_H

#include "_global.h"
#include "addrinfo.h"
#include "patternfind.h"

extern std::map<Range, MEMPAGE, RangeCompare> memoryPages;
extern bool bListAllPages;
extern DWORD memMapThreadCounter;

struct SimplePage
{
    duint address;
    duint size;

    SimplePage(duint address, duint size)
    {
        this->address = address;
        this->size = size;
    }
};

void MemUpdateMap();
void MemUpdateMapAsync();
duint MemFindBaseAddr(duint Address, duint* Size, bool Refresh = false);
bool MemRead(duint BaseAddress, void* Buffer, duint Size, duint* NumberOfBytesRead = nullptr, bool cache = false);
bool MemReadUnsafe(duint BaseAddress, void* Buffer, duint Size, duint* NumberOfBytesRead = nullptr);
bool MemWrite(duint BaseAddress, const void* Buffer, duint Size, duint* NumberOfBytesWritten = nullptr);
bool MemPatch(duint BaseAddress, const void* Buffer, duint Size, duint* NumberOfBytesWritten = nullptr);
bool MemIsValidReadPtr(duint Address, bool cache = false);
bool MemIsValidReadPtrUnsafe(duint Address, bool cache = false);
bool MemIsCanonicalAddress(duint Address);
bool MemIsCodePage(duint Address, bool Refresh);
duint MemAllocRemote(duint Address, duint Size, DWORD Type = MEM_RESERVE | MEM_COMMIT, DWORD Protect = PAGE_EXECUTE_READWRITE);
bool MemFreeRemote(duint Address);
bool MemGetPageInfo(duint Address, MEMPAGE* PageInfo, bool Refresh = false);
bool MemSetPageRights(duint Address, const char* Rights);
bool MemGetPageRights(duint Address, char* Rights);
bool MemPageRightsToString(DWORD Protect, char* Rights);
bool MemPageRightsFromString(DWORD* Protect, const char* Rights);
bool MemFindInPage(const SimplePage & page, duint startoffset, const std::vector<PatternByte> & pattern, std::vector<duint> & results, duint maxresults);
bool MemFindInMap(const std::vector<SimplePage> & pages, const std::vector<PatternByte> & pattern, std::vector<duint> & results, duint maxresults, bool progress = true);
bool MemDecodePointer(duint* Pointer, bool vistaPlus);

#endif // _MEMORY_H