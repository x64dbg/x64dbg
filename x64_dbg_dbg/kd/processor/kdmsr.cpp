#include "../../console.h"
#include "stdafx.h"

/*--

Module Name:
kdmsr.cpp

Purpose:
Reading and setting Model Specific Registers.

--*/

ULONG64 KdMsrRead(ULONG Msr)
{
	//
	// Does not work as intended (wrong processor MSR)
	//
	dprintf("The getting of MSRs manually is not available\n");
	return 0;

	ULONG64 val;

	if (!SUCCEEDED(KdState.DebugDataSpaces2->ReadMsr(Msr, &val)))
		return 0;

	return val;
}

bool KdMsrSet(ULONG Msr, ULONG64 Value)
{
	//
	// Does not work as intended (wrong processor MSR)
	//
	dprintf("The setting of MSRs manually is not available\n");
	return false;

	if (!SUCCEEDED(KdState.DebugDataSpaces2->WriteMsr(Msr, Value)))
		return false;

	return true;
}