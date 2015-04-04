#pragma once

#include "_global.h"
#include "addrinfo.h"

extern std::map<Range, MEMPAGE, RangeCompare> memoryPages;
extern bool bListAllPages;

void MemUpdateMap(HANDLE hProcess);
uint MemFindBaseAddr(uint Address, uint* Size, bool Refresh = false);
bool MemRead(void* BaseAddress, void* Buffer, SIZE_T Size, SIZE_T* NumberOfBytesRead);
bool MemWrite(void* BaseAddress, void* Buffer, SIZE_T Size, SIZE_T* NumberOfBytesWritten);
bool MemPatch(void* BaseAddress, void* Buffer, SIZE_T Size, SIZE_T* NumberOfBytesWritten);
bool MemIsValidReadPtr(uint Address);
bool MemIsCanonicalAddress(uint Address);
void* MemAllocRemote(uint Address, SIZE_T Size, DWORD Protect);
void MemFreeRemote(uint Address);