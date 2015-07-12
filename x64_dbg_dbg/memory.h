#pragma once

#include "_global.h"
#include "addrinfo.h"

extern std::map<Range, MEMPAGE, RangeCompare> memoryPages;
extern bool bListAllPages;

void MemUpdateMap();
uint MemFindBaseAddr(uint Address, uint* Size, bool Refresh = false);
bool MemRead(uint BaseAddress, void* Buffer, SIZE_T Size, SIZE_T* NumberOfBytesRead);
bool MemWrite(uint BaseAddress, const void* Buffer, SIZE_T Size, SIZE_T* NumberOfBytesWritten);
bool MemPatch(uint BaseAddress, const void* Buffer, SIZE_T Size, SIZE_T* NumberOfBytesWritten);
bool MemIsValidReadPtr(uint Address);
bool MemIsCanonicalAddress(uint Address);
uint MemAllocRemote(uint Address, uint Size, DWORD Type, DWORD Protect);
bool MemFreeRemote(uint Address);