/**
 @file stackinfo.cpp

 @brief Implements the stackinfo class.
 */

#include "stackinfo.h"
#include "memory.h"
#include "disasm_helper.h"
#include "disasm_fast.h"
#include "_exports.h"
#include "module.h"
#include "thread.h"

bool stackcommentget(duint addr, STACK_COMMENT* comment)
{
    duint data = 0;
    memset(comment, 0, sizeof(STACK_COMMENT));
    MemRead(addr, &data, sizeof(duint));
    if(!MemIsValidReadPtr(data)) //the stack value is no pointer
        return false;

    duint size = 0;
    duint base = MemFindBaseAddr(data, &size);
    duint readStart = data - 16 * 4;
    if(readStart < base)
        readStart = base;
    unsigned char disasmData[256];
    MemRead(readStart, disasmData, sizeof(disasmData));
    duint prev = disasmback(disasmData, 0, sizeof(disasmData), data - readStart, 1);
    duint previousInstr = readStart + prev;

    BASIC_INSTRUCTION_INFO basicinfo;
    bool valid = disasmfast(disasmData + prev, previousInstr, &basicinfo);
    if(valid && basicinfo.call) //call
    {
        char label[MAX_LABEL_SIZE] = "";
        ADDRINFO addrinfo;
        addrinfo.flags = flaglabel;
        if(_dbg_addrinfoget(data, SEG_DEFAULT, &addrinfo))
            strcpy_s(label, addrinfo.label);
        char module[MAX_MODULE_SIZE] = "";
        ModNameFromAddr(data, module, false);
        char returnToAddr[MAX_COMMENT_SIZE] = "";
        if(*module)
            sprintf(returnToAddr, "%s.", module);
        if(!*label)
            sprintf(label, fhex, data);
        strcat(returnToAddr, label);

        data = basicinfo.addr;
        if(data)
        {
            *label = 0;
            addrinfo.flags = flaglabel;
            if(_dbg_addrinfoget(data, SEG_DEFAULT, &addrinfo))
                strcpy_s(label, addrinfo.label);
            *module = 0;
            ModNameFromAddr(data, module, false);
            char returnFromAddr[MAX_COMMENT_SIZE] = "";
            if(*module)
                sprintf(returnFromAddr, "%s.", module);
            if(!*label)
                sprintf(label, fhex, data);
            strcat_s(returnFromAddr, label);
            sprintf_s(comment->comment, "return to %s from %s", returnToAddr, returnFromAddr);
        }
        else
            sprintf_s(comment->comment, "return to %s from ???", returnToAddr);
        strcpy_s(comment->color, "#ff0000");
        return true;
    }

    //string
    STRING_TYPE strtype;
    char string[512] = "";
    if(disasmgetstringat(data, &strtype, string, string, 500))
    {
        if(strtype == str_ascii)
            sprintf(comment->comment, "\"%s\"", string);
        else //unicode
            sprintf(comment->comment, "L\"%s\"", string);
        return true;
    }

    //label
    char label[MAX_LABEL_SIZE] = "";
    ADDRINFO addrinfo;
    addrinfo.flags = flaglabel;
    if(_dbg_addrinfoget(data, SEG_DEFAULT, &addrinfo))
        strcpy_s(label, addrinfo.label);
    char module[MAX_MODULE_SIZE] = "";
    ModNameFromAddr(data, module, false);

    if(*module) //module
    {
        if(*label) //+label
            sprintf(comment->comment, "%s.%s", module, label);
        else //module only
            sprintf(comment->comment, "%s." fhex, module, data);
        return true;
    }
    else if(*label) //label only
    {
        sprintf(comment->comment, "<%s>", label);
        return true;
    }

    return false;
}

BOOL CALLBACK StackReadProcessMemoryProc64(HANDLE hProcess, DWORD64 lpBaseAddress, PVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead)
{
    // Fix for 64-bit sizes
    SIZE_T bytesRead = 0;

    if(MemRead((duint)lpBaseAddress, lpBuffer, nSize, &bytesRead))
    {
        if(lpNumberOfBytesRead)
            *lpNumberOfBytesRead = (DWORD)bytesRead;

        return true;
    }

    return false;
}

