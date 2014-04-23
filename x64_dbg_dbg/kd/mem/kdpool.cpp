#include "../stdafx.h"

/*--

Module Name:
kdpool.cpp

Purpose:
Mapping and parsing Window's POOL_DESCRIPTOR/PoolVector
(PagedPool and NonPagedPool[NX])

"In Windows 7 and earlier versions of Windows, all memory allocated from the nonpaged pool is executable"
"Windows 8 and later versions of Windows should allocate their nonpaged memory from the no-execute (NX) nonpaged pool"

--*/

// _POOL_HEADER
ULONG Pool_HeaderSize;

KdOffsetBitField PoolBits_PreviousSize;
KdOffsetBitField PoolBits_PoolType;

bool KdPoolLoadSymbols()
{
#define MAKEFIELD(base, sym)	static ULONG __z ##sym; if (KdFindField(#base, #sym, &__z ##sym)) KdFields.Set(#base, #sym, __z ##sym); else return false;
#define GETBITS(str, mem)		KdFindFieldBitType(#str "." #mem, &VadBits_ ##mem.m_Start, &VadBits_ ##mem.m_Size);

	// POOL_DESCRIPTOR
	MAKEFIELD(_POOL_DESCRIPTOR, PoolType);		// POOL_TYPE
	MAKEFIELD(_POOL_DESCRIPTOR, PoolIndex);		// ULONG
	MAKEFIELD(_POOL_DESCRIPTOR, ListHeads);		// LIST_ENTRY[]

	// POOL_HEADER
	if (!KdGetFieldSize("_POOL_HEADER", &Pool_HeaderSize))
		return false;

#undef GETBITS
#undef MAKEFIELD

	return true;
}

ULONG KdPoolGetHeaderSize()
{
	return Pool_HeaderSize;
}