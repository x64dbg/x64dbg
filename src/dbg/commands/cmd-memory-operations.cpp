#include "cmd-memory-operations.h"
#include "console.h"
#include "debugger.h"
#include "memory.h"
#include "variable.h"
#include "filehelper.h"
#include "value.h"

CMDRESULT cbDebugAlloc(int argc, char* argv[])
{
    duint size = 0x1000, addr = 0;
    if(argc > 1)
        if(!valfromstring(argv[1], &size, false))
            return STATUS_ERROR;
    if(argc > 2)
        if(!valfromstring(argv[2], &addr, false))
            return STATUS_ERROR;
    duint mem = (duint)MemAllocRemote(addr, size);
    if(!mem)
        dputs(QT_TRANSLATE_NOOP("DBG", "VirtualAllocEx failed"));
    else
        dprintf("%p\n", mem);
    if(mem)
        varset("$lastalloc", mem, true);
    //update memory map
    MemUpdateMap();
    GuiUpdateMemoryView();

    varset("$res", mem, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugFree(int argc, char* argv[])
{
    duint lastalloc;
    varget("$lastalloc", &lastalloc, 0, 0);
    duint addr = lastalloc;
    if(argc > 1)
    {
        if(!valfromstring(argv[1], &addr, false))
            return STATUS_ERROR;
    }
    else if(!lastalloc)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "$lastalloc is zero, provide a page address"));
        return STATUS_ERROR;
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
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugMemset(int argc, char* argv[])
{
    duint addr;
    duint value;
    duint size;
    if(argc < 3)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Not enough arguments"));
        return STATUS_ERROR;
    }
    if(!valfromstring(argv[1], &addr, false) || !valfromstring(argv[2], &value, false))
        return STATUS_ERROR;
    if(argc > 3)
    {
        if(!valfromstring(argv[3], &size, false))
            return STATUS_ERROR;
    }
    else
    {
        duint base = MemFindBaseAddr(addr, &size, true);
        if(!base)
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Invalid address specified"));
            return STATUS_ERROR;
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
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugGetPageRights(int argc, char* argv[])
{
    duint addr = 0;
    char rights[RIGHTS_STRING_SIZE];

    if(argc != 2 || !valfromstring(argv[1], &addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: using an address as arg1\n"));
        return STATUS_ERROR;
    }

    if(!MemGetPageRights(addr, rights))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Error getting rights of page: %s\n"), argv[1]);
        return STATUS_ERROR;
    }

    dprintf(QT_TRANSLATE_NOOP("DBG", "Page: %p, Rights: %s\n"), addr, rights);

    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetPageRights(int argc, char* argv[])
{
    duint addr = 0;
    char rights[RIGHTS_STRING_SIZE];

    if(argc < 3 || !valfromstring(argv[1], &addr))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Error: Using an address as arg1 and as arg2: Execute, ExecuteRead, ExecuteReadWrite, ExecuteWriteCopy, NoAccess, ReadOnly, ReadWrite, WriteCopy. You can add a G at first for add PAGE GUARD, example: GReadOnly\n"));
        return STATUS_ERROR;
    }

    if(!MemSetPageRights(addr, argv[2]))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Error: Set rights of %p with Rights: %s\n"), addr, argv[2]);
        return STATUS_ERROR;
    }

    if(!MemGetPageRights(addr, rights))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Error getting rights of page: %s\n"), argv[1]);
        return STATUS_ERROR;
    }

    //update the memory map
    MemUpdateMap();
    GuiUpdateMemoryView();

    dprintf(QT_TRANSLATE_NOOP("DBG", "New rights of %p: %s\n"), addr, rights);

    return STATUS_CONTINUE;
}

CMDRESULT cbInstrSavedata(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 4))
        return STATUS_ERROR;
    duint addr, size;
    if(!valfromstring(argv[2], &addr, false) || !valfromstring(argv[3], &size, false))
        return STATUS_ERROR;

    Memory<unsigned char*> data(size);
    if(!MemRead(addr, data(), data.size()))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to read memory..."));
        return STATUS_ERROR;
    }

    String name = argv[1];
    if(name == ":memdump:")
        name = StringUtils::sprintf("%s\\memdumps\\memdump_%X_%p_%x.bin", szProgramDir, fdProcessInfo->dwProcessId, addr, size);

    if(!FileHelper::WriteAllData(name, data(), data.size()))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to write file..."));
        return STATUS_ERROR;
    }
#ifdef _WIN64
    dprintf(QT_TRANSLATE_NOOP("DBG", "%p[% llX] written to \"%s\" !\n"), addr, size, name.c_str());
#else //x86
    dprintf(QT_TRANSLATE_NOOP("DBG", "%p[% X] written to \"%s\" !\n"), addr, size, name.c_str());
#endif

    return STATUS_CONTINUE;
}