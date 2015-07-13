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
uint MemAllocRemote(uint Address, uint Size, DWORD Type = MEM_RESERVE | MEM_COMMIT, DWORD Protect = PAGE_EXECUTE_READWRITE);
bool MemFreeRemote(uint Address);