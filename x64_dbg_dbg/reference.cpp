#include "reference.h"
#include "debugger.h"
#include "memory.h"
#include "console.h"
#include "module.h"

int reffind(uint addr, uint size, CBREF cbRef, void* userinfo, bool silent, const char* name)
{
    uint regionSize = 0;
    uint regionBase = MemFindBaseAddr(addr, &regionSize, true);

	// If the memory page wasn't found, fail
    if(!regionBase || !regionSize)
    {
        if(!silent)
            dprintf("Invalid memory page 0x%p", addr);

        return 0;
    }

	// Assume the entire range is used
	uint scanStart	= regionBase;
	uint scanSize	= regionSize;

	// Otherwise use custom boundaries if size was supplied
	if (size)
	{
        uint maxsize = size - (addr - regionBase);

		// Make sure the size fits in one page
		scanStart	= addr;
		scanSize	= min(size, maxsize);
    }

	// Allocate and read a buffer from the remote process
    Memory<unsigned char*> data(scanSize, "reffind:data");

    if(!MemRead((PVOID)scanStart, data, scanSize, nullptr))
    {
        if(!silent)
            dprintf("Error reading memory in reference search\n");

        return 0;
    }

	// Determine the full module name
	char fullName[deflen];
	char moduleName[MAX_MODULE_SIZE];

	if (ModNameFromAddr(scanStart, moduleName, true))
		sprintf_s(fullName, "%s (%s)", name, moduleName);
	else
		sprintf_s(fullName, "%s (%p)", name, scanStart);

	// Initialize the disassembler
	DISASM disasm;
	memset(&disasm, 0, sizeof(disasm));

#ifdef _WIN64
	disasm.Archi = 64;
#endif // _WIN64
	disasm.EIP			= (UIntPtr)data;
	disasm.VirtualAddr	= (UInt64)scanStart;

	// Allow an "initialization" notice
	REFINFO refInfo;
	refInfo.refcount	= 0;
	refInfo.userinfo	= userinfo;
	refInfo.name		= fullName;

    cbRef(0, 0, &refInfo);

	//concurrency::parallel_for(uint(0), scanSize, [&](uint i)
	for (uint i = 0; i < scanSize;)
	{
		// Print the progress every 4096 bytes
        if((i % 0x1000) == 0)
        {
            float percent = (float)i / (float)scanSize;
            GuiReferenceSetProgress((int)(percent * 100.0f));
        }

		// Disassemble the instruction
        int len = Disasm(&disasm);

        if(len != UNKNOWN_OPCODE)
        {
			BASIC_INSTRUCTION_INFO basicinfo;
            fillbasicinfo(&disasm, &basicinfo);
            basicinfo.size = len;

            if(cbRef(&disasm, &basicinfo, &refInfo))
				refInfo.refcount++;
        }
		else
		{
			// Invalid instruction detected, so just skip the byte
			len = 1;
		}

		disasm.EIP			+= len;
		disasm.VirtualAddr	+= len;
		i					+= len;
    }

    GuiReferenceSetProgress(100);
    GuiReferenceReloadData();
    return refInfo.refcount;
}
