#pragma once

bool	KdMemQuery			(ULONG64 Address, MEMORY_BASIC_INFORMATION64 *Information);
ULONG64 KdMemFindBaseAddress(ULONG64 Address, ULONG64 *Size);
bool	KdMemRead			(const ULONG64 BaseAddress, PVOID Buffer, ULONG Size, PSIZE_T NumberOfBytesRead);
ULONG64	KdMemReadPointer	(const ULONG64 Address, bool *Success);
bool	KdMemWrite			(const ULONG64 BaseAddress, PVOID Buffer, ULONG Size, PSIZE_T NumberOfBytesWritten);
bool	KdMemIsReadValid	(ULONG64 Address);
PVOID	KdMemAllocate		(ULONG64 BaseAddress, ULONG Size, DWORD Protect);
void	KdMemFree			(ULONG64 Address);