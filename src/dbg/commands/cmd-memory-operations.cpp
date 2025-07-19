#include "cmd-memory-operations.h"
#include "console.h"
#include "debugger.h"
#include "memory.h"
#include "variable.h"
#include "filehelper.h"
#include "value.h"
#include "stringformat.h"
#include "comment.h"

bool cbDebugAlloc(int argc, char* argv[])
{
    duint size = 0x1000, addr = 0;
    if(argc > 1)
        if(!valfromstring(argv[1], &size, false))
            return false;
    if(argc > 2)
        if(!valfromstring(argv[2], &addr, false))
            return false;
    duint mem = (duint)MemAllocRemote(addr, size);
    if(!mem)
        dputs(QT_TRANSLATE_NOOP("DBG", "VirtualAllocEx failed"));
    else
        dprintf("%p\n", mem);
    if(mem)
        varset("$lastalloc", mem, true);
    if(mem)
        CommentSet(mem, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "User-allocated memory")), true);
    //update memory map
    MemUpdateMap();
    GuiUpdateMemoryView();

    varset("$res", mem, false);
    return true;
}

bool cbDebugFree(int argc, char* argv[])
{
    duint lastalloc;
    varget("$lastalloc", &lastalloc, 0, 0);
    duint addr = lastalloc;
    if(argc > 1)
    {
        if(!valfromstring(argv[1], &addr, false))
            return false;
    }
    else if(!lastalloc)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "$lastalloc is zero, provide a page address"));
        return false;
    }
    if(addr == lastalloc)
        varset("$lastalloc", (duint)0, true);
    bool ok = !!VirtualFreeEx(fdProcessInfo->hProcess, (void*)addr, 0, MEM_RELEASE);
    if(!ok)
        dputs(QT_TRANSLATE_NOOP("DBG", "VirtualFreeEx failed"));
    //update memory map
    MemUpdateMap();
    GuiUpdateMemoryView();

    varset("$res", ok, false);
    return true;
}

bool cbDebugMemset(int argc, char* argv[])
{
    duint addr;
    duint value;
    duint size;
    if(IsArgumentsLessThan(argc, 3))
        return false;
    if(!valfromstring(argv[1], &addr, false) || !valfromstring(argv[2], &value, false))
        return false;
    if(argc > 3)
    {
        if(!valfromstring(argv[3], &size, false))
            return false;
    }
    else
    {
        duint base = MemFindBaseAddr(addr, &size, true);
        if(!base)
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Invalid address specified"));
            return false;
        }
        duint diff = addr - base;
        addr = base + diff;
        size -= diff;
    }
    BYTE fi = value & 0xFF;
    if(!Fill((void*)addr, size & 0xFFFFFFFF, &fi))
        dputs(QT_TRANSLATE_NOOP("DBG", "Memset failed"));
    else
        dprintf(QT_TRANSLATE_NOOP("DBG", "Memory %p (size: %.8X) set to %.2X\n"), addr, DWORD(size & 0xFFFFFFFF), BYTE(value & 0xFF));
    return true;
}

bool cbDebugMemcpy(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 4))
        return false;

    duint dst = 0, src = 0, size = 0;
    if(!valfromstring(argv[1], &dst, false) || !valfromstring(argv[2], &src, false) || !valfromstring(argv[3], &size, false))
        return false;

    duint totalNumberOfBytesWritten = 0;
    std::vector<uint8_t> buffer;
    buffer.resize(PAGE_SIZE); //copy a page at a time
    for(size_t i = 0; i < size; i += buffer.size())
    {
        duint NumberOfBytesRead = 0;
        auto readOk = MemRead(src + i, buffer.data(), std::min(buffer.size(), (size_t)(size - i)), &NumberOfBytesRead);
        duint NumberOfBytesWritten = 0;
        auto writeOk = MemWrite(dst + i, buffer.data(), NumberOfBytesRead, &NumberOfBytesWritten);
        totalNumberOfBytesWritten += NumberOfBytesWritten;
        if(!readOk || !writeOk)
            break;
    }
    GuiUpdateAllViews();
    varset("$result", totalNumberOfBytesWritten, false);
    return true;
}

bool cbDebugGetPageRights(int argc, char* argv[])
{
    duint addr = 0;
    char rights[RIGHTS_STRING_SIZE];

    if(argc != 2 || !valfromstring(argv[1], &addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: using an address as arg1\n"));
        return false;
    }

    if(!MemGetPageRights(addr, rights))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Error getting rights of page: %s\n"), argv[1]);
        return false;
    }

    dprintf(QT_TRANSLATE_NOOP("DBG", "Page: %p, Rights: %s\n"), addr, rights);

    return true;
}

