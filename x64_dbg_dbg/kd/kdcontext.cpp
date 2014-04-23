#include "stdafx.h"
#include "../stackinfo.h"

/*--

Module Name:
kdcontext.cpp

Purpose:
Reading and writing registers on the client machine.
This uses custom context structures.

--*/

bool KdContextLoadSymbols()
{
#define MAKEREG(reg, idx) KdGetRegIndex(#reg, &KdState.OS.RegisterIds[idx]);

	//
	// Registers
	//
	MAKEREG(eax, 0);
	MAKEREG(ebx, 1);
	MAKEREG(ecx, 2);
	MAKEREG(edx, 3);
	MAKEREG(esi, 4);
	MAKEREG(edi, 5);
	MAKEREG(ebp, 6);
	MAKEREG(esp, 7);
	MAKEREG(eip, 8);

	// X64 ONLY
	if (KdState.OS.Ptr64)
	{
		MAKEREG(rax, 0);
		MAKEREG(rbx, 1);
		MAKEREG(rcx, 2);
		MAKEREG(rdx, 3);
		MAKEREG(rsi, 4);
		MAKEREG(rdi, 5);
		MAKEREG(rbp, 6);
		MAKEREG(rsp, 7);
		MAKEREG(rip, 8);

		MAKEREG(r8, 9);
		MAKEREG(r9, 10);
		MAKEREG(r10, 11);
		MAKEREG(r11, 12);
		MAKEREG(r12, 13);
		MAKEREG(r13, 14);
		MAKEREG(r14, 15);
		MAKEREG(r15, 16);
	}
	// --------

	MAKEREG(efl, 17);

	MAKEREG(cs, 18);
	MAKEREG(ds, 19);
	MAKEREG(es, 20);
	MAKEREG(fs, 21);
	MAKEREG(gs, 22);
	MAKEREG(ss, 23);

	MAKEREG(dr0, 24);
	MAKEREG(dr1, 25);
	MAKEREG(dr2, 26);
	MAKEREG(dr3, 27);
	MAKEREG(dr6, 28);
	MAKEREG(dr7, 29);

	MAKEREG(cr0, 30);
	MAKEREG(cr2, 31);
	MAKEREG(cr3, 32);
	MAKEREG(cr4, 33);

	// X64 ONLY
	if (KdState.OS.Ptr64)
	{
		MAKEREG(cr8, 34);
	}
	// --------

#undef MAKEREG

	return true;
}

bool KdContextGet(KD_CONTEXT *Context)
{
	// Zero out any data
	memset(Context, 0, sizeof(KD_CONTEXT));

	// Don't use a memcpy, sizes may differ
	for (int i = 0; i < KDREG_MAX; i++)
	{
		DEBUG_VALUE registerVal;
		ULONG		registerId = KdState.OS.RegisterIds[i];

		// Skip invalid registers
		if (registerId == KDREG_INVALID)
			continue;
		
		if (!SUCCEEDED(KdState.DebugRegisters->GetValue(registerId, &registerVal)))
			return false;

#ifdef _WIN64
		Context->Registers[i] = registerVal.I64;
#else
		Context->Registers[i] = registerVal.I32;
#endif
	}

	return true;
}

bool KdContextSet(KD_CONTEXT *Context)
{
	// Don't use a memcpy, sizes may differ
	for (int i = 0; i < KDREG_MAX; i++)
	{
		DEBUG_VALUE registerVal;
		ULONG		registerId = KdState.OS.RegisterIds[i];

		// Skip invalid registers
		if (registerId == KDREG_INVALID)
			continue;

#ifdef _WIN64
		registerVal.I64 = Context->Registers[i];
#else
		registerVal.I32 = Context->Registers[i];
#endif

		if (!SUCCEEDED(KdState.DebugRegisters->SetValue(registerId, &registerVal)))
			return false;
	}

	return true;
}

bool KdContextToRegdump(REGDUMP *Dump)
{
	KD_CONTEXT ctx;
	if (!KdContextGet(&ctx))
		return false;

	Dump->cax = ctx.Cax;
	Dump->ccx = ctx.Ccx;
	Dump->cdx = ctx.Cdx;
	Dump->cbx = ctx.Cbx;
	Dump->csp = ctx.Csp;
	Dump->cbp = ctx.Cbp;
	Dump->csi = ctx.Csi;
	Dump->cdi = ctx.Cdi;

#ifdef _WIN64
	Dump->r8 = ctx.R8;
	Dump->r9 = ctx.R9;
	Dump->r10 = ctx.R10;
	Dump->r11 = ctx.R11;
	Dump->r12 = ctx.R12;
	Dump->r13 = ctx.R13;
	Dump->r14 = ctx.R14;
	Dump->r15 = ctx.R15;
#endif

	Dump->cip = (duint) ctx.Cip;
	Dump->eflags = (duint) ctx.Eflags;
	Dump->gs = (unsigned short) (ctx.GS & 0xFFFF);
	Dump->fs = (unsigned short) (ctx.FS & 0xFFFF);
	Dump->es = (unsigned short) (ctx.ES & 0xFFFF);
	Dump->ds = (unsigned short) (ctx.DS & 0xFFFF);
	Dump->cs = (unsigned short) (ctx.CS & 0xFFFF);
	Dump->ss = (unsigned short) (ctx.SS & 0xFFFF);
	Dump->dr0 = (duint) ctx.Dr0;
	Dump->dr1 = (duint) ctx.Dr1;
	Dump->dr2 = (duint) ctx.Dr2;
	Dump->dr3 = (duint) ctx.Dr3;
	Dump->dr6 = (duint) ctx.Dr6;
	Dump->dr7 = (duint) ctx.Dr7;

	return true;
}