#pragma once

class EventCallbacks : public DebugBaseEventCallbacks
{
public:
	// IUnknown
	STDMETHOD(QueryInterface)(THIS_ REFIID InterfaceId, PVOID* Interface);
	STDMETHOD_(ULONG, AddRef)(THIS);
	STDMETHOD_(ULONG, Release)(THIS);

	// IDebugEventCallbacks
	STDMETHOD(GetInterestMask)(THIS_ PULONG Mask);

	STDMETHOD(Breakpoint)(THIS_ PDEBUG_BREAKPOINT Bp);
	STDMETHOD(Exception)(THIS_ PEXCEPTION_RECORD64 Exception, ULONG FirstChance);

	STDMETHOD(CreateThread)(THIS_ ULONG64 Handle, ULONG64 DataOffset, ULONG64 StartOffset);
	STDMETHOD(ExitThread)(THIS_ ULONG ExitCode);

	STDMETHOD(CreateProcess)(
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
		ULONG64 StartOffset);
	STDMETHOD(ExitProcess)(THIS_ ULONG ExitCode);

	STDMETHOD(LoadModule)(THIS_ ULONG64 ImageFileHandle, ULONG64 BaseOffset, ULONG ModuleSize, PCSTR ModuleName, PCSTR ImageName, ULONG CheckSum, ULONG TimeDateStamp);
	STDMETHOD(UnloadModule)(THIS_ PCSTR ImageBaseName, ULONG64 BaseOffset);

	STDMETHOD(SystemError)(THIS_ ULONG Error, ULONG Level);
	STDMETHOD(SessionStatus)(THIS_ ULONG Status);
	STDMETHOD(ChangeDebuggeeState)(THIS_ ULONG Flags, ULONG64 Argument);
	STDMETHOD(ChangeEngineState)(THIS_ ULONG Flags, ULONG64 Argument);
	STDMETHOD(ChangeSymbolState)(THIS_ ULONG Flags, ULONG64 Argument);
};