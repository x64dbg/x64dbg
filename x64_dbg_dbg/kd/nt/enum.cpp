#include "../stdafx.h"

/*--

Module Name:
enum.cpp

Purpose:
Enumeration helper function for nt.cpp

--*/

bool DriverEnum(ULONG64 ModuleEntry, PVOID Callback)
{
	// Adjust to the CONTAINING_RECORD (base of struct)
	ModuleEntry -= KdFields["_LDR_DATA_TABLE_ENTRY"]["InLoadOrderLinks"];

	// _LDR_DATA_TABLE_ENTRY
	return ((KdEnumCallback) Callback)(ModuleEntry);
}

bool ProcessEnum(ULONG64 ProcessEntry, PVOID Callback)
{
	// Adjust to the CONTAINING_RECORD (base of struct)
	ProcessEntry -= KdFields["_EPROCESS"]["ActiveProcessLinks"];

	// _EPROCESS
	return ((KdEnumCallback) Callback)(ProcessEntry);
}

bool ThreadEnum(ULONG64 EThread, PVOID Callback)
{
	// Adjust to the CONTAINING_RECORD (base of struct)
	EThread -= KdFields["_ETHREAD"]["ThreadListEntry"];

	// _ETHREAD
	return ((KdEnumCallback) Callback)(EThread);
}