#include "../console.h"
#include "../memory.h"
#include "stdafx.h"

/*--

Module Name:
kdssdt.cpp

Purpose:
Dumping and enumerating a _KSERVICE_TABLE_DESCRIPTOR, also known
as the System Service Descriptor Table.

Each KeServiceDescriptorTable(Shadow) index can contain one of the following:
ntoskrnl ------> !KiServiceTable
win32k   ------> !W32pServiceTable
IIS      ------> !???
none     ------> nullptr

--*/

_KSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable[4];
_KSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTableShadow[4];

bool KdSSDTUpdate()
{
	// Read the system tables
	// They are possibly modified and need to be updated

	if (KdState.m_DebugOutput)
		dprintf("Updating KeServiceDescriptorTable(Shadow)...\n");

	if (!KdMemRead(KdSymbols["nt"]["KeServiceDescriptorTable"], &KeServiceDescriptorTable, sizeof(KeServiceDescriptorTable), nullptr))
		return false;

	if (!KdMemRead(KdSymbols["nt"]["KeServiceDescriptorTableShadow"], &KeServiceDescriptorTableShadow, sizeof(KeServiceDescriptorTableShadow), nullptr))
		return false;

	return true;
}

ULONG64 KdSSDTGetPointer(ULONG64 TableBase, ULONG64 TableEntry)
{
#ifdef _WIN64
	// SSDT pointers are relative to the base in X64
	return (TableEntry >> 4) + TableBase;
#else
	// Otherwise it's 32-bit and a direct pointer
	return TableEntry;
#endif
}

bool KdSSDTEnumerate(ULONG TableIndex, bool Shadow, bool (* Callback)(ULONG Index, ULONG64 Entry))
{
	// Get the table pointer
	PKSERVICE_TABLE_DESCRIPTOR tablePointer = nullptr;
	
	if (Shadow)
		tablePointer = &KeServiceDescriptorTableShadow[TableIndex];
	else
		tablePointer = &KeServiceDescriptorTable[TableIndex];

	ULONG64 tableBase	= (ULONG64)tablePointer->ServiceTableBase;
	ULONG64 tableCount	= (ULONG64)tablePointer->NumberOfService;

	// Allocate a buffer to read them all at once
	ULONG *buf = (ULONG *)BridgeAlloc(sizeof(ULONG) * tableCount);

	if (!buf)
		return false;

	// Read it
	if (!KdMemRead(tableBase, buf, sizeof(ULONG) * tableCount, nullptr))
		return false;

	for (ULONG i = 0; i < tableCount; i++)
	{
		ULONG64 func = KdSSDTGetPointer(tableBase, buf[i]);

		if (!Callback(i, func))
		{
			BridgeFree(buf);
			return false;
		}
	}

	BridgeFree(buf);
	return true;
}