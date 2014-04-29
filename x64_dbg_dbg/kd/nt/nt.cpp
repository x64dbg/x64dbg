#include "../stdafx.h"

/*--

Module Name:
nt.cpp

Purpose:
Enumeration and data functions based directly off of reading kernel memory.
Does not rely on any caching and should always be up to date.

--*/

ULONG64 cache = 0;

// See: enum.cpp
bool DriverEnum	(ULONG64 ModuleEntry, PVOID Callback);
bool ProcessEnum(ULONG64 ProcessEntry, PVOID Callback);
bool ThreadEnum	(ULONG64 EThread, PVOID Callback);

ULONG64 NtKdCurrrentProcess()
{
	// Returns the debugger's current EPROCESS
	ULONG64 kprocess = 0;

	if (!SUCCEEDED(KdState.DebugSystemObjects->GetCurrentProcessDataOffset(&kprocess)))
		kprocess = cache;

	cache = kprocess;

	return kprocess - KdFields["_EPROCESS"]["Pcb"];
}

ULONG64 NtKdCurrentThread()
{
	// Returns the debugger's current ETHREAD
	ULONG64 kthread = 0;
	
	if (!SUCCEEDED(KdState.DebugSystemObjects->GetCurrentThreadDataOffset(&kthread)))
		DebugBreak();

	return kthread - KdFields["_ETHREAD"]["Tcb"];
}

ULONG64 NtKdKernelBase()
{
	// Returns the base of NTOSKRNL
	return KdDebuggerData.KernBase;
}

bool NtKdEnumerateDrivers(KdEnumCallback Callback)
{
	return KdWalkListEntry(KdSymbols["nt"]["PsLoadedModuleList"], Callback, DriverEnum);
}

bool NtKdEnumerateProcesses(KdEnumCallback Callback)
{
	return KdWalkListEntry(KdSymbols["nt"]["PsActiveProcessHead"], Callback, ProcessEnum);
}

bool NtKdEnumerateProcessThreads(ULONG64 EProcess, KdEnumCallback Callback)
{
	return KdWalkListEntry(EProcess + KdFields["_EPROCESS"]["ThreadListHead"], Callback, ThreadEnum);
}

bool NtKdEnumerateProcessVads(ULONG64 EProcess, KdEnumCallback Callback)
{
	return KdVadEnumerateProcess(EProcess, Callback);
}