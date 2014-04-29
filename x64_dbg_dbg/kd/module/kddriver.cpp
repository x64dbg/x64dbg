#include "../../console.h"
#include "../stdafx.h"

/*--

Module Name:
kddriver.cpp

Purpose:
Caching of loaded driver images in the kernel.

--*/

std::vector<DriverEntry> DriverList;

void UpdateGuiModuleList();

void KdDriverClear()
{
	DriverList.clear();
}

void KdDriverLoad(const char *Name, ULONG64 BaseAddress, ULONG64 ModuleSize)
{
	// Make sure it doesn't already exist
	for (std::vector<DriverEntry>::iterator itr = DriverList.begin(); itr != DriverList.end(); itr++)
	{
		if (itr->Base == BaseAddress && itr->Size == ModuleSize)
			return;
	}

	dprintf("Driver load: [0x%llX] %s\n", BaseAddress, Name);

	// Create an entry
	DriverEntry dr;
	strcpy_s(dr.File, Name);
	dr.Base = BaseAddress;
	dr.Size = ModuleSize;

	// Add it to the list
	DriverList.push_back(dr);

	// Update the GUI
	UpdateGuiModuleList();
}

void KdDriverUnload(ULONG64 BaseAddress)
{
	for (std::vector<DriverEntry>::iterator itr = DriverList.begin(); itr != DriverList.end(); itr++)
	{
		if (itr->Base == BaseAddress)
		{
			dprintf("Driver exit: [0x%llX] %s\n", BaseAddress, itr->File);

			DriverList.erase(itr);
			UpdateGuiModuleList();

			break;
		}
	}
}

bool KdDriverEnumerate(bool (* Callback)(DriverEntry *))
{
	for (std::vector<DriverEntry>::iterator itr = DriverList.begin(); itr != DriverList.end(); itr++)
	{
		if (!Callback(&(*itr)))
			return false;
	}

	return true;
}

DriverEntry *KdDriverGetOwnerModule(ULONG64 Address)
{
	for (std::vector<DriverEntry>::iterator itr = DriverList.begin(); itr != DriverList.end(); itr++)
	{
		// Check if it is within range
		if (Address >= itr->Base && Address <= (itr->Base + itr->Size))
			return &(*itr);
	}

	return nullptr;
}

void UpdateGuiModuleList()
{
	size_t count = DriverList.size();

	SYMBOLMODULEINFO *data = (SYMBOLMODULEINFO *)BridgeAlloc(sizeof(SYMBOLMODULEINFO) * count);

	for (size_t i = 0; i < count; i++)
	{
		data[i].base = DriverList[i].Base;
		strcpy_s(data[i].name, DriverList[i].File);
	}

	GuiSymbolUpdateModuleList(count, data);
}