DWORD64 CALLBACK StackGetModuleBaseProc64(HANDLE hProcess, DWORD64 Address)
{
    return (DWORD64)ModBaseFromAddr((duint)Address);
}

DWORD64 CALLBACK StackTranslateAddressProc64(HANDLE hProcess, HANDLE hThread, LPADDRESS64 lpaddr)
{
    ASSERT_ALWAYS("This function should never be called");
    return 0;
}

void StackEntryFromFrame(CALLSTACKENTRY* Entry, duint Address, duint From, duint To)
{
    Entry->addr = Address;
    Entry->from = From;
    Entry->to = To;

    char label[MAX_LABEL_SIZE] = "";
    ADDRINFO addrinfo;
    addrinfo.flags = flaglabel;
    if(_dbg_addrinfoget(Entry->to, SEG_DEFAULT, &addrinfo))
        strcpy_s(label, addrinfo.label);
    char module[MAX_MODULE_SIZE] = "";
    ModNameFromAddr(Entry->to, module, false);
    char returnToAddr[MAX_COMMENT_SIZE] = "";
    if(*module)
        sprintf(returnToAddr, "%s.", module);
    if(!*label)
        sprintf(label, fhex, Entry->to);
    strcat(returnToAddr, label);

    if(Entry->from)
    {
        *label = 0;
        addrinfo.flags = flaglabel;
        if(_dbg_addrinfoget(Entry->from, SEG_DEFAULT, &addrinfo))
            strcpy_s(label, addrinfo.label);
        *module = 0;
        ModNameFromAddr(Entry->from, module, false);
        char returnFromAddr[MAX_COMMENT_SIZE] = "";
        if(*module)
            sprintf(returnFromAddr, "%s.", module);
        if(!*label)
            sprintf(label, fhex, Entry->from);
        strcat(returnFromAddr, label);

        sprintf_s(Entry->comment, "return to %s from %s", returnToAddr, returnFromAddr);
    }
    else
        sprintf_s(Entry->comment, "return to %s from ???", returnToAddr);
}

void stackgetcallstack(duint csp, CALLSTACK* callstack)
{
    // Gather context data
    CONTEXT context;
    memset(&context, 0, sizeof(CONTEXT));

    context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;

    if(SuspendThread(hActiveThread) == -1)
        return;

    if(!GetThreadContext(hActiveThread, &context))
        return;

    if(ResumeThread(hActiveThread) == -1)
        return;

    // Set up all frame data
    STACKFRAME64 frame;
    ZeroMemory(&frame, sizeof(STACKFRAME64));

#ifdef _M_IX86
    DWORD machineType = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset = context.Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = csp;
    frame.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset = context.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rsp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = csp;
    frame.AddrStack.Mode = AddrModeFlat;
#endif

    // Container for each callstack entry (20 pre-allocated entries)
    std::vector<CALLSTACKENTRY> callstackVector;
    callstackVector.reserve(20);

    while(true)
    {
        if(!StackWalk64(
                    machineType,
                    fdProcessInfo->hProcess,
                    hActiveThread,
                    &frame,
                    &context,
                    StackReadProcessMemoryProc64,
                    SymFunctionTableAccess64,
                    StackGetModuleBaseProc64,
                    StackTranslateAddressProc64))
        {
            // Maybe it failed, maybe we have finished walking the stack
            break;
        }

        if(frame.AddrPC.Offset != 0)
        {
            // Valid frame
            CALLSTACKENTRY entry;
            memset(&entry, 0, sizeof(CALLSTACKENTRY));

            StackEntryFromFrame(&entry, (duint)frame.AddrFrame.Offset, (duint)frame.AddrReturn.Offset, (duint)frame.AddrPC.Offset);
            callstackVector.push_back(entry);
        }
        else
        {
            // Base reached
            break;
        }
    }

    // Convert to a C data structure
    callstack->total = (int)callstackVector.size();

    if(callstack->total > 0)
    {
        callstack->entries = (CALLSTACKENTRY*)BridgeAlloc(callstack->total * sizeof(CALLSTACKENTRY));

        // Copy data directly from the vector
        memcpy(callstack->entries, callstackVector.data(), callstack->total * sizeof(CALLSTACKENTRY));
    }
}