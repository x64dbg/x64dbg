#include "../console.h"
#include "stdafx.h"

// IUnknown
STDMETHODIMP EventCallbacks::QueryInterface(THIS_ REFIID InterfaceId, PVOID* Interface)
{
#if _MSC_VER >= 1100
	if (IsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
		IsEqualIID(InterfaceId, __uuidof(IDebugEventCallbacks)))
#else
	if (IsEqualIID(InterfaceId, IID_IUnknown) ||
		IsEqualIID(InterfaceId, IID_IDebugEventCallbacks))
#endif
	{
		*Interface = (IDebugEventCallbacks *)this;

		this->AddRef();
		return S_OK;
	}

	*Interface = NULL;
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) EventCallbacks::AddRef(THIS)
{
	return 1;
}

STDMETHODIMP_(ULONG) EventCallbacks::Release(THIS)
{
	return 0;
}

// IDebugEventCallbacks
STDMETHODIMP EventCallbacks::GetInterestMask(THIS_ PULONG Mask)
{
	*Mask =
		DEBUG_EVENT_BREAKPOINT |
		DEBUG_EVENT_EXCEPTION |
		DEBUG_EVENT_CREATE_THREAD |
		DEBUG_EVENT_EXIT_THREAD |
		DEBUG_EVENT_CREATE_PROCESS |
		DEBUG_EVENT_EXIT_PROCESS |
		DEBUG_EVENT_LOAD_MODULE |
		DEBUG_EVENT_UNLOAD_MODULE |
		DEBUG_EVENT_SYSTEM_ERROR |
		DEBUG_EVENT_SESSION_STATUS |
		DEBUG_EVENT_CHANGE_DEBUGGEE_STATE |
		DEBUG_EVENT_CHANGE_ENGINE_STATE |
		DEBUG_EVENT_CHANGE_SYMBOL_STATE;

	return S_OK;
}

STDMETHODIMP EventCallbacks::Breakpoint(THIS_ PDEBUG_BREAKPOINT Bp)
{
	// Return value ignored here
	KdEventBreakpoint(Bp);

	return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP EventCallbacks::Exception(THIS_ PEXCEPTION_RECORD64 Exception, ULONG FirstChance)
{
	// Return value is simply ignored here
	KdEventException(Exception, FirstChance);

	return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP EventCallbacks::CreateThread(THIS_ ULONG64 Handle, ULONG64 DataOffset, ULONG64 StartOffset)
{
	dprintf("THREAD CREATED: 0x%p\n", StartOffset);

	return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP EventCallbacks::ExitThread(THIS_ ULONG ExitCode)
{
	dprintf("THREAD EXITED: 0x%x\n", ExitCode);

	return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP EventCallbacks::CreateProcess(
	THIS_
	ULONG64 ImageFileHandle,
	ULONG64 Handle,
	ULONG64 BaseOffset,
	ULONG ModuleSize,
	PCSTR ModuleName,
	PCSTR ImageName,
	ULONG CheckSum,
	ULONG TimeDateStamp,
	ULONG64 InitialThreadHandle,
	ULONG64 ThreadDataOffset,
	ULONG64 StartOffset)
{
	dprintf("PROCESS CREATED: %s\n", ModuleName);

	return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP EventCallbacks::ExitProcess(THIS_ ULONG ExitCode)
{
	dprintf("PROCESS EXITED: 0x%x\n", ExitCode);

	return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP EventCallbacks::LoadModule(THIS_ ULONG64 ImageFileHandle, ULONG64 BaseOffset, ULONG ModuleSize, PCSTR ModuleName, PCSTR ImageName, ULONG CheckSum, ULONG TimeDateStamp)
{
	// Modules loaded at >= MM_SYSTEM_RANGE_START are all kernel
	// Modules loaded at <= MM_HIGHEST_USER_ADDRESS are processes or libraries

	if (BaseOffset >= MM_SYSTEM_RANGE_START)
		KdDriverLoad(ImageName, BaseOffset, ModuleSize);
	else if (BaseOffset <= MM_HIGHEST_USER_ADDRESS)
	//	DebugBreak();
	//else
		dprintf("MODULE LOADED: 0x%llx - %s\n", BaseOffset, ImageName);

	return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP EventCallbacks::UnloadModule(THIS_ PCSTR ImageBaseName, ULONG64 BaseOffset)
{
	// Modules unloaded at >= MM_SYSTEM_RANGE_START are all kernel
	// Modules unloaded at <= MM_HIGHEST_USER_ADDRESS are processes or libraries

	if (BaseOffset >= MM_SYSTEM_RANGE_START)
		KdDriverUnload(BaseOffset);
	else if (BaseOffset <= MM_HIGHEST_USER_ADDRESS)
	//	DebugBreak();
	//else
		dprintf("MODULE UNLOADED: 0x%llx - %s\n", BaseOffset, ImageBaseName);

	return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP EventCallbacks::SystemError(THIS_ ULONG Error, ULONG Level)
{
	dprintf("SYSTEM ERROR: 0x%x\n", Error);

	return DEBUG_STATUS_NO_CHANGE;
}

STDMETHODIMP EventCallbacks::SessionStatus(THIS_ ULONG Status)
{
	return S_OK;
}

STDMETHODIMP EventCallbacks::ChangeDebuggeeState(THIS_ ULONG Flags, ULONG64 Argument)
{
	return S_OK;
}

STDMETHODIMP EventCallbacks::ChangeEngineState(THIS_ ULONG Flags, ULONG64 Argument)
{
	//
	// NOTE:
	// This is called after an action is actually performed
	// The return value is also ignored
	//

	// Switch between 32-bit or 64-bit mode
	bool ptr64 = KdState.DebugControl2->IsPointer64Bit() == S_OK;

	if (KdState.OS.Ptr64 != ptr64)
	{
		KdState.OS.LastPtr64	= KdState.OS.Ptr64;
		KdState.OS.Ptr64		= ptr64;

		KdContextLoadSymbols();
	}

	if (Flags == DEBUG_CES_EXECUTION_STATUS)
	{
		// Argument might be OR'd with DEBUG_STATUS_INSIDE_WAIT,
		// so remove it
		switch (Argument & DEBUG_STATUS_MASK)
		{
		case DEBUG_STATUS_GO:
			// Currently executing
			break;

		case DEBUG_STATUS_STEP_OVER:
			// Single step over
			break;

		case DEBUG_STATUS_STEP_INTO:
			// Single step into
			break;

		case DEBUG_STATUS_BREAK:
			// Paused
			break;

		case DEBUG_STATUS_STEP_BRANCH:
			// Single step branch/jump
			break;
		}
	}

	if (Flags == DEBUG_CES_SYSTEMS)
	{
		// System was connected/added
	}

	return S_OK;
}

STDMETHODIMP EventCallbacks::ChangeSymbolState(THIS_ ULONG Flags, ULONG64 Argument)
{
	return S_OK;
}