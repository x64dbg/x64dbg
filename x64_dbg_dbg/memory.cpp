#include "memory.h"
#include "debugger.h"

uint memfindbaseaddr(HANDLE hProcess, uint addr, uint* size)
{
#ifdef KDEBUGGER_ENABLE
	if (KdDebugEnabled())
	{
		ULONG64 size64 = 0;
		ULONG64 ret = KdMemFindBaseAddress(addr, &size64);

		if (size)
			*size = size64;

		return ret;
	}
#endif // KDEBUGGER_ENABLE

	MEMORY_BASIC_INFORMATION mbi;
	DWORD numBytes;
	uint MyAddress = 0, newAddress = 0;
	do
	{
		numBytes = VirtualQueryEx(hProcess, (LPCVOID) MyAddress, &mbi, sizeof(mbi));
		newAddress = (uint) mbi.BaseAddress + mbi.RegionSize;
		if (mbi.State == MEM_COMMIT and addr < newAddress and addr >= MyAddress)
		{
			if (size)
				*size = mbi.RegionSize;
			return (uint) mbi.BaseAddress;
		}
		if (newAddress <= MyAddress)
			numBytes = 0;
		else
			MyAddress = newAddress;
	} while (numBytes);
	return 0;
}

bool memread(HANDLE hProcess, const void* lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
{
#ifdef KDEBUGGER_ENABLE
	if (KdDebugEnabled())
		return KdMemRead((ULONG64) lpBaseAddress, (PVOID) lpBuffer, nSize, lpNumberOfBytesRead);
#endif // KDEBUGGER_ENABLE

	if (!hProcess or !lpBaseAddress or !lpBuffer or !nSize) //generic failures
		return false;
	SIZE_T read = 0;
	DWORD oldprotect = 0;
	bool ret = MemoryReadSafe(hProcess, (void*) lpBaseAddress, lpBuffer, nSize, &read); //try 'normal' RPM
	if (ret and read == nSize) //'normal' RPM worked!
	{
		if (lpNumberOfBytesRead)
			*lpNumberOfBytesRead = read;
		return true;
	}
	for (uint i = 0; i < nSize; i++) //read byte-per-byte
	{
		unsigned char* curaddr = (unsigned char*) lpBaseAddress + i;
		unsigned char* curbuf = (unsigned char*) lpBuffer + i;
		ret = MemoryReadSafe(hProcess, curaddr, curbuf, 1, 0); //try 'normal' RPM
		if (!ret) //we failed
		{
			if (lpNumberOfBytesRead)
				*lpNumberOfBytesRead = i;
			SetLastError(ERROR_PARTIAL_COPY);
			return false;
		}
	}
	return true;
}

bool memwrite(HANDLE hProcess, void* lpBaseAddress, const void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten)
{
#ifdef KDEBUGGER_ENABLE
	if (KdDebugEnabled())
		return KdMemWrite((ULONG64) lpBaseAddress, (PVOID) lpBuffer, nSize, lpNumberOfBytesWritten);
#endif // KDEBUGGER_ENABLE

	if (!hProcess or !lpBaseAddress or !lpBuffer or !nSize) //generic failures
		return false;
	SIZE_T written = 0;
	DWORD oldprotect = 0;
	bool ret = MemoryWriteSafe(hProcess, lpBaseAddress, lpBuffer, nSize, &written);
	if (ret and written == nSize) //'normal' WPM worked!
	{
		if (lpNumberOfBytesWritten)
			*lpNumberOfBytesWritten = written;
		return true;
	}
	for (uint i = 0; i < nSize; i++) //write byte-per-byte
	{
		unsigned char* curaddr = (unsigned char*) lpBaseAddress + i;
		unsigned char* curbuf = (unsigned char*) lpBuffer + i;
		ret = MemoryWriteSafe(hProcess, curaddr, curbuf, 1, 0); //try 'normal' WPM
		if (!ret) //we failed
		{
			if (lpNumberOfBytesWritten)
				*lpNumberOfBytesWritten = i;
			SetLastError(ERROR_PARTIAL_COPY);
			return false;
		}
	}
	return true;
}

bool memisvalidreadptr(HANDLE hProcess, uint addr)
{
#ifdef KDEBUGGER_ENABLE
	if (KdDebugEnabled())
		return KdMemIsReadValid(addr);
#endif // KDEBUGGER_ENABLE

	unsigned char a = 0;
	return memread(hProcess, (void*) addr, &a, 1, 0);
}

void* memalloc(HANDLE hProcess, uint addr, DWORD size, DWORD fdProtect)
{
#ifdef KDEBUGGER_ENABLE
	if (KdDebugEnabled())
		return KdMemAllocate(addr, size, fdProtect);
#endif // KDEBUGGER_ENABLE

	return VirtualAllocEx(hProcess, (void*) addr, size, MEM_RESERVE | MEM_COMMIT, fdProtect);
}

void memfree(HANDLE hProcess, uint addr)
{
#ifdef KDEBUGGER_ENABLE
	if (KdDebugEnabled())
		KdMemFree(addr);
#endif // KDEBUGGER_ENABLE

	VirtualFreeEx(hProcess, (void*) addr, 0, MEM_RELEASE);
}