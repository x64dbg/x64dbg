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
#include "threading.h"
#include "exhandlerinfo.h"
#include "symbolinfo.h"
#include "debugger.h"
#include "dbghelp_safe.h"

using SehMap = std::unordered_map<duint, STACK_COMMENT>;
static SehMap SehCache;
bool ShowSuspectedCallStack;

void stackupdateseh()
{
    SehMap newcache;
    std::vector<duint> SEHList;
    if(ExHandlerGetSEH(SEHList))
    {
        STACK_COMMENT comment;
        strcpy_s(comment.color, "!sehclr"); // Special token for SEH chain color.
        auto count = SEHList.size();
        for(duint i = 0; i < count; i++)
        {
            if(i + 1 != count)
                sprintf_s(comment.comment, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Pointer to SEH_Record[%d]")), i + 1);
            else
                sprintf_s(comment.comment, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "End of SEH Chain")));
            newcache.insert({ SEHList[i], comment });
        }
    }
    EXCLUSIVE_ACQUIRE(LockSehCache);
    SehCache = std::move(newcache);
}

template<size_t _Count>
static void getSymAddrName(duint addr, char(& str)[_Count])
{
    BRIDGE_ADDRINFO addrinfo;
    if(addr == 0)
    {
        memcpy(str, "???", 4);
        return;
    }
    addrinfo.flags = flaglabel | flagmodule;
    _dbg_addrinfoget(addr, SEG_DEFAULT, &addrinfo);
    if(addrinfo.module[0] != '\0')
        _snprintf_s(str, _TRUNCATE, "%s.", addrinfo.module);
    if(addrinfo.label[0] == '\0')
        _snprintf_s(addrinfo.label, _TRUNCATE, "%p", addr);
    strncat_s(str, addrinfo.label, _TRUNCATE);
}

bool stackcommentget(duint addr, STACK_COMMENT* comment)
{
    SHARED_ACQUIRE(LockSehCache);
    const auto found = SehCache.find(addr);
    if(found != SehCache.end())
    {
        *comment = found->second;
        return true;
    }
    SHARED_RELEASE();

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
        char returnToAddr[MAX_LABEL_SIZE] = "";
        getSymAddrName(data, returnToAddr);

        data = basicinfo.addr;
        char returnFromAddr[MAX_LABEL_SIZE] = "";
        getSymAddrName(data, returnFromAddr);
        _snprintf_s(comment->comment, _TRUNCATE, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "return to %s from %s")), returnToAddr, returnFromAddr);
        strcpy_s(comment->color, "!rtnclr"); // Special token for return address color;
        return true;
    }

    //string
    char string[MAX_STRING_SIZE] = "";
    if(DbgGetStringAt(data, string))
    {
        strncpy_s(comment->comment, string, _TRUNCATE);
        return true;
    }

    //label
    char label[MAX_LABEL_SIZE] = "";
    BRIDGE_ADDRINFO addrinfo;
    addrinfo.flags = flaglabel;
    if(_dbg_addrinfoget(data, SEG_DEFAULT, &addrinfo))
        strcpy_s(label, addrinfo.label);
    char module[MAX_MODULE_SIZE] = "";
    ModNameFromAddr(data, module, false);

    if(*module) //module
    {
        if(*label) //+label
            sprintf_s(comment->comment, "%s.%s", module, label);
        else //module only
            sprintf_s(comment->comment, "%s.%p", module, data);
        return true;
    }
    else if(*label) //label only
    {
        sprintf_s(comment->comment, "<%s>", label);
        return true;
    }

    return false;
}

static BOOL CALLBACK StackReadProcessMemoryProc64(HANDLE hProcess, DWORD64 lpBaseAddress, PVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead)
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

static PVOID CALLBACK StackSymFunctionTableAccess64(HANDLE hProcess, DWORD64 AddrBase)
{
#ifdef _WIN64
    // https://github.com/dotnet/coreclr/blob/master/src/unwinder/amd64/dbs_stack_x64.cpp
    MODINFO* info = ModInfoFromAddr(AddrBase);
    if(!info)
        return nullptr;

    DWORD rva = DWORD(AddrBase - info->base);
    auto found = std::lower_bound(info->runtimeFunctions.begin(), info->runtimeFunctions.end(), rva, [](const RUNTIME_FUNCTION & a, const DWORD & rva)
    {
        return a.EndAddress <= rva;
    });

    if(found != info->runtimeFunctions.end() && rva >= found->BeginAddress)
        return &found->BeginAddress;
#endif // _WIN64

    return SymFunctionTableAccess64(hProcess, AddrBase);
}

static DWORD64 CALLBACK StackGetModuleBaseProc64(HANDLE hProcess, DWORD64 Address)
{
    return (DWORD64)ModBaseFromAddr((duint)Address);
}

static DWORD64 CALLBACK StackTranslateAddressProc64(HANDLE hProcess, HANDLE hThread, LPADDRESS64 lpaddr)
{
    ASSERT_ALWAYS("This function should never be called");
    return 0;
}

