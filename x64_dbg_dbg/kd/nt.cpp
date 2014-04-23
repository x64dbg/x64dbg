#include "stdafx.h"

/*--

Module Name:
nt.cpp

Purpose:
Enumeration and data functions based directly off of reading kernel memory.
Does not rely on any caching and should always be up to date.

--*/

ULONG64 cache = 0;

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
	auto Enum = [](ULONG64 ModuleEntry, PVOID Callback)
	{
		// Adjust to the CONTAINING_RECORD (base of struct)
		ModuleEntry -= KdFields["_LDR_DATA_TABLE_ENTRY"]["InLoadOrderLinks"];

		// _LDR_DATA_TABLE_ENTRY
		return ((KdEnumCallback)Callback)(ModuleEntry);
	};

	return KdWalkListEntry(KdSymbols["nt"]["PsLoadedModuleList"], Callback, Enum);
}

bool NtKdEnumerateProcesses(KdEnumCallback Callback)
{
	auto Enum = [](ULONG64 ProcessEntry, PVOID Callback)
	{
		// Adjust to the CONTAINING_RECORD (base of struct)
		ProcessEntry -= KdFields["_EPROCESS"]["ActiveProcessLinks"];

		// _EPROCESS
		return ((KdEnumCallback)Callback)(ProcessEntry);
	};

	return KdWalkListEntry(KdSymbols["nt"]["PsActiveProcessHead"], Callback, Enum);
}

bool NtKdEnumerateProcessThreads(ULONG64 EProcess, KdEnumCallback Callback)
{
	auto Enum = [](ULONG64 EThread, PVOID Callback)
	{
		// Adjust to the CONTAINING_RECORD (base of struct)
		EThread -= KdFields["_ETHREAD"]["ThreadListEntry"];

		// _ETHREAD
		return ((KdEnumCallback)Callback)(EThread);
	};

	return KdWalkListEntry(EProcess + KdFields["_EPROCESS"]["ThreadListHead"], Callback, Enum);
}

bool NtKdEnumerateProcessVads(ULONG64 EProcess, KdEnumCallback Callback)
{
	return KdVadEnumerateProcess(EProcess, Callback);
}