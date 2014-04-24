#include "../console.h"
#include "stdafx.h"

/*--

Module Name:
sym.cpp

Purpose:
Symbol lookups.

--*/

bool KdFindSymbol(const char *Symbol, PULONG64 Offset)
{
	if (!SUCCEEDED(KdState.DebugSymbols->GetOffsetByName(Symbol, Offset)))
	{
		dprintf("Failed to find symbol '%s'\n", Symbol);
		return false;
	}

	if (KdState.m_DebugOutput)
		dprintf("%s: 0x%08llX\n", Symbol, *Offset);

	return true;
}

bool KdFindField(const char *Symbol, const char *Field, PULONG Offset)
{
	ULONG typeId;

	if (!SUCCEEDED(KdState.DebugSymbols->GetTypeId(NtKdKernelBase(), Symbol, &typeId)))
	{
		dprintf("Failed to get type id for '%s'\n", Symbol);
		return false;
	}

	if (!SUCCEEDED(KdState.DebugSymbols->GetFieldOffset(NtKdKernelBase(), typeId, Field, Offset)))
	{
		dprintf("Failed to find field '%s' for symbol '%s'\n", Field, Symbol);
		return false;
	}

	if (KdState.m_DebugOutput)
		dprintf("%s.%s: 0x%X\n", Symbol, Field, *Offset);

	return true;
}

bool KdFindFieldBitType(const char *Type, PULONG BitStart, PULONG BitSize)
{
	ULONG typeId;

	if (!SUCCEEDED(KdState.DebugSymbols->GetSymbolTypeId(Type, &typeId, nullptr)))
	{
		dprintf("Failed to get type id for '%s'\n", Type);
		return false;
	}

	if (BitStart && !SymGetTypeInfo(KdState.SymbolHandle, NtKdKernelBase(), typeId, TI_GET_BITPOSITION, BitStart))
		return false;

	if (BitSize && !SymGetTypeInfo(KdState.SymbolHandle, NtKdKernelBase(), typeId, TI_GET_LENGTH, BitSize))
		return false;

	return true;
}

bool KdGetFieldSize(const char *Symbol, PULONG Size)
{
	ULONG typeId;

	if (!SUCCEEDED(KdState.DebugSymbols->GetTypeId(NtKdKernelBase(), Symbol, &typeId)))
	{
		dprintf("Failed to get type id for '%s'\n", Symbol);
		return false;
	}

	if (!SUCCEEDED(KdState.DebugSymbols->GetTypeSize(NtKdKernelBase(), typeId, Size)))
	{
		dprintf("Failed to get type size for symbol '%s'\n", Symbol);
		return false;
	}

	return true;
}

bool KdGetRegIndex(const char *Register, PULONG Index)
{
	// Set it by default
	*Index = KDREG_INVALID;

	// IDebugRegisters::GetIndexByName was somewhat unreliable in my testing at least
	ULONG registerCount = 0;

	if (!SUCCEEDED(KdState.DebugRegisters->GetNumberRegisters(&registerCount)))
	{
		dprintf("Failed to get register count for machine\n");
		return false;
	}

	for (ULONG i = 0; i < registerCount; i++)
	{
		char name[64];
		memset(name, 0, sizeof(name));

		if (!SUCCEEDED(KdState.DebugRegisters->GetDescription(i, name, ARRAYSIZE(name), nullptr, nullptr)))
		{
			dprintf("Failed to query register index %d\n", i);
			break;
		}

		if (_stricmp(name, Register))
			continue;

		if (KdState.m_DebugOutput)
			dprintf("Register '%s' maps to index %d\n", Register, i);

		*Index = i;
		return true;
	}

	return false;
}