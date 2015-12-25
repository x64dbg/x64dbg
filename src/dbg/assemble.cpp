/**
 @file assemble.cpp

 @brief Implements the assemble class.
 */

#include "assemble.h"
#include "memory.h"
#include "XEDParse\XEDParse.h"
#include "value.h"
#include "disasm_helper.h"
#include "debugger.h"

static bool cbUnknown(const char* text, ULONGLONG* value)
{
    if(!text || !value)
        return false;
    duint val;
    if(!valfromstring(text, &val))
        return false;
    *value = val;
    return true;
}

bool assemble(duint addr, unsigned char* dest, int* size, const char* instruction, char* error)
{
    if(strlen(instruction) >= XEDPARSE_MAXBUFSIZE)
        return false;
    XEDPARSE parse;
    memset(&parse, 0, sizeof(parse));
#ifdef _WIN64
    parse.x64 = true;
#else //x86
    parse.x64 = false;
#endif
    parse.cbUnknown = cbUnknown;
    parse.cip = addr;
    strcpy_s(parse.instr, instruction);
    if(XEDParseAssemble(&parse) == XEDPARSE_ERROR)
    {
        if(error)
            strcpy_s(error, MAX_ERROR_SIZE, parse.error);
        return false;
    }

    if(dest)
        memcpy(dest, parse.dest, parse.dest_size);
    if(size)
        *size = parse.dest_size;

    return true;
}

bool assembleat(duint addr, const char* instruction, int* size, char* error, bool fillnop)
{
    int destSize;
    unsigned char dest[16];
    if(!assemble(addr, dest, &destSize, instruction, error))
        return false;
    //calculate the number of NOPs to insert
    int origLen = disasmgetsize(addr);
    while(origLen < destSize)
        origLen += disasmgetsize(addr + origLen);
    int nopsize = origLen - destSize;
    unsigned char nops[16];
    memset(nops, 0x90, sizeof(nops));

    if(size)
        *size = destSize;

    // Check if the instruction doesn't set IP to non-executable memory
    if (isInstructionPointingToExMemory(addr, dest) == NX_MEMORY)
        GuiDisplayWarning("Non-executable memory region", "Assembled instruction points to non-executable memory region !");

    bool ret = MemPatch(addr, dest, destSize);

    if (ret)
    {
        if (fillnop && nopsize)
        {
            if (size)
                *size += nopsize;

            // Ignored if the memory patch for NOPs fail (although it should not)
            MemPatch(addr + destSize, nops, nopsize);
        }

        // Update GUI if any patching succeeded
        GuiUpdatePatches();
    }
    else
    {
        // Tell the user writing is blocked
        strcpy_s(error, MAX_ERROR_SIZE, "Error while writing process memory");
    }

    return ret;
}

INSTR_POINTING_TO isInstructionPointingToExMemory(duint addr, const unsigned char* dest)
{
    MEMMAP wMemMapStruct;
    DISASM_ARG arg;
    DISASM_INSTR wDisasInstrStruct;
    MEMORY_BASIC_INFORMATION *wMbiStruct = NULL;
    duint instrArgCount;
    dsint instrMemValues[3] = {0};
    duint instrMemValuesIndex = 0;
    dsint mbiBaseAddr, mbiEndAddr;

    disasmget((unsigned char*)dest, 0, &wDisasInstrStruct);

    instrArgCount = wDisasInstrStruct.argcount;
    if (instrArgCount)
    {
        // Loop through all arguements
        for (int i = 0; i < wDisasInstrStruct.argcount; i++)
        {
            arg = wDisasInstrStruct.arg[i];

            // Check if any of the arguments is a memory
            if (arg.type == arg_memory)
            {
                instrMemValues[instrMemValuesIndex] = addr + arg.memvalue; // add current instruction VA for rip-relative addressing
                instrMemValuesIndex++;
            }
            else if (wDisasInstrStruct.type == instr_branch)
            {
                instrMemValues[instrMemValuesIndex] = addr + arg.value;
                instrMemValuesIndex++;
            }
        }
    }

    // No memory pointer in the instruction, no need to go further
    if (!instrMemValuesIndex)
        return NO_POINTER;

    // Get memory map to locate the sections to which the instr memory address belongs to
    DbgMemMap(&wMemMapStruct);

    // For each memPointerValue
    for (auto & memValue : instrMemValues)
    {
        // Loop through the memMaps
        for (int i = 0; i < wMemMapStruct.count; i++)
        {
            wMbiStruct  = &(wMemMapStruct.page)[i].mbi;
            mbiBaseAddr = (dsint)wMbiStruct->BaseAddress;
            mbiEndAddr  = (dsint)wMbiStruct->BaseAddress + (dsint)wMbiStruct->RegionSize;
            if (memValue >= mbiBaseAddr && memValue < mbiEndAddr)
            {
                if (wMbiStruct->Protect == PAGE_EXECUTE ||
                        wMbiStruct->Protect == PAGE_EXECUTE_READ ||
                        wMbiStruct->Protect == PAGE_EXECUTE_READWRITE ||
                        wMbiStruct->Protect == PAGE_EXECUTE_WRITECOPY)
                {
                    // Memory region is marked as executable
#ifndef _WIN64
                    DWORD lpFlagsDep;
                    BOOL bPermanentDep;

                    // DEP is disabled if lpFlagsDep == 0
                    typedef BOOL (WINAPI *GETPROCESSDEPPOLICY)(
                        _In_  HANDLE  hProcess,
                        _Out_ LPDWORD lpFlags,
                        _Out_ PBOOL   lpPermanent
                    );
                    static GETPROCESSDEPPOLICY GPDP = (GETPROCESSDEPPOLICY)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetProcessDEPPolicy");
                    if (GPDP && GPDP(fdProcessInfo->hProcess, &lpFlagsDep, &bPermanentDep) && lpFlagsDep != 0)
                        return EX_MEMORY;
#else
                    // DEP enabled on x64
                    return EX_MEMORY;
#endif
                }
            }
        }
    }

    return NX_MEMORY;
}
