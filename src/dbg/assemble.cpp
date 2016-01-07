/**
 @file assemble.cpp

 @brief Implements the assemble class.
 */

#include "assemble.h"
#include "memory.h"
#include "XEDParse\XEDParse.h"
#include "value.h"
#include "disasm_fast.h"
#include "debugger.h"
#include "disasm_helper.h"
#include "memory.h"

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

static bool isInstructionPointingToExMemory(duint addr, const unsigned char* dest)
{
    BASIC_INSTRUCTION_INFO basicinfo;
    // Check if the instruction changes CIP and if it does not pretent it does point to valid executable memory.
    if(!disasmfast(dest, addr, &basicinfo) || !basicinfo.branch)
        return true;

    // An instruction pointing to invalid memory does not point to executable memory.
    if(!MemIsValidReadPtr(basicinfo.addr))
        return false;

    // Check if memory region is marked as executable
    if(MemIsCodePage(basicinfo.addr, false))
        return true;

#ifndef _WIN64
    DWORD lpFlagsDep;
    BOOL bPermanentDep;

    // DEP is disabled if lpFlagsDep == 0
    typedef BOOL(WINAPI * GETPROCESSDEPPOLICY)(
        _In_  HANDLE  hProcess,
        _Out_ LPDWORD lpFlags,
        _Out_ PBOOL   lpPermanent
    );
    static auto GPDP = GETPROCESSDEPPOLICY(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetProcessDEPPolicy"));

    // If DEP is disabled it doesn't matter where the memory points because it's executable anyway.
    if(GPDP && GPDP(fdProcessInfo->hProcess, &lpFlagsDep, &bPermanentDep) && lpFlagsDep == 0)
        return true;
#endif //_WIN64

    return false;
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
    if(!isInstructionPointingToExMemory(addr, dest))
        GuiDisplayWarning("Non-executable memory region", "Assembled branch does not point to an executable memory region!");

    bool ret = MemPatch(addr, dest, destSize);

    if(ret)
    {
        if(fillnop && nopsize)
        {
            if(size)
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