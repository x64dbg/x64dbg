#include "../../memory.h"
#include "../stdafx.h"

bool KdMemQuery(ULONG64 Address, MEMORY_BASIC_INFORMATION64 *Information)
{
	// Zero out any garbage data
	memset(Information, 0, sizeof(MEMORY_BASIC_INFORMATION64));

	// If it's kernel memory check PTEs and driver images
	if (Address >= MM_SYSTEM_RANGE_START)
	{
		Information->AllocationProtect = PAGE_EXECUTE_READWRITE;
		Information->Protect = PAGE_EXECUTE_READWRITE;
		Information->State = MEM_COMMIT;

		DriverEntry *entry = KdDriverGetOwnerModule(Address);

		if (entry)
		{
			Information->AllocationBase = entry->Base;
			Information->BaseAddress = entry->Base;
			Information->RegionSize = entry->Size;
			Information->Type = MEM_IMAGE;
		}
		else
		{
			Information->AllocationBase = PAGE_ALIGN(Address);
			Information->BaseAddress = PAGE_ALIGN(Address);
			Information->RegionSize = PAGE_SIZE * 2;
			Information->Type = MEM_PRIVATE;
		}

		// ?!?!?!
		return true;
	}

	//
	// Is it KUSER_SHARED_DATA?
	//
	if (PAGE_ALIGN(Address) == MM_SHARED_USER_DATA_VA)
	{
		Information->AllocationBase = MM_SHARED_USER_DATA_VA;
		Information->AllocationProtect = PAGE_READONLY;
		Information->BaseAddress = PAGE_ALIGN(Address);
		Information->Protect = PAGE_READONLY;
		Information->RegionSize = PAGE_SIZE;
		Information->State = MEM_COMMIT;
		Information->Type = MEM_PRIVATE;

		return true;
	}

	//
	// Otherwise check VADs
	//

	// Allocate and read the node
	ULONG64 vadAddr = KdVadForProcessAddress(NtKdCurrrentProcess(), Address);

	// Unable to find a VAD for this address
	// FIXME (TEMPORARY) - Why are addresses of 0 passed in here?
	if (!vadAddr)
	{
		Information->AllocationBase = PAGE_ALIGN(Address);
		Information->AllocationProtect = PAGE_EXECUTE_READWRITE;
		Information->BaseAddress = PAGE_ALIGN(Address);
		Information->Protect = PAGE_EXECUTE_READWRITE;
		Information->RegionSize = PAGE_SIZE * 2;
		Information->State = MEM_COMMIT;
		Information->Type = MEM_PRIVATE;

		//DebugBreak();
		return true;
	}

	PMMVAD vad = KdVadAllocNode(&vadAddr);

	// Failed to allocate or read memory
	if (!vad)
		return false;

	ULONG startVpn;
	ULONG endVpn;
	KdVadGetVpns(vad, &startVpn, &endVpn);

	// Gather all of the memory flags
	KdVadGetFlags(vad, &Information->Protect, &Information->State, &Information->Type);

	// !!! Hack !!!
	Information->AllocationProtect = Information->Protect;

	// Determine the allocation base and size
	Information->BaseAddress	= PAGE_ALIGN(Address);
	Information->AllocationBase = MI_VPN_TO_VA((ULONG64)startVpn);
	Information->RegionSize		= MI_VPN_TO_VA((ULONG64)endVpn) - Information->AllocationBase;

	BridgeFree(vad);

	return true;
}

ULONG64 KdMemFindBaseAddress(ULONG64 Address, ULONG64 *Size)
{
	MEMORY_BASIC_INFORMATION64 mbi;

	if (!KdMemQuery(Address, &mbi))
		return 0;

	if (Size)
		*Size = mbi.RegionSize;

	return mbi.BaseAddress;
}

bool KdMemRead(const ULONG64 BaseAddress, PVOID Buffer, ULONG Size, PSIZE_T NumberOfBytesRead)
{
	if (KdState.m_Initialized)
	{
		if (NumberOfBytesRead)
			*NumberOfBytesRead = Size;

		// if (MemCacheEnabled())
		// {
		// }

#ifdef KDEBUGGER_VMIPC_EXT
		if (VmClientEnabled())
			return VmClientReadMemory(BaseAddress, Buffer, Size);
#endif // KDEBUGGER_VMIPC_EXT
	}

	ULONG bytesRead = 0;
	HRESULT result	= KdState.DebugDataSpaces2->ReadVirtual(BaseAddress, Buffer, Size, &bytesRead);

	if (!SUCCEEDED(result))
		return false;

	if (NumberOfBytesRead)
		*NumberOfBytesRead = bytesRead;

	return true;
}

ULONG64 KdMemReadPointer(const ULONG64 Address, bool *Success)
{
	ULONG64 buf		= 0;
	bool success	= KdMemRead(Address, &buf, sizeof(buf), nullptr);

	if (Success)
		*Success = success;

	return buf;
}

bool KdMemWrite(const ULONG64 BaseAddress, PVOID Buffer, ULONG Size, PSIZE_T NumberOfBytesWritten)
{
#ifdef KDEBUGGER_VMIPC_EXT
	// Causes a CPU fault sometimes
	// if (VmClientWriteMemory(BaseAddress, Buffer, Size))
	//	return true;
#endif // KDEBUGGER_VMIPC_EXT

	ULONG bytesWritten	= 0;
	HRESULT result		= KdState.DebugDataSpaces2->WriteVirtual(BaseAddress, Buffer, Size, &bytesWritten);

	if (!SUCCEEDED(result))
		return false;

	if (NumberOfBytesWritten)
		*NumberOfBytesWritten = bytesWritten;

	return true;
}

bool KdMemIsReadValid(ULONG64 Address)
{
	unsigned char a = 0;
	return KdMemRead(Address, &a, sizeof(a), nullptr);
}

PVOID KdMemAllocate(ULONG64 BaseAddress, ULONG Size, DWORD Protect)
{
	DebugBreak();
	return nullptr;
}

void KdMemFree(ULONG64 Address)
{
	DebugBreak();
}

bool KdMemScan(ULONG64 Address, ULONG Size, PBYTE Signature, PBYTE Mask, KdEnumCallback Callback)
{
	// Allocate the memory
	PBYTE data = (PBYTE)BridgeAlloc(Size);

	// Get a copy of it
	if (!KdMemRead(Address, data, Size, nullptr))
		return false;

	auto MemCompare = [](PBYTE Data, PBYTE Signature, PBYTE Mask)
	{
		while (*Mask)
		{
			if (*Mask == 'x' && *Data != *Signature)
				return false;

			++Mask;
			++Signature;
			++Data;
		}

		return (*Mask == 0);
	};

	for (ULONG i = 0; i < Size; i++)
	{
		if (MemCompare((data + i), Signature, Mask))
		{
			if (!Callback(i))
			{
				BridgeFree(data);
				return false;
			}
		}
	}

	BridgeFree(data);
	return true;
}