void StackEntryFromFrame(CALLSTACKENTRY* Entry, duint Address, duint From, duint To)
{
    Entry->addr = Address;
    Entry->from = From;
    Entry->to = To;

    /* https://github.com/x64dbg/x64dbg/pull/1478
    char returnToAddr[MAX_LABEL_SIZE] = "";
    getSymAddrName(To, returnToAddr);
    char returnFromAddr[MAX_LABEL_SIZE] = "";
    getSymAddrName(From, returnFromAddr);
    _snprintf_s(Entry->comment, _TRUNCATE, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "return to %s from %s")), returnToAddr, returnFromAddr);
    */

    getSymAddrName(From, Entry->comment);
}

#define MAX_CALLSTACK_CACHE 20
using CallstackMap = std::unordered_map<duint, std::vector<CALLSTACKENTRY>>;
static CallstackMap CallstackCache;

void stackupdatecallstack(duint csp)
{
    std::vector<CALLSTACKENTRY> callstack;
    stackgetcallstack(csp, callstack, false);
}

static void stackgetsuspectedcallstack(duint csp, std::vector<CALLSTACKENTRY> & callstackVector)
{
    duint size;
    duint base = MemFindBaseAddr(csp, &size);
    if(!base)
        return;
    duint end = base + size;
    size = end - csp;
    Memory<duint*> stackdata(size);
    MemRead(csp, stackdata(), size);
    for(duint i = csp; i < end; i += sizeof(duint))
    {
        duint data = stackdata()[(i - csp) / sizeof(duint)];
        duint size = 0;
        duint base = MemFindBaseAddr(data, &size);
        duint readStart = data - 16 * 4;
        if(readStart < base)
            readStart = base;
        unsigned char disasmData[256];
        if(base != 0 && size != 0 && MemRead(readStart, disasmData, sizeof(disasmData)))
        {
            duint prev = disasmback(disasmData, 0, sizeof(disasmData), data - readStart, 1);
            duint previousInstr = readStart + prev;

            BASIC_INSTRUCTION_INFO basicinfo;
            bool valid = disasmfast(disasmData + prev, previousInstr, &basicinfo);
            if(valid && basicinfo.call)
            {
                CALLSTACKENTRY stackframe;
                stackframe.addr = i;
                stackframe.to = data;
                char returnToAddr[MAX_LABEL_SIZE] = "";
                getSymAddrName(data, returnToAddr);

                data = basicinfo.addr;
                char returnFromAddr[MAX_LABEL_SIZE] = "";
                getSymAddrName(data, returnFromAddr);
                _snprintf_s(stackframe.comment, _TRUNCATE, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "return to %s from %s")), returnToAddr, returnFromAddr);
                stackframe.from = data;
                callstackVector.push_back(stackframe);
            }
        }
    }
}

void stackgetcallstack(duint csp, std::vector<CALLSTACKENTRY> & callstackVector, bool cache)
{
    if(cache)
    {
        SHARED_ACQUIRE(LockCallstackCache);
        auto found = CallstackCache.find(csp);
        if(found != CallstackCache.end())
        {
            callstackVector = found->second;
            return;
        }
        callstackVector.clear();
        return;
    }

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

    if(ShowSuspectedCallStack)
    {
        stackgetsuspectedcallstack(csp, callstackVector);
    }
    else
    {
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
        frame.AddrFrame.Offset = context.Rbp;
        frame.AddrFrame.Mode = AddrModeFlat;
        frame.AddrStack.Offset = csp;
        frame.AddrStack.Mode = AddrModeFlat;
#endif

        const int MaxWalks = 50;
        // Container for each callstack entry (50 pre-allocated entries)
        callstackVector.clear();
        callstackVector.reserve(MaxWalks);

        for(auto i = 0; i < MaxWalks; i++)
        {
            if(!SafeStackWalk64(
                        machineType,
                        fdProcessInfo->hProcess,
                        hActiveThread,
                        &frame,
                        &context,
                        StackReadProcessMemoryProc64,
                        StackSymFunctionTableAccess64,
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

                StackEntryFromFrame(&entry, (duint)frame.AddrFrame.Offset + sizeof(duint), (duint)frame.AddrPC.Offset, (duint)frame.AddrReturn.Offset);
                callstackVector.push_back(entry);
            }
            else
            {
                // Base reached
                break;
            }
        }
    }

    EXCLUSIVE_ACQUIRE(LockCallstackCache);
    if(CallstackCache.size() > MAX_CALLSTACK_CACHE)
        CallstackCache.clear();
    CallstackCache[csp] = callstackVector;
}

void stackgetcallstack(duint csp, CALLSTACK* callstack)
{
    std::vector<CALLSTACKENTRY> callstackVector;
    stackgetcallstack(csp, callstackVector, true);

    // Convert to a C data structure
    callstack->total = (int)callstackVector.size();

    if(callstack->total > 0)
    {
        callstack->entries = (CALLSTACKENTRY*)BridgeAlloc(callstack->total * sizeof(CALLSTACKENTRY));

        // Copy data directly from the vector
        memcpy(callstack->entries, callstackVector.data(), callstack->total * sizeof(CALLSTACKENTRY));
    }
}

void stackupdatesettings()
{
    ShowSuspectedCallStack = settingboolget("Engine", "ShowSuspectedCallStack");
    std::vector<CALLSTACKENTRY> dummy;
    if(hActiveThread)
        stackgetcallstack(GetContextDataEx(hActiveThread, UE_CSP), dummy, false);
}
