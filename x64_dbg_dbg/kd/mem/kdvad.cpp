#include "../../memory.h"
#include "../stdafx.h"

/*--

Module Name:
kdvad.cpp

Purpose:
Provides lookups and information on Virtual Address Descriptors used
in each process (See EPROCESS.VadRoot). This doesn't apply to kernel addresses.

--*/

// _MM_AVL_TABLE
ULONG Vad_BalancedRoot;

// MM_AVL_TABLE will contain either:
// _MM_AVL_NODE
// _MMADDRESS_NODE
ULONG Vad_NodeSize;

ULONG Vad_LeftChild;
ULONG Vad_RightChild;
ULONG Vad_StartingVpn;
ULONG Vad_EndingVpn;

ULONG Vad_Union0;
ULONG Vad_Union1;

KdOffsetBitField VadBits_VadType;
KdOffsetBitField VadBits_Protection;
KdOffsetBitField VadBits_PrivateMemory;
KdOffsetBitField VadBits_MemCommit;

bool KdVadLoadSymbols()
{
#define GETBITS(str, mem) KdFindFieldBitType(#str "." #mem, &VadBits_ ##mem.m_Start, &VadBits_ ##mem.m_Size);

	// Try _MMADDRESS_NODE first
	if (KdGetFieldSize("_MMADDRESS_NODE", &Vad_NodeSize))
	{
		if (!KdFindField("_MMADDRESS_NODE", "LeftChild", &Vad_LeftChild))		return false;
		if (!KdFindField("_MMADDRESS_NODE", "RightChild", &Vad_RightChild))		return false;
		if (!KdFindField("_MMADDRESS_NODE", "StartingVpn", &Vad_StartingVpn))	return false;
		if (!KdFindField("_MMADDRESS_NODE", "EndingVpn", &Vad_EndingVpn))		return false;
	}
	else
	{
		// Fall back to _MM_AVL_NODE/_MMVAD_SHORT
		if (!KdGetFieldSize("_MMVAD_SHORT", &Vad_NodeSize))
			return false;

		if (!KdFindField("_MM_AVL_NODE", "LeftChild", &Vad_LeftChild))		return false;
		if (!KdFindField("_MM_AVL_NODE", "RightChild", &Vad_RightChild))	return false;
		if (!KdFindField("_MMVAD_SHORT", "StartingVpn", &Vad_StartingVpn))	return false;
		if (!KdFindField("_MMVAD_SHORT", "EndingVpn", &Vad_EndingVpn))		return false;

		// Adjustment for the different structure
		ULONG vadNodeOffset;
		if (!KdFindField("_MMVAD_SHORT", "VadNode", &vadNodeOffset))		return false;

		Vad_StartingVpn += vadNodeOffset;
		Vad_EndingVpn	+= vadNodeOffset;
	}

	//
	// _MMVAD_SHORT has different versions (See Win7/Win8)
	// Determine if '_MMVAD_SHORT::u1' exists and if so, it's the newer version
	//

	// Should always succeed
	if (!KdFindField("_MMVAD_SHORT", "u", &Vad_Union0))
		return false;

	if (KdFindField("_MMVAD_SHORT", "u1", &Vad_Union1))
	{
		// Newer _MMVAD_SHORT
		GETBITS(_MMVAD_FLAGS, VadType);
		GETBITS(_MMVAD_FLAGS, Protection);
		GETBITS(_MMVAD_FLAGS, PrivateMemory);
		GETBITS(_MMVAD_FLAGS1, MemCommit);
	}
	else
	{
		// Older _MMVAD_SHORT
		GETBITS(_MMVAD_FLAGS, VadType);
		GETBITS(_MMVAD_FLAGS, Protection);
		GETBITS(_MMVAD_FLAGS, MemCommit);
		GETBITS(_MMVAD_FLAGS, PrivateMemory);
	}

#undef GETBITS

	return true;
}

ULONG KdVadGetNodeSize()
{
	return Vad_NodeSize;
}

PMMVAD KdVadAllocNode(ULONG64 *Address)
{
	PMMVAD data = (PMMVAD)BridgeAlloc(KdVadGetNodeSize());

	// If an address is supplied, read the memory for the node
	if (Address)
	{
		if (!KdMemRead(*Address, data, KdVadGetNodeSize(), nullptr))
		{
			BridgeFree(data);
			return nullptr;
		}
	}

	return data;
}

void KdVadGetNodes(PMMVAD Vad, ULONG64 *Left, ULONG64 *Right)
{
#ifdef _WIN64
	if (Left)
		*Left = *(ULONG64 *)(Vad + Vad_LeftChild);

	if (Right)
		*Right = *(ULONG64 *)(Vad + Vad_RightChild);
#else
	if (Left)
		*Left = *(ULONG *)(Vad + Vad_LeftChild);

	if (Right)
		*Right = *(ULONG *)(Vad + Vad_RightChild);
#endif
}

