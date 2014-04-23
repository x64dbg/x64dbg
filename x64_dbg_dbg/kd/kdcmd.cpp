#include "../_global.h"
#include "../command.h"
#include "../argument.h"
#include "../console.h"
#include "../x64_dbg.h"
#include "stdafx.h"

CMDRESULT KdCmdKd(int argc, char* argv[]);
CMDRESULT KdCmdProcess(int argc, char* argv[]);
CMDRESULT KdCmdVad(int argc, char* argv[]);
CMDRESULT KdCmdRun(int argc, char *argv[]);
CMDRESULT KdCmdStepInto(int argc, char *argv[]);
CMDRESULT KdCmdStepOver(int argc, char *argv[]);

void KdAddCommands()
{
	cmddel(dbggetcommandlist(), "run");
	cmddel(dbggetcommandlist(), "StepInto");
	cmddel(dbggetcommandlist(), "StepOver");

	cmdnew(dbggetcommandlist(), "kd", KdCmdKd, true);								// Execute a WinDbg command
	cmdnew(dbggetcommandlist(), "kdproc", KdCmdProcess, true);						// Switch process contexts
	cmdnew(dbggetcommandlist(), "kdvad", KdCmdVad, true);							// Find VAD for process address
	cmdnew(dbggetcommandlist(), "run\1go\1r\1g", KdCmdRun, true);					// Resume execution
	cmdnew(dbggetcommandlist(), "StepInto\1sti", KdCmdStepInto, true);				// Execute a single instruction
	cmdnew(dbggetcommandlist(), "StepOver\1step\1sto\1st", KdCmdStepOver, true);	// Execute over a single instruction
}

CMDRESULT KdCmdKd(int argc, char* argv[])
{
	// Format and concatenate arguments to send to WinDbg
	char buffer[deflen];
	memset(buffer, 0, sizeof(buffer));

	for (int i = 1; i < argc; i++)
	{
		char arg[64];
		argget(*argv, arg, i, true);

		strcat_s(buffer, argv[i]);
		strcat_s(buffer, " ");
	}

	// Remove the last space if there is one
	char *p = strrchr(buffer, ' ');

	if (p)
		*p = '\0';

	// Log it
	dprintf("Kd > %s\n", buffer);

	// Execute the debugger command
	KdState.DebugControl2->Execute(DEBUG_OUTCTL_THIS_CLIENT, buffer, DEBUG_EXECUTE_DEFAULT);

	return STATUS_CONTINUE;
}

CMDRESULT KdCmdProcess(int argc, char* argv[])
{
	if (argc < 2)
	{
		dprintf("Usage: kdproc <address>\n\tSets the debugger context to a specific EPROCESS\n");
		return STATUS_ERROR;
	}

	// Convert the string to a value
	ULONG64 argval;
	sscanf(strchr(*argv, ' ') + 1, "%llX", &argval);

	// Execute the debugger command
	KdSetProcessContext(argval);

	return STATUS_CONTINUE;
}

CMDRESULT KdCmdVad(int argc, char* argv[])
{
	if (argc < 2)
	{
		dprintf("Usage: kdvad <address>\n\tFinds a virtual address descriptor for an address\n");
		return STATUS_ERROR;
	}

	// Convert the string to a value
	ULONG64 argval;
	sscanf(strchr(*argv, ' ') + 1, "%llX", &argval);

	// Execute the debugger command
	dprintf("VAD: 0x%08llX\n", KdVadForProcessAddress(NtKdCurrrentProcess(), argval));

	return STATUS_CONTINUE;
}

CMDRESULT KdCmdRun(int argc, char *argv[])
{
	// Update status
	KdState.m_WasLastSingleStep = false;

	GuiSetDebugState(running);
	KdState.DebugControl2->SetExecutionStatus(DEBUG_STATUS_GO);

	return STATUS_CONTINUE;
}

CMDRESULT KdCmdStepInto(int argc, char *argv[])
{
	// Update status
	KdState.m_WasLastSingleStep = true;

	GuiSetDebugState(running);
	KdState.DebugControl2->SetExecutionStatus(DEBUG_STATUS_STEP_INTO);

	return STATUS_CONTINUE;
}

CMDRESULT KdCmdStepOver(int argc, char *argv[])
{
	// Update status
	KdState.m_WasLastSingleStep = true;

	GuiSetDebugState(running);
	KdState.DebugControl2->SetExecutionStatus(DEBUG_STATUS_STEP_OVER);

	return STATUS_CONTINUE;
}