bool cbDebugSetPageRights(int argc, char* argv[])
{
    duint addr = 0;
    char rights[RIGHTS_STRING_SIZE];

    if(argc < 3 || !valfromstring(argv[1], &addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: Using an address as arg1 and as arg2: Execute, ExecuteRead, ExecuteReadWrite, ExecuteWriteCopy, NoAccess, ReadOnly, ReadWrite, WriteCopy. You can add a G at first for add PAGE GUARD, example: GReadOnly\n"));
        return false;
    }

    if(!MemSetPageRights(addr, argv[2]))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Error: Set rights of %p with Rights: %s\n"), addr, argv[2]);
        return false;
    }

    if(!MemGetPageRights(addr, rights))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Error getting rights of page: %s\n"), argv[1]);
        return false;
    }

    //update the memory map
    MemUpdateMap();
    GuiUpdateMemoryView();

    dprintf(QT_TRANSLATE_NOOP("DBG", "New rights of %p: %s\n"), addr, rights);

    return true;
}

bool cbInstrSavedata(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 4))
        return false;
    duint addr, size;
    if(!valfromstring(argv[2], &addr, false) || !valfromstring(argv[3], &size, false))
        return false;

    bool success = true;
    Memory<unsigned char*> data(size);
    if(!MemReadDumb(addr, data(), data.size()))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read (all) memory..."));
        success = false;
    }

    String name = stringformatinline(argv[1]);
    if(name == ":memdump:")
        name = StringUtils::sprintf("%s\\memdumps\\memdump_%X_%p_%x.bin", szUserDir, fdProcessInfo->dwProcessId, addr, size);

    if(!FileHelper::WriteAllData(name, data(), data.size()))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to write file..."));
        return false;
    }
#ifdef _WIN64
    dprintf(QT_TRANSLATE_NOOP("DBG", "%p[%llX] written to \"%s\" !\n"), addr, size, name.c_str());
#else //x86
    dprintf(QT_TRANSLATE_NOOP("DBG", "%p[%X] written to \"%s\" !\n"), addr, size, name.c_str());
#endif

    return success;
}

bool cbInstrMinidump(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;

    // TODO: allow this with all threads suspended
    // TODO: add an option to only dump modules
    // TODO: add to the context menu
    if(DbgIsRunning())
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Cannot dump while running..."));
        return false;
    }

    HANDLE hFile = CreateFileA(argv[1], GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Could not open file!"));
        return false;
    }

    // Disable all software breakpoints
    std::vector<BREAKPOINT> disabledBreakpoints;
    {
        std::vector<BREAKPOINT> bplist;
        BpGetList(&bplist);
        for(const auto & bp : bplist)
        {
            if(bp.type == BPNORMAL && bp.active && DeleteBPX(bp.addr))
                disabledBreakpoints.push_back(bp);
        }
    }

    CONTEXT context = {};
    context.ContextFlags = CONTEXT_ALL;
    GetThreadContext(DbgGetThreadHandle(), &context);
    context.EFlags &= ~0x100; // remove trap flag

    EXCEPTION_POINTERS exceptionPointers = {};
    exceptionPointers.ContextRecord = &context;
    exceptionPointers.ExceptionRecord = &getLastExceptionInfo().ExceptionRecord;
    if(exceptionPointers.ExceptionRecord->ExceptionCode == 0)
    {
        auto & exceptionRecord = *exceptionPointers.ExceptionRecord;
        exceptionRecord.ExceptionCode = 0xFFFFFFFF;
#ifdef _WIN64
        exceptionRecord.ExceptionAddress = PVOID(context.Rip);
#else
        exceptionRecord.ExceptionAddress = PVOID(context.Eip);
#endif // _WIN64
    }

    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo = {};
    exceptionInfo.ThreadId = DbgGetThreadId();
    exceptionInfo.ExceptionPointers = &exceptionPointers;
    exceptionInfo.ClientPointers = FALSE;
    auto dumpType = MINIDUMP_TYPE(MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpIgnoreInaccessibleMemory | MiniDumpWithHandleData);
    auto dumpSaved = !!MiniDumpWriteDump(DbgGetProcessHandle(), DbgGetProcessId(), hFile, dumpType, &exceptionInfo, nullptr, nullptr);
    auto lastError = GetLastError() & 0xFFFF; // HRESULT_FROM_WIN32

    // Re-enable all breakpoints that were previously disabled
    for(const auto & bp : disabledBreakpoints)
    {
        SetBPX(bp.addr, bp.titantype, cbUserBreakpoint);
    }

    if(dumpSaved)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Dump saved!"));
    }
    else
    {
        String error = stringformatinline(StringUtils::sprintf("{winerror@%x}", lastError));
        dprintf(QT_TRANSLATE_NOOP("DBG", "MiniDumpWriteDump failed. GetLastError() = %s.\n"), error.c_str());
    }

    CloseHandle(hFile);
    return true;
}
