#ifndef _MEMORY_H
#define _MEMORY_H

#include "_global.h"

#ifndef KDEBUGGER_ENABLE
#define PAGE_SIZE 0x1000
#endif // ndef KDEBUGGER_ENABLE

uint memfindbaseaddr(HANDLE hProcess, uint addr, uint* size);
bool memread(HANDLE hProcess, const void* lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead);
bool memwrite(HANDLE hProcess, void* lpBaseAddress, const void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten);
bool memisvalidreadptr(HANDLE hProcess, uint addr);
void* memalloc(HANDLE hProcess, uint addr, DWORD size, DWORD fdProtect);
void memfree(HANDLE hProcess, uint addr);

#endif // _MEMORY_H
