#include "../stdafx.h"

/*--

Module Name:
vmclient.cpp

Purpose:
Fast interprocess communication using an external dll injected into a
virtual machine. Allows direct physical memory access to the machine.
~2.5x faster than using DbgEng with VirtualKD.

--*/

#ifdef KDEBUGGER_VMIPC_EXT
HANDLE g_Pipe;
HANDLE g_MapFile;
LPVOID g_MapMemory;
bool g_Enabled;

bool VmClientEstablishIPC()
{
	// Establish a pipe connection for reading and writing messages
	g_Pipe = CreateFile(VM_CLIENT_IPC_PATH, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (g_Pipe == INVALID_HANDLE_VALUE)
		return false;

	// Create a mapping of the page file
	g_MapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, VM_CLIENT_IPC_MEM_SIZE, VM_CLIENT_IPC_MEM_PATH);

	if (!g_MapFile)
		return false;

	// Now map it into the process's memory
	g_MapMemory = MapViewOfFile(g_MapFile, FILE_MAP_ALL_ACCESS, 0, 0, VM_CLIENT_IPC_MEM_SIZE);

	if (!g_MapMemory)
		return false;

	// Set the status
	g_Enabled = true;

	return true;
}

void VmClientTerminateIPC()
{
	if (g_Enabled)
	{
		// Send the connection termination command
		IPCPacket packet;
		memset(&packet, 0, sizeof(IPCPacket));

		packet.MessageType	= VM_CLIENT_SHUTDOWN_CMD;
		packet.Status		= 1;

		// Return value is ignored
		VmClientSendPacket(&packet);

		if (g_MapMemory)
			UnmapViewOfFile(g_MapMemory);

		if (g_Pipe)
			CloseHandle(g_Pipe);

		if (g_MapFile)
			CloseHandle(g_MapFile);
	}

	g_MapMemory = 0;
	g_Pipe		= 0;
	g_MapFile	= 0;
	g_Enabled	= false;
}

bool ReadFromPipe(IPCPacket *Packet)
{
	DWORD bytesRead;

	if (!ReadFile(g_Pipe, Packet, sizeof(IPCPacket), &bytesRead, nullptr))
		return false;

	return bytesRead == sizeof(IPCPacket);
}

bool WriteToPipe(IPCPacket *Packet)
{
	DWORD bytesWritten;

	if (!WriteFile(g_Pipe, Packet, sizeof(IPCPacket), &bytesWritten, nullptr))
		return false;

	return bytesWritten == sizeof(IPCPacket);
}

bool VmClientEnabled()
{
	return g_Enabled;
}

bool VmClientSendPacket(IPCPacket *Packet)
{
	if (!WriteToPipe(Packet))
		return false;

	if (!ReadFromPipe(Packet))
		return false;

	return true;
}

bool VmClientReadMemory(ULONG64 Address, PVOID Buffer, ULONG Size)
{
	// Set up the base packet and parameters
	IPCPacket packet;
	memset(&packet, 0, sizeof(IPCPacket));

	//
	// THIS SHOULD BE DONE ON THE VIRTUAL MACHINE END
	// PAGES ARE NOT A STATIC SIZE; EX: LARGE PAGES, 1G PAGES
	// Temporary for now
	//

	// If (SIZE > PAGE_SIZE) or (ADDRESS extends boundary), multiple reads will be needed
	// Pages in physical memory are not always contiguous
	ULONG pageCount = BYTES_TO_PAGES(Size);

	if (pageCount > 1)
	{
		// Read the memory in a span of pages
		// Determine the number of bytes between ADDRESS and the next page
		ULONG64 offset		= 0;
		ULONG64 readBase	= Address;
		ULONG64 readSize	= ROUND_TO_PAGES(Address) - Address;

		for (ULONG i = 0; i < pageCount; i++)
		{
			// Directly read the memory
			packet.MessageType = VM_CLIENT_READMEM_CMD;	// Command
			packet.Status = 1;							// Last status
			packet.Parameters[0] = 1;					// Physical (0)/Virtual (1)
			packet.Parameters[1] = readBase;			// Address
			packet.Parameters[2] = readSize;			// Size
			packet.Parameters[3] = KdState.OS.Cr3;		// Physical translation base

			// Check for success
			if (!VmClientSendPacket(&packet) || !packet.Status)
				return false;

			// Copy the memory into the buffer
			memcpy(((PBYTE)Buffer + offset), g_MapMemory, readSize);

			offset += readSize;
			readBase += readSize;

			Size -= readSize;
			readSize = (Size > PAGE_SIZE) ? PAGE_SIZE : Size;
		}
	}
	else
	{
		// Directly read the memory
		packet.MessageType = VM_CLIENT_READMEM_CMD;	// Command
		packet.Status = 1;							// Last status
		packet.Parameters[0] = 1;					// Physical (0)/Virtual (1)
		packet.Parameters[1] = Address;				// Address
		packet.Parameters[2] = Size;				// Size
		packet.Parameters[3] = KdState.OS.Cr3;		// Physical translation base

		// Check for success
		if (!VmClientSendPacket(&packet) || !packet.Status)
			return false;

		// Copy the memory into the buffer
		memcpy(Buffer, g_MapMemory, Size);
	}

	return true;
}

bool VmClientWriteMemory(ULONG64 Address, PVOID Buffer, ULONG Size)
{
	IPCPacket packet;
	packet.MessageType		= VM_CLIENT_WRITEMEM_CMD;
	packet.Status			= 1;
	packet.Parameters[0]	= 1;
	packet.Parameters[1]	= Address;
	packet.Parameters[2]	= Size;
	packet.Parameters[3]	= KdState.OS.Cr3;

	// Copy the memory into the buffer
	memcpy(g_MapMemory, Buffer, Size);

	if (!WriteToPipe(&packet))
		return false;

	if (!ReadFromPipe(&packet))
		return false;

	// Check for success
	if (!packet.Status)
		return false;

	return true;
}
#endif // KDEBUGGER_VMIPC_EXT