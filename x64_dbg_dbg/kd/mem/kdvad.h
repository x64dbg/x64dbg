#pragma once

typedef enum MI_VAD_TYPE
{
	VadNone,
	VadDevicePhysicalMemory,
	VadImageMap,
	VadAwe,
	VadWriteWatch,
	VadLargePages,
	VadRotatePhysical,
	VadLargePageSection,
} MI_VAD_TYPE, *PMI_VAD_TYPE;

typedef unsigned char *PMMVAD;

bool	KdVadLoadSymbols		();
ULONG	KdVadGetNodeSize		();
PMMVAD	KdVadAllocNode			(ULONG64 *Address);
void	KdVadGetNodes			(PMMVAD Vad, ULONG64 *Left, ULONG64 *Right);
void	KdVadGetVpns			(PMMVAD Vad, ULONG *Start, ULONG *End);
void	KdVadGetFlags			(PMMVAD Vad, ULONG *Protection, ULONG *State, ULONG *Type);
ULONG64 KdVadForProcessAddress	(ULONG64 EProcess, ULONG64 Address);
bool	KdVadEnumerateProcess	(ULONG64 EProcess, KdEnumCallback Callback);
bool	KdVadRecursiveWalk		(ULONG64 Node, KdEnumCallback Callback);