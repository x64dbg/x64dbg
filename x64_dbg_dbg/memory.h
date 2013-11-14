#ifndef _MEMORY_H
#define _MEMORY_H

#include "_global.h"

#define PAGE_SIZE 0x1000

uint memfindbaseaddr(HANDLE hProcess, uint addr, uint* size);
bool memread(HANDLE hProcess, const void* lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead);
void* memalloc(HANDLE hProcess, uint addr, DWORD size, DWORD fdProtect);

#endif // _MEMORY_H
