#include "../stdafx.h"

/*--

Module Name:
kdkprcb.cpp

Purpose:
Modifying and reading from the Kernel Processor Control Region Block [KPRCB]
structures in KiProcessorBlock.

--*/

bool KdKprcbLoadSymbols()
{
#define MAKEFIELD(base, sym)	static ULONG __z ##sym; if (KdFindField(#base, #sym, &__z ##sym)) KdFields.Set(#base, #sym, __z ##sym); else return false;

	MAKEFIELD(_KPRCB, MinorVersion);	// WORD
	MAKEFIELD(_KPRCB, MajorVersion);	// WORD
	MAKEFIELD(_KPRCB, Context);			// PCONTEXT

#undef MAKEFIELD

	return true;
}

ULONG64 KdKprcbForProcessor(ULONG Processor)
{
	// Get the pointer from KiProcessorBlock
	ULONG64 cpuBlockPtr = KdSymbols["nt"]["KiProcessorBlock"] + (Processor * sizeof(PVOID));
	ULONG64 cpuKPRCB	= 0;
	
	if (!KdMemRead(cpuBlockPtr, &cpuKPRCB, sizeof(cpuKPRCB), nullptr))
		return 0;

	return cpuKPRCB;
}

bool KdKprcbContextGet(CONTEXT *Context, ULONG Processor)
{
	// Clear any previous data
	memset(Context, 0, sizeof(CONTEXT));

	ULONG64 kprcb		= KdKprcbForProcessor(Processor);
	ULONG64 contextPtr	= 0;

	if (!kprcb)
		return false;

	// Increment pointer to the offset
	kprcb += KdFields["_KPRCB"]["Context"];

	// It's a pointer to CONTEXT
	if (!KdMemRead(kprcb, &contextPtr, sizeof(contextPtr), nullptr))
		return false;

	// Read all of the data
	return KdMemRead(contextPtr, Context, sizeof(CONTEXT), nullptr);
}

bool KdKprcbContextSet(CONTEXT *Context, ULONG Processor)
{
	ULONG64 kprcb		= KdKprcbForProcessor(Processor);
	ULONG64 contextPtr	= 0;

	if (!kprcb)
		return false;

	// Increment pointer to the offset
	kprcb += KdFields["_KPRCB"]["Context"];

	// It's a pointer to CONTEXT
	if (!KdMemRead(kprcb, &contextPtr, sizeof(contextPtr), nullptr))
		return false;

	// Write all of the data
	return KdMemWrite(contextPtr, Context, sizeof(CONTEXT), nullptr);
}