#include "../_global.h"
#include "../command.h"
#include "../argument.h"
#include "../_plugins.h"
#include "../plugin_loader.h"
#include "../console.h"
#include "../variable.h"
#include "../threading.h"
#include "../debugger.h"
#include "stdafx.h"

HRESULT (STDAPICALLTYPE * pfnDebugCreate)(REFIID InterfaceId, PVOID *Interface);

bool KdDebugEnabled()
{
	// fix this...
	return true;
}

CMDRESULT KdDebugInit(int argc, char *argv[])
{
	if (DbgIsDebugging())
	{
		dputs("Debugger is already running!");
		return STATUS_ERROR;
	}

	// Reset any previous variables
	KdStateReset();

	// Try to get the command line
	strcpy_s(KdState.m_CommandLine, "com:port=\\\\.\\pipe\\kd_Windows_7_x64,baud=115200,pipe,reconnect");
	/*if (!argget(*argv, KdState.m_CommandLine, 0, false))
	{
		dprintf("Invalid command line specified\n");
		return STATUS_ERROR;
	}*/

	// Set the directory
	// NOT SUPPORTED ON BASE WINXP (die already)
	//SetDllDirectory("C:\\Program Files (x86)\\Windows Kits\\8.1\\Debuggers\\x64");

	// Try to load the dbgeng.dll library
	HMODULE hDbgEng = LoadLibraryA("C:\\Program Files (x86)\\Windows Kits\\8.1\\Debuggers\\x64\\dbgeng.dll");

	if (!hDbgEng)
	{
		dprintf("Unable to load DbgEng library (0x%X)\n", GetLastError());
		return STATUS_ERROR;
	}

	*(FARPROC *)&pfnDebugCreate = GetProcAddress(hDbgEng, "DebugCreate");

	if (!pfnDebugCreate)
	{
		dprintf("Unable to find export 'DebugCreate' in DbgEng library\n");
		return STATUS_ERROR;
	}

	// Wait for the debugger to stop
	wait(WAITID_STOP);
	waitclear();

	// Create the debugging loop thread
	HANDLE hThread = CreateThread(nullptr, 0, KdDebugLoop, nullptr, 0, nullptr);

	if (!hThread)
	{
		dprintf("Failed to create debug thread!\n");
		return STATUS_ERROR;
	}

	CloseHandle(hThread);

	return STATUS_CONTINUE;
}

void KdDebugShutdown()
{
#define SAFE_RELEASE(x) if (x){ x->Release(); x = nullptr; }

	//
	// Notify the event manager
	//
	KdEventUninitialized();

	//
	// Kill IPC
	//
#ifdef KDEBUGGER_VMIPC_EXT
	VmClientTerminateIPC();
#endif // KDEBUGGER_VMIPC_EXT

	//
	// Release handles and pointers
	//
	SAFE_RELEASE(KdState.DebugAdvanced);
	SAFE_RELEASE(KdState.DebugClient);
	SAFE_RELEASE(KdState.DebugControl2);
	SAFE_RELEASE(KdState.DebugDataSpaces2);
	SAFE_RELEASE(KdState.DebugRegisters);
	SAFE_RELEASE(KdState.DebugSymbols);
	SAFE_RELEASE(KdState.DebugSystemObjects);

	//
	// Clear the final state
	//
	KdState.m_Debugging = false;

#undef SAFE_RELEASE
}

DWORD WINAPI KdDebugLoop(LPVOID lpArg)
{
	// Thread is locked and running
	lock(WAITID_STOP);

	// Get a handle to all needed WinDbg interfaces
	if (!KdDebugQueryInterfaces())
		return 1;

	// Set the user options in the debug engine
	if (!KdDebugSetOptions())
		return 1;

	//
	//
	// Connect to the remote kernel
	//
	//
	dprintf("Attaching to remote kernel with command line: %s\n", KdState.m_CommandLine);

	HRESULT status = KdState.DebugClient->AttachKernel(DEBUG_ATTACH_KERNEL_CONNECTION, KdState.m_CommandLine);

	if (!SUCCEEDED(status))
	{
		dprintf("Failed to attach, 0x%X\n", status);
		return 1;
	}

	//
	//
	// Break-in
	//
	//
	status = KdState.DebugControl2->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);

	if (!SUCCEEDED(status))
	{
		dprintf("Initial debug break-in event failed, 0x%X\n", status);
		return 1;
	}

	// This won't work
	// It also returns ptr32 true if running under WOW64
	KdState.OS.Ptr64 = true;

	//
	//
	// Debugger is connected at this point
	//
	//

	// Undocumented; reversed from a SymGetTypeInfo call
	KdState.SymbolHandle	= (HANDLE)0xF0F0F0F0;
	fdProcessInfo->hProcess = KdState.SymbolHandle;

	varset("$hp", (uint)KdState.SymbolHandle, true);
	varset("$pid", (uint)0, true);

	KdState.m_Debugging = true;
	KdState.m_DebugOutput = true;

	//
	//
	// Load the virtual machine's physical memory pipe (if applicable)
	//
	//
