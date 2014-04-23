#pragma once

typedef bool (* KdEnumCallback)(ULONG64 Entry);
typedef bool (* KdListEnumCallback)(ULONG64 Entry, PVOID UserData);

ULONG64 NtKdCurrrentProcess			();
ULONG64 NtKdCurrentThread			();
ULONG64 NtKdKernelBase				();
bool	NtKdEnumerateDrivers		(KdEnumCallback Callback);
bool	NtKdEnumerateProcesses		(KdEnumCallback Callback);
bool	NtKdEnumerateProcessThreads	(ULONG64 EProcess, KdEnumCallback Callback);
bool	NtKdEnumerateProcessVads	(ULONG64 EProcess, KdEnumCallback Callback);