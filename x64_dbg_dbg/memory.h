#pragma once

#include "_global.h"
#include "addrinfo.h"

extern std::map<Range, MEMPAGE, RangeCompare> memoryPages;
extern bool bListAllPages;

void MemUpdateMap();
uint MemFindBaseAddr(uint Address, uint* Size, bool Refresh = false);
bool MemRead(uint BaseAddress, void* Buffer, uint Size, uint* NumberOfBytesRead = nullptr);
bool MemWrite(uint BaseAddress, const void* Buffer, uint Size, uint* NumberOfBytesWritten = nullptr);
bool MemPatch(uint BaseAddress, const void* Buffer, uint Size, uint* NumberOfBytesWritten = nullptr);
bool MemIsValidReadPtr(uint Address);
bool MemIsCanonicalAddress(uint Address);
bool MemIsCodePage(uint Address, bool Refresh);
uint MemAllocRemote(uint Address, uint Size, DWORD Type = MEM_RESERVE | MEM_COMMIT, DWORD Protect = PAGE_EXECUTE_READWRITE);
bool MemFreeRemote(uint Address);
uint MemGetPageAligned(uint Address);
bool MemGetPageInfo(uint Address, MEMPAGE* PageInfo, bool Refresh = false);
bool MemSetPageRights(uint Address, const char* Rights);
bool MemGetPageRights(uint Address, char* Rights);
bool MemPageRightsToString(DWORD Protect, char* Rights);
bool MemPageRightsFromString(DWORD* Protect, const char* Rights);