#ifdef KDEBUGGER_VMIPC_EXT
	if (!VmClientEstablishIPC())
		dprintf("Remote IPC is unavailable\n");
#endif // KDEBUGGER_VMIPC_EXT

	//
	//
	// Load symbols and offsets
	//
	//
	dprintf("\n\n");
	dprintf("***************************************\n");
	dprintf("*          Gathering symbols          *\n");
	dprintf("***************************************\n");

	if (!KdLoadSymbols())
	{
		dprintf("Unable to load symbols\n");
		return 1;
	}

	//
	//
	// Notify the event manager
	//
	//
	KdEventInitialized();

	ULONG LastState = 0;
	// Actual debugging loop
	while (true)
	{
		ULONG ExecStatus = 0;

		Sleep(50);

		if ((KdState.DebugControl2->GetExecutionStatus(&ExecStatus)) != S_OK)
		{
			dprintf("GetExecutionStatus failed\n");
			break; // quit while loop if GetExecutionStatus failed
		}
		if (ExecStatus == DEBUG_STATUS_NO_DEBUGGEE)
		{
			dprintf("no debuggee\n");
			break; // quit while loop if no debugee
		}

		if (LastState != ExecStatus)
		{
			switch (ExecStatus)
			{
			case DEBUG_STATUS_GO:
				KdEventResumed();
				break;

			case DEBUG_STATUS_BREAK:
				KdEventPaused();
				break;

			case DEBUG_STATUS_STEP_OVER:
				KdEventSingleStepOver();
				break;

			case DEBUG_STATUS_STEP_INTO:
				KdEventSingleStepInto();
				break;

			case DEBUG_STATUS_STEP_BRANCH:
				KdEventSingleStepBranch();
				break;

			case DEBUG_STATUS_NO_DEBUGGEE:
				dprintf("No debugee\n");
				break;
			}
		}

		LastState = ExecStatus;

		if (
			ExecStatus == DEBUG_STATUS_GO ||
			ExecStatus == DEBUG_STATUS_STEP_OVER ||
			ExecStatus == DEBUG_STATUS_STEP_OVER ||
			ExecStatus == DEBUG_STATUS_STEP_INTO ||
			ExecStatus == DEBUG_STATUS_STEP_BRANCH
			)
		{
			if ((status = KdState.DebugControl2->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE)) == S_OK)
			{
				continue;  // do not prompt or stop on the above execution status
			}
			if (status != E_UNEXPECTED)
			{
				dprintf("Wait For Event in main loop failed\n");
				break;  // quit while loop on a fatal error in waitforevent
			}
		}
	}

	//
	//
	// Shutdown
	//
	//
	KdDebugShutdown();

	// Thread exiting
	unlock(WAITID_STOP);

	return 0;
}

bool KdDebugQueryInterfaces()
{
#define QUERY_INTERFACE(a) if (!SUCCEEDED(KdState.DebugClient->QueryInterface(__uuidof(I##a), (PVOID *)&KdState.##a))) \
	{ \
	dprintf("Failed to query interface 'I%s'\n", #a); \
	return false; \
}

	// Grab any interface pointers first
	// IDebugClient
	HRESULT status = pfnDebugCreate(__uuidof(IDebugClient), (PVOID *)&KdState.DebugClient);

	if (!SUCCEEDED(status))
	{
		dprintf("DebugCreate failed, 0x%X\n", status);
		return false;
	}

	QUERY_INTERFACE(DebugAdvanced);
	QUERY_INTERFACE(DebugControl2);
	QUERY_INTERFACE(DebugDataSpaces2);
	QUERY_INTERFACE(DebugRegisters);
	QUERY_INTERFACE(DebugSymbols);
	QUERY_INTERFACE(DebugSystemObjects);

	// Register WinDbg callbacks
	KdState.DebugClient->SetOutputCallbacks(&KdState.OutputData);
	KdState.DebugClient->SetEventCallbacks(&KdState.EventData);

#undef QUERY_INTERFACE

	return true;
}

bool KdDebugSetOptions()
{
	// Set the initial breakpoint options
	KdState.DebugControl2->AddEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK);
	KdState.DebugControl2->AddEngineOptions(DEBUG_ENGOPT_INITIAL_MODULE_BREAK);

	return true;
}

struct UNICODE_STRING64
{
	USHORT	Length;
	USHORT	MaximumLength;
	ULONG64	Buffer;
};

