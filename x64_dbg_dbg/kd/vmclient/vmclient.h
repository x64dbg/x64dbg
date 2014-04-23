#pragma once

#ifdef KDEBUGGER_VMIPC_EXT
#define VM_CLIENT_READMEM_CMD		0
#define VM_CLIENT_WRITEMEM_CMD		1
#define VM_CLIENT_SHUTDOWN_CMD		2

#define VM_CLIENT_IPC_MEM_SIZE		8192
#define VM_CLIENT_IPC_MEM_PATH		"Global\\VmCliSharedMem"
#define VM_CLIENT_IPC_PATH			"\\\\.\\Pipe\\VmCliIpc"

struct IPCPacket
{
	ULONG		MessageType;
	ULONG		Status;
	ULONG64		Parameters[4];
};

bool VmClientEstablishIPC();
void VmClientTerminateIPC();

bool VmClientEnabled		();
bool VmClientSendPacket		(IPCPacket *Packet);
bool VmClientReadMemory		(ULONG64 Address, PVOID Buffer, ULONG Size);
bool VmClientWriteMemory	(ULONG64 Address, PVOID Buffer, ULONG Size);
#endif // KDEBUGGER_VMIPC_EXT