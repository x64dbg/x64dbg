#pragma once

typedef struct _KSERVICE_TABLE_DESCRIPTOR
{
	ULONG_PTR	ServiceTableBase;		// SSDT (System Service Dispatch Table) base address
	ULONG_PTR	ServiceCounterTableBase;// For checked builds, contains the number of calls each service in SSDT is
	ULONG		NumberOfService;		// The number of service functions, (NumberOfService * sizeof(ULONG)) is the address table size
	ULONG_PTR	ParamTableBase;			// SSPT (System Service Parameter Table) base address
} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

bool	KdSSDTUpdate		();
ULONG64 KdSSDTGetPointer	(ULONG64 TableBase, ULONG64 TableEntry);
bool	KdSSDTEnumerate		(ULONG TableIndex, bool Shadow, bool(* Callback)(ULONG Index, ULONG64 Entry));