#pragma once

#define KDREG_MAX		35			// Maximum number of registers stored in KD_CONTEXT
#define KDREG_INVALID	0xFFFFFFFF	// Invalid register type id

// Forward declare this instead of including the entire file
struct REGDUMP;

typedef struct _KD_CONTEXT
{
#ifdef _WIN64
	typedef ULONG64 RegType;
#else
	typedef ULONG RegType;
#endif

	union
	{
		RegType Registers[KDREG_MAX];

		struct
		{
			RegType Cax;
			RegType Cbx;
			RegType Ccx;
			RegType Cdx;
			RegType Csi;
			RegType Cdi;
			RegType Cbp;
			RegType Csp;
			RegType Cip;

			RegType R8;
			RegType R9;
			RegType R10;
			RegType R11;
			RegType R12;
			RegType R13;
			RegType R14;
			RegType R15;

			RegType Eflags;

			RegType CS;
			RegType DS;
			RegType ES;
			RegType FS;
			RegType GS;
			RegType SS;

			RegType Dr0;
			RegType Dr1;
			RegType Dr2;
			RegType Dr3;
			RegType Dr6;
			RegType Dr7;

			RegType Cr0;
			RegType Cr2;
			RegType Cr3;
			RegType Cr4;
			RegType Cr8;
		};
	};
} KD_CONTEXT, *PKD_CONTEXT;

bool KdContextLoadSymbols	();
bool KdContextGet			(KD_CONTEXT *Context);
bool KdContextSet			(KD_CONTEXT *Context);
bool KdContextToRegdump		(REGDUMP *Dump);