#pragma once

struct DriverEntry
{
	char	File[MAX_PATH];
	ULONG64 Base;
	ULONG64 Size;
};

void KdDriverLoad		(const char *Name, ULONG64 BaseAddress, ULONG64 ModuleSize);
void KdDriverUnload		(ULONG64 BaseAddress);
bool KdDriverEnumerate	(bool (* Callback)(DriverEntry *));

DriverEntry *KdDriverGetOwnerModule(ULONG64 Address);