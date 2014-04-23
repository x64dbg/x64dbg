#include "../console.h"
#include "stdafx.h"

class PatchList
{
private:
	struct PatchEntry
	{
		ULONG64 Address;
		BYTE	Data[8];
		BYTE	OldData[8];
		ULONG	Size;
	};

	std::vector<PatchEntry> Patches;

public:
	void AddPatch(ULONG64 Address, PBYTE Buffer, ULONG Size)
	{
		PatchEntry entry;
		entry.Address = Address;
		memcpy(entry.Data, Buffer, Size);
		entry.Size = Size;

		Patches.push_back(entry);
	}

	bool WritePatches()
	{
		for (auto& entry : Patches)
		{
			// Save the old instructions
			if (!KdMemRead(entry.Address, entry.OldData, entry.Size, nullptr))
				return false;

			// Now overwrite the data
			if (!KdMemWrite(entry.Address, entry.Data, entry.Size, nullptr))
				return false;
		}

		return true;
	}
};

ULONG64 CallToAddr(ULONG64 Address)
{
	ULONG64 base	= Address;
	ULONG offset	= 0;

	if (!KdMemRead(base + 0x1, &offset, sizeof(offset), nullptr))
		return 0;

	return base + offset + 0x5;
}

bool PatchGuardLoadSymbols()
{
	PatchList pgPatches;
	ULONG64 temp;

	//
	// Attempt to enable PatchGuard for analysis
	// Irony?
	//

	// Windows 8.1 - Based off of KiFilterFiberContext
	if (KdFindSymbol("nt!KiFilterFiberContext", &temp))
	{
		// CHECK (KdDebuggerNotPresent)
		pgPatches.AddPatch(temp + 0x20, (PBYTE)"\x90", 1);			// CLI
		pgPatches.AddPatch(temp + 0x2A, (PBYTE)"\x90\x90\x90", 3);	// JMP SHORT; STI

		// Main PatchGuard initialization
		// This function is over 90000 bytes long
		ULONG64 KiInitializePatchGuard = CallToAddr(temp + 0x117);
		temp = KiInitializePatchGuard;

		if (!KiInitializePatchGuard)
			return false;

		// CHECK (KdDebuggerNotPresent)
		pgPatches.AddPatch(temp + 0x2D, (PBYTE)"\x90", 1);			// CLI
		pgPatches.AddPatch(temp + 0x38, (PBYTE)"\x90\x90\x90", 3);	// JMP SHORT; STI

		pgPatches.AddPatch(temp + 0xF6, (PBYTE)"\x90", 1);			// CLI
		pgPatches.AddPatch(temp + 0xFF, (PBYTE)"\x90\x90\x90", 3);	// JMP SHORT; STI

		pgPatches.AddPatch(temp + 0x1A0, (PBYTE)"\x90", 1);			// CLI
		pgPatches.AddPatch(temp + 0x1AB, (PBYTE)"\x90\x90\x90", 3);	// JMP SHORT; STI

		pgPatches.AddPatch(temp + 0xB66, (PBYTE)"\x90", 1);			// CLI
		pgPatches.AddPatch(temp + 0xB71, (PBYTE)"\x90\x90\x90", 3);	// JMP SHORT; STI

		pgPatches.AddPatch(temp + 0x101C, (PBYTE)"\x90", 1);		// CLI
		pgPatches.AddPatch(temp + 0x1027, (PBYTE)"\x90\x90\x90", 3);// JMP SHORT; STI

		dprintf("PG INIT IS AT 0x%llx\n", KiInitializePatchGuard);
	}

	return pgPatches.WritePatches();
}

void PGFixupInitialization()
{
	// Patch it's initialization functions
}