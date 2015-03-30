#pragma once

#include "_global.h"
#include "addrinfo.h"

typedef std::map<Range, MEMPAGE, RangeCompare> MemoryMap;

extern MemoryMap memoryPages;
extern bool bListAllPages;

void MemUpdateMap(HANDLE hProcess);
uint MemFindBaseAddr(uint addr, uint* Size, bool refresh = false);
bool MemRead(void* BaseAddress, void* Buffer, SIZE_T Size, SIZE_T* NumberOfBytesRead);
bool MemWrite(void* BaseAddress, void* Buffer, SIZE_T Size, SIZE_T* NumberOfBytesWritten);
bool MemPatch(void* BaseAddress, void* Buffer, SIZE_T Size, SIZE_T* NumberOfBytesWritten);
bool MemIsValidReadPtr(uint Address);
bool MemIsCanonicalAddress(uint Address);
void* MemAllocRemote(uint Address, SIZE_T Size, DWORD Protect);
void MemFreeRemote(uint Address);