struct LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY64 InLoadOrderLinks;
	LIST_ENTRY64 InMemoryOrderLinks;
	LIST_ENTRY64 InInitializationOrderLinks;
	ULONG64 DllBase;
	ULONG64 EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING64 FullDllName;
	UNICODE_STRING64 BaseDllName;
	ULONG Flags;
	WORD LoadCount;
	WORD TlsIndex;
};

int sys = 0;
bool KdLoadSymbols()
{
#define MAKESYM(base, sym)		ULONG64 __z ##sym; if (KdFindSymbol(#base "!" #sym, &__z ##sym)) KdSymbols.Set(#base, #sym, __z ##sym); else return false;
#define MAKEFIELD(base, sym)	ULONG __z ##sym; if (KdFindField(#base, #sym, &__z ##sym)) KdFields.Set(#base, #sym, __z ##sym); else return false;
#define MAKEPTRVAL(val)			if (!KdMemRead(val, &val, sizeof(val), nullptr)) return false;

	// Disable usermode symbols at first (if they're enabled)
	// This can be re-enabled later
	// "Disable loading of kernel debugger symbols."
	KdState.DebugControl2->Execute(DEBUG_OUTCTL_THIS_CLIENT, "!gflag -ksl", DEBUG_EXECUTE_DEFAULT);

	// "Disable paging of kernel stacks."
	// KdState.DebugControl2->Execute(DEBUG_OUTCTL_THIS_CLIENT, "!gflag +dps", DEBUG_EXECUTE_DEFAULT);

	// Force a WinDbg symbol reload
	// Reloads all usermode and kernelmode symbols (for the process context)
	KdState.DebugControl2->Execute(DEBUG_OUTCTL_THIS_CLIENT, ".reload", DEBUG_EXECUTE_DEFAULT);

	MAKESYM(nt, KdDebuggerDataBlock);			// KDDEBUGGER_DATA64/32
	MAKESYM(nt, KdDebuggerNotPresent);			// BYTE

	MAKESYM(nt, KiProcessorBlock);				// PKPRCB[ProcessorCount]
	MAKESYM(nt, KeServiceDescriptorTable);		// KSERVICE_TABLE_DESCRIPTOR
	MAKESYM(nt, KeServiceDescriptorTableShadow);// KSERVICE_TABLE_DESCRIPTOR
	MAKESYM(nt, HalDispatchTable);				// HAL_DISPATCH
	MAKESYM(nt, PsLoadedModuleList);			// LIST_ENTRY
	MAKESYM(nt, PsActiveProcessHead);			// LIST_ENTRY
	MAKESYM(nt, PsInitialSystemProcess);		// EPROCESS

	// Read and fill out KdDebuggerDataBlock64/32
	KdMemRead(KdSymbols["nt"]["KdDebuggerDataBlock"], &KdDebuggerData, sizeof(KdDebuggerData), nullptr);

	// Fix up the pointers so they're actually values
	// TODO: THESE ARE NOT INITIALIZED AT THE FIRST BREAK IN!!!
	MAKEPTRVAL(KdDebuggerData.MmHighestUserAddress);
	MAKEPTRVAL(KdDebuggerData.MmSystemRangeStart);
	MAKEPTRVAL(KdDebuggerData.MmUserProbeAddress);

	// Calculate PAGE_SHIFT from PAGE_SIZE
	KdState.OS.PageSize = KdDebuggerData.MmPageSize;

	for (ULONG i = 0; i < (sizeof(ULONG) * 8); i++)
	{
		if ((KdDebuggerData.MmPageSize & (ULONG64)(1 << i)) != 0)
			KdState.OS.PageShift = i;
	}

	// EPROCESS
	MAKEFIELD(_EPROCESS, Pcb);					// KPROCESS
	MAKEFIELD(_EPROCESS, ActiveProcessLinks);	// LIST_ENTRY
	MAKEFIELD(_EPROCESS, ImageFileName);		// UCHAR[16]
	MAKEFIELD(_EPROCESS, ThreadListHead);		// LIST_ENTRY
	MAKEFIELD(_EPROCESS, ActiveThreads);		// ULONG
	MAKEFIELD(_EPROCESS, Peb);					// PPEB
	MAKEFIELD(_EPROCESS, VadRoot);				// MM_AVL_TABLE

	// ETHREAD
	MAKEFIELD(_ETHREAD, Tcb);					// KTHREAD
	MAKEFIELD(_ETHREAD, Cid);					// CLIENT_ID
	MAKEFIELD(_ETHREAD, ThreadListEntry);		// LIST_ENTRY

	// LDR_DATA_TABLE_ENTRY
	MAKEFIELD(_LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);				// LIST_ENTRY
	MAKEFIELD(_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);			// LIST_ENTRY
	MAKEFIELD(_LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);	// LIST_ENTRY
	MAKEFIELD(_LDR_DATA_TABLE_ENTRY, DllBase);						// PVOID
	MAKEFIELD(_LDR_DATA_TABLE_ENTRY, EntryPoint);					// PVOID

	//
	// Kernel memory pools
	//
	if (!KdPoolLoadSymbols())
		return false;

	//
	// Virtual Address Descriptors (VADs)
	//
	if (!KdVadLoadSymbols())
		return false;

	//
	// Kernel Processor Control Region Block
	//
	if (!KdKprcbLoadSymbols())
		return false;

	//
	// System and processor contexts
	//
	if (!KdContextLoadSymbols())
		return false;

	//
	// VM helper 'emulation' driver
	//
	//if (!KdOpEmuLoad())
	//	dprintf("VM emulation helper driver not detected. Features unavailable.\n");

	//
	// Kernel Patch Protection - "PatchGuard"
	//
	if (!PatchGuardLoadSymbols())
		dprintf("Failed to load PatchGuard symbols\n");

	// Update the initial driver list
	auto DriverEnum = [](ULONG64 addr)
	{
		// Read the loader entry
		LDR_DATA_TABLE_ENTRY ldr;
		if (!KdMemRead(addr, &ldr, sizeof(ldr), nullptr))
			return false;

		// Read the base file names
		wchar_t dllNameW[MAX_PATH];
		memset(dllNameW, 0, sizeof(dllNameW));

		if (!KdMemRead(ldr.BaseDllName.Buffer, &dllNameW, ldr.BaseDllName.Length, nullptr))
			return false;

		// Unicode to multi-byte
		char dllNameA[MAX_PATH];
		memset(dllNameA, 0, sizeof(dllNameA));

		wcstombs(dllNameA, dllNameW, MAX_PATH);

		// Notify the handler
		KdDriverLoad(dllNameA, ldr.DllBase, ldr.SizeOfImage);
		return true;
	};

	if (!NtKdEnumerateDrivers(DriverEnum))
		dprintf("Failed to enumerate initial list of drivers\n");

	// Update the initial process list
	auto ProcessEnum = [](ULONG64 EProcess)
	{
		char temp[64];
		memset(&temp, 0, sizeof(temp));

		ULONG64 peb = 0;

		KdState.DebugDataSpaces2->ReadVirtual(EProcess + KdFields["_EPROCESS"]["ImageFileName"], &temp, sizeof(temp), nullptr);
		KdState.DebugDataSpaces2->ReadVirtual(EProcess + KdFields["_EPROCESS"]["Peb"], &peb, sizeof(peb), nullptr);

		dprintf("Process: %s\t\t0x%08llX\n", temp, peb);

		auto vads = [](ULONG64 Node)
		{
			dprintf("VAD Node: 0x%08llX\n", Node);
			return true;
		};

		//NtKdEnumerateProcessVads(EProcess, vads);

		// The first process is always 'System'
		if (sys == 0)
		{
			KdSetProcessContext(EProcess);
		}

		sys++;

		//strcpy_s(g_syms[g_symcount].name, temp);
		//g_syms[g_symcount++].base = peb;

		//KdSetProcessContext(EProcess);

		auto thread = [](ULONG64 EThread)
		{
			ULONG id;
			KdState.DebugDataSpaces2->ReadVirtual(EThread + KdFields["_ETHREAD"]["Cid"], &id, sizeof(id), nullptr);

			dprintf("\tThread: 0x%x\t\t0x%08llX\n", id, EThread);

			return true;
		};

		//NtKdEnumerateProcessThreads(EProcess, thread);

		return true;
	};

	NtKdEnumerateProcesses(ProcessEnum);

#undef MAKEPTRVAL
#undef MAKEFIELD
#undef MAKESYM

	return true;
}

void KdSetProcessContext(ULONG64 EProcess)
{
	char buf[64];
	sprintf_s(buf, ".process /r /p 0x%08llX", EProcess);

	// Log it
	dprintf("Switching process to 0x%08llX...\n", EProcess);

	// Execute the debugger command
	KdState.DebugControl2->Execute(DEBUG_OUTCTL_IGNORE, buf, DEBUG_EXECUTE_DEFAULT);
}

bool KdWalkListEntry(ULONG64 ListStart, PVOID UserData, KdListEnumCallback Callback)
{
	LIST_ENTRY64 list;

	if (!KdMemRead(ListStart, &list, sizeof(list), nullptr))
	{
		dprintf("Failed to read list memory at 0x%08llX\n", ListStart);
		return false;
	}

	ULONG64 listCurrent	= list.Flink;
	ULONG64 listEnd		= ListStart;

	while (listCurrent && listCurrent != listEnd)
	{
		if (!Callback(listCurrent, UserData))
			return false;

		// Move to the next forward link
		if (!KdMemRead(listCurrent, &listCurrent, sizeof(listCurrent), nullptr))
			return false;
	}

	return true;
}