#pragma once

struct KdState_s
{
	// Basic variables
	bool m_Initialized;
	bool m_Debugging;
	bool m_DebugOutput;
	char m_CommandLine[1024];

	// True if the last command was a single step
	bool m_WasLastSingleStep;

	// WinDbg interface pointers and values
	HANDLE				SymbolHandle;

	IDebugAdvanced		*DebugAdvanced;
	IDebugClient		*DebugClient;
	IDebugControl2		*DebugControl2;
	IDebugDataSpaces2	*DebugDataSpaces2;
	IDebugRegisters		*DebugRegisters;
	IDebugSymbols		*DebugSymbols;
	IDebugSystemObjects *DebugSystemObjects;

	// User defined callbacks
	OutputCallbacks		OutputData;
	EventCallbacks		EventData;

	// OS data
	struct  
	{
		ULONG64			PageSize;
		ULONG			PageShift;
		ULONG64 Cr3;

		// Can be switched at runtime due to WOW64
		bool			Ptr64;
		bool			LastPtr64;

		// Opcode fixes for WinDbg
		bool			OpcodeEmulation;

		// Used for contexts
		ULONG			RegisterIds[1024];
	} OS;
};

extern KdState_s			KdState;
extern KdOffsetManager		KdSymbols;
extern KdOffsetManager		KdFields;

#ifdef _WIN64
extern KDDEBUGGER_DATA64	KdDebuggerData;
#else
extern KDDEBUGGER_DATA32	KdDebuggerData;
#endif

void KdStateReset();