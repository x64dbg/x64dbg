#include "../_global.h"
#include "../_plugins.h"
#include "../plugin_loader.h"
#include "../console.h"
#include "stdafx.h"

/*--

Module Name:
kdevent.cpp

Purpose:
Handling messages and events passed by WinDbg's interface.

--*/

KD_CONTEXT LastContext;
EXCEPTION_RECORD64 LastException;

// Called when symbols are loaded and the debug loop starts
void KdEventInitialized()
{
	// Symbols loaded and everything is ready
	KdState.m_Initialized = true;

	// Notify plugins
	PLUG_CB_INITDEBUG initInfo;
	initInfo.szFileName = KdState.m_CommandLine;
	plugincbcall(CB_INITDEBUG, &initInfo);

	// Update initial GUI
	GuiSetDebugState(initialized);
	GuiAddRecentFile(KdState.m_CommandLine);
	GuiUpdateAllViews();

	// First break-in exception (if there was one)
	if (true)
	{
		GuiSetDebugState(paused);
		GuiDisasmAt(LastException.ExceptionAddress, LastException.ExceptionAddress);
	}
}

void KdEventUninitialized()
{
	// Notify plugins
	PLUG_CB_STOPDEBUG stopInfo;
	stopInfo.reserved = 0;
	plugincbcall(CB_STOPDEBUG, &stopInfo);

	// This is no longer valid
	KdState.m_Initialized = false;

	// Update the GUI elements
	GuiSetDebugState(stopped);
	GuiDisasmAt(0, 0);
	GuiUpdateAllViews();
}

bool KdEventBreakpoint(PDEBUG_BREAKPOINT Breakpoint)
{
	ULONG64 offset;
	Breakpoint->GetOffset(&offset);
	dprintf("Breakpoint at 0x%llX\n", offset);

	// Save the context
	KdContextGet(&LastContext);

	GuiDisasmAt(offset, LastContext.Cip);
	GuiUpdateRegisterView();
	GuiSetDebugState(paused);

	return true;
}

bool KdEventException(PEXCEPTION_RECORD64 Exception, ULONG FirstChance)
{
	// Save the context
	KdContextGet(&LastContext);

	// Then save the exception data
	memcpy(&LastException, Exception, sizeof(EXCEPTION_RECORD64));

	dprintf("Exception 0x%X (INT3) at 0x%08llX\n", LastException.ExceptionCode, LastException.ExceptionAddress);

	// Symbols must be loaded first
	if (!KdState.m_Initialized)
		return false;

	// Any other processing
	GuiSetDebugState(paused);
	GuiDisasmAt(Exception->ExceptionAddress, LastContext.Cip);
	GuiUpdateRegisterView();

	return true;
}

bool KdEventResumed()
{
	GuiSetDebugState(running);

	// View address stays the same, but IP is unknown
	GuiDisasmAt(LastContext.Cip, 0);

	return true;
}

bool KdEventPaused()
{
	// Don't update all of the UI every time the debugger is paused
	// This takes more time to read and transfer the memory
	if (!KdState.m_WasLastSingleStep)
		GuiUpdateAllViews();

	// Save the context
	KdContextGet(&LastContext);
	KdState.OS.Cr3 = LastContext.Cr3;

	GuiSetDebugState(paused);
	GuiDisasmAt(LastContext.Cip, LastContext.Cip);
	GuiStackDumpAt(LastContext.Csp, LastContext.Csp);
	GuiUpdateRegisterView();

	return true;
}

bool KdEventSingleStepOver()
{
	return true;
}

bool KdEventSingleStepInto()
{
	return true;
}

bool KdEventSingleStepBranch()
{
	dprintf("ss branch\n");

	return true;
}