void KdVadGetVpns(PMMVAD Vad, ULONG *Start, ULONG *End)
{
	if (Start)
		*Start = *(ULONG *)(Vad + Vad_StartingVpn);

	if (End)
		*End = *(ULONG *)(Vad + Vad_EndingVpn);
}

void KdVadGetFlags(PMMVAD Vad, ULONG *Protection, ULONG *State, ULONG *Type)
{
	ULONG u		= *(ULONG *)(Vad + Vad_Union0);
	ULONG u1	= Vad_Union1 ? *(ULONG *)(Vad + Vad_Union1) : 0;

	ULONG vadType = VadBits_VadType.GetValue(u);

	if (Protection)
	{
		ULONG protection = VadBits_Protection.GetValue(u);

		*Protection = MmProtectToValue[protection];
	}

	if (State)
	{
		// Use 'u1' only if it is nonzero
		// Otherwise use 'u'
		ULONG memCommit = VadBits_MemCommit.GetValue(u1 ? u1 : u);

		// Temp (TODO)
		*State = memCommit ? MEM_COMMIT : MEM_FREE;
	}

	if (Type)
	{
		ULONG privateMemory = VadBits_PrivateMemory.GetValue(u);

		if (privateMemory || vadType == VadRotatePhysical)
			*Type = MEM_PRIVATE;
		else if (vadType == VadImageMap)
			*Type = MEM_IMAGE;
		else
			*Type = MEM_MAPPED;
	}
}

ULONG64 KdVadForProcessAddress(ULONG64 EProcess, ULONG64 Address)
{
	// Verify the range first
	if (Address < MI_LOWEST_VAD_ADDRESS || Address > MM_HIGHEST_VAD_ADDRESS)
		return 0;

	// Walk the VAD list of the process and find the specific address
	// Get all base offsets first
	ULONG64 vadRoot			= EProcess + KdFields["_EPROCESS"]["VadRoot"];
	ULONG64 balancedRoot	= vadRoot + Vad_BalancedRoot;

	// PAGE_SHIFT to get the VPN
	Address = MI_VA_TO_VPN(Address);

	// NOTE:
	// VadRoot isn't an actual VAD node, just the list start
	PMMVAD vad = KdVadAllocNode(nullptr);

	for (ULONG64 node = balancedRoot; node;)
	{
		if (!KdMemRead(node, vad, KdVadGetNodeSize(), nullptr))
			break;

		ULONG64 leftChild;
		ULONG64 rightChild;
		KdVadGetNodes(vad, &leftChild, &rightChild);

		ULONG vpnStart;
		ULONG vpnEnd;
		KdVadGetVpns(vad, &vpnStart, &vpnEnd);

		// First check if it's within range
		if (Address >= vpnStart && Address <= vpnEnd)
		{
			BridgeFree(vad);
			return node;
		}

		// If it's less than, use the left side
		if (Address < vpnStart)
			node = leftChild;

		// If it's more than, use the right side
		if (Address > vpnStart)
			node = rightChild;
	}

	BridgeFree(vad);
	return 0;
}

bool KdVadEnumerateProcess(ULONG64 EProcess, KdEnumCallback Callback)
{
	// Walk the VAD list of the process going both left and right
	// Get all base offsets first
	ULONG64 vadRoot			= EProcess + KdFields["_EPROCESS"]["VadRoot"];
	ULONG64 balancedRoot	= vadRoot + Vad_BalancedRoot;

	// NOTE:
	// VadRoot isn't an actual VAD node, just the list start
	PMMVAD vad = KdVadAllocNode(&balancedRoot);

	if (!vad)
		return false;

	ULONG64 leftNode;
	ULONG64 rightNode;
	KdVadGetNodes(vad, &leftNode, &rightNode);

	// Free the data
	BridgeFree(vad);

	// First left node to be checked
	if (!KdVadRecursiveWalk(leftNode, Callback))
		return false;

	// First right node to be checked
	if (!KdVadRecursiveWalk(rightNode, Callback))
		return false;

	return true;
}

bool KdVadRecursiveWalk(ULONG64 Node, KdEnumCallback Callback)
{
	// Node == 0 indicates the end of the list
	if (!Node)
		return true;

	// Execute the callback first
	if (!Callback(Node))
		return false;

	// Move on to the next node and read it
	PMMVAD vad = KdVadAllocNode(&Node);

	if (!vad)
		return false;

	ULONG64 leftChild;
	ULONG64 rightChild;
	KdVadGetNodes(vad, &leftChild, &rightChild);

	// Free the memory, it's no longer needed
	BridgeFree(vad);

	// Left
	if (!KdVadRecursiveWalk(leftChild, Callback))
		return false;

	// Right
	if (!KdVadRecursiveWalk(rightChild, Callback))
		return false;

	return true;
}