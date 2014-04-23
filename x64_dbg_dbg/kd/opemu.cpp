#include "../console.h"
#include "stdafx.h"

/*--

Module Name:
opemu.cpp

Purpose:
Emulation of certain opcodes because of WinDbg's inability to debug them.
Some opcodes like 'swapgs' cause a processor fault and it stops executing.

NOTE: This can't be fixed for certain opcodes (swapgs, iretXX)

--*/

#define READ_BIT_RANGE(value, start, end) (((value) >> (start)) & ((1 << (end - start)) - 1))

#define OPCODE_SWAPGS ((PBYTE)"\x0F\x01\xF8")
#define OPCODE_SYSRET ((PBYTE)"\x48\x0F\x07")

#define DRIVER_VM_NAME	"KDebuggerExt"
#define DRIVER_VM_SYM	"kdext"

bool KdOpEmuLoad()
{
#define MAKESYM(base, sym)		static ULONG64 __z ##sym; if (KdFindSymbol(#base "!" #sym, &__z ##sym)) KdSymbols.Set(#base, #sym, __z ##sym); else return false;

	// First detect if the driver is loaded or not
	// ...

	// Load the driver exports from it
	MAKESYM(kdext, KDE_X64Emu_Swapgs);
	MAKESYM(kdext, KDE_X64Emu_ReturnToDbg);
	MAKESYM(kdext, KDE_Driver_Swapgs);

	// Set the global state variable
	KdState.OS.OpcodeEmulation = true;

#undef MAKESYM

	return true;
}

bool OpCheckForEmulation(KD_CONTEXT *Context)
{
	// A size of 4 is good enough for now
	// though X64 can have an instruction length max of 16
	char buf[4];

	if (!KdMemRead(Context->Cip, buf, sizeof(buf), nullptr))
		return false;

	if (!memcmp(buf, OPCODE_SWAPGS, 3))
		return OpEmuSwapgs(Context);

	if (!memcmp(buf, OPCODE_SYSRET, 3))
		return OpEmuSysret(Context);

	return false;
}

bool OpEmuSwapgs(KD_CONTEXT *Context)
{
	/*
	ULONG64 IA32_GS_BASE		= KdMsrRead(MSR_IA32_GS_BASE);
	ULONG64 IA32_KERNEL_GS_BASE = KdMsrRead(MSR_IA32_KERNEL_GS_BASE);

	if (KdState.m_DebugOutput)
	{
		dprintf("Emulating SWAPGS (0x%08llX -> 0x%08llX)\n", Context->Cip, Context->Cip + 0x3);

		dprintf("MSR[0x%X] = 0x%08llX\n", MSR_IA32_GS_BASE, IA32_GS_BASE);
		dprintf("MSR[0x%X] = 0x%08llX\n", MSR_IA32_KERNEL_GS_BASE, IA32_KERNEL_GS_BASE);
	}*/

	/*
	HANDLED IN KERNELMODE ONLY

	Code
	---------
		0F 01 F8

	Operation
	---------
		if mode != 64 then #UD;
		if CPL != 0 then #GP (0);

		temp = GS base;
		GS base = MSR_KernelGSbase;
		MSR_KernelGSbase = temp;

		ip = ip + 3H;
	*/

	//ULONG64 temp = IA32_GS_BASE;
	//KdMsrSet(MSR_IA32_GS_BASE, IA32_KERNEL_GS_BASE);
	//KdMsrSet(MSR_IA32_KERNEL_GS_BASE, temp);
	//Context->Cip += 3;

	// Modify IP to point to helper driver
	//Context->Cip = KdSymbols["kdext"]["KDE_X64Emu_Swapgs"];

	return true;
}

bool OpEmuSyscall(KD_CONTEXT *Context)
{
	/*
	HANDLED IN USERMODE ONLY

	Code
	---------

	Operation
	---------
	*/

	return false;
}

bool OpEmuSysret(KD_CONTEXT *Context)
{
	IA32_STAR_BITS IA32_STAR	= { KdMsrRead(MSR_IA32_STAR) };
	IA32_FMASK_BITS IA32_FMASK	= { KdMsrRead(MSR_IA32_FMASK) };

	if (KdState.m_DebugOutput)
	{
		dprintf("Emulating SYSRET (0x%08llX -> 0x%08llX)\n", Context->Cip, Context->Ccx);

		dprintf("MSR[0x%X] = 0x%08llX\n", MSR_IA32_STAR, IA32_STAR.All);
		dprintf("MSR[0x%X] = 0x%08llX\n", MSR_IA32_FMASK, IA32_FMASK.All);
	}

	/*
	HANDLED IN KERNELMODE ONLY

	Code
	---------
		48 0F 07

	Operation
	---------
		(* Some code stripped out *)

		RIP = RCX;

		(* incorrect? *) RFLAGS = (R11 & 3C7FD7H) | 2;
		RFLAGS = (R11 & IA32_FMASK);

		CS.Selector = IA32_STAR[63:48]+16
		CS.Selector = CS.Selector OR 3

		CPL = 3

		SS.Selector = (IA32_STAR[63:48]+8) OR 3
	*/

	// Not happening without this critical piece
	// CPL = 3;

	Context->Cip = Context->Ccx;
	Context->Eflags = (Context->R11 & IA32_FMASK.SyscallFlagMask);//(Context->R11 & 0x3C7FD7) | 2;

	Context->CS = (IA32_STAR.SysretCS + 16) | 3;
	Context->SS = (IA32_STAR.SysretCS + 8) | 3;

	return true;
}

bool OpEmuSysenter(KD_CONTEXT *Context)
{
	/*
	HANDLED IN USERMODE? ONLY

	Code
	---------

	Operation
	---------
	*/

	return false;
}

bool OpEmuSysexit(KD_CONTEXT *Context)
{
	/*
	HANDLED IN KERNELMODE ONLY

	Code
	---------

	Operation
	---------
	*/

	return false;
}