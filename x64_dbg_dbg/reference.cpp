/**
 @file reference.cpp

 @brief Implements the reference class.
 */

#include "reference.h"
#include "memory.h"
#include "console.h"
#include "module.h"

int RefFind(uint Address, uint Size, CBREF Callback, void* UserData, bool Silent, const char* Name)
{
    uint regionSize = 0;
    uint regionBase = MemFindBaseAddr(Address, &regionSize, true);

    // If the memory page wasn't found, fail
    if(!regionBase || !regionSize)
    {
        if(!Silent)
            dprintf("Invalid memory page 0x%p", Address);

        return 0;
    }

    // Assume the entire range is used
    uint scanStart = regionBase;
    uint scanSize = regionSize;

    // Otherwise use custom boundaries if size was supplied
    if(Size)
    {
        uint maxsize = Size - (Address - regionBase);

        // Make sure the size fits in one page
        scanStart = Address;
        scanSize = min(Size, maxsize);
    }

    // Allocate and read a buffer from the remote process
    Memory<unsigned char*> data(scanSize, "reffind:data");

    if(!MemRead(scanStart, data(), scanSize))
    {
        if(!Silent)
            dprintf("Error reading memory in reference search\n");

        return 0;
    }

    // Determine the full module name
    char fullName[deflen];
    char moduleName[MAX_MODULE_SIZE];

    if(ModNameFromAddr(scanStart, moduleName, true))
        sprintf_s(fullName, "%s (%s)", Name, moduleName);
    else
        sprintf_s(fullName, "%s (%p)", Name, scanStart);

    // Initialize the disassembler
    Capstone cp;
    unsigned char* dataptr = data();

    // Allow an "initialization" notice
    REFINFO refInfo;
    refInfo.refcount = 0;
    refInfo.userinfo = UserData;
    refInfo.name = fullName;

    Callback(0, 0, &refInfo);

    //concurrency::parallel_for(uint(0), scanSize, [&](uint i)
    for(uint i = 0; i < scanSize;)
    {
        // Print the progress every 4096 bytes
        if((i % 0x1000) == 0)
        {
            // Percent = (current / total) * 100
            // Integer = floor(percent)
            int percent = (int)floor(((float)i / (float)scanSize) * 100.0f);

            GuiReferenceSetProgress(percent);
        }

        // Disassemble the instruction
        int len;
        if(cp.Disassemble(scanStart, dataptr, MAX_DISASM_BUFFER))
        {
            BASIC_INSTRUCTION_INFO basicinfo;
            fillbasicinfo(&cp, &basicinfo);

            if(Callback(&cp, &basicinfo, &refInfo))
                refInfo.refcount++;
            len = cp.Size();
        }
        else
        {
            // Invalid instruction detected, so just skip the byte
            len = 1;
        }

        dataptr += len;
        scanStart += len;
        i += len;
    }

    GuiReferenceSetProgress(100);
    GuiReferenceReloadData();
    return refInfo.refcount;
}
