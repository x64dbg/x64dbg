#include "cmd-analysis.h"
#include "ntdll/ntdll.h"
#include "linearanalysis.h"
#include "memory.h"
#include "exceptiondirectoryanalysis.h"
#include "controlflowanalysis.h"
#include "analysis_nukem.h"
#include "xrefsanalysis.h"
#include "recursiveanalysis.h"
#include "value.h"
#include "advancedanalysis.h"
#include "debugger.h"
#include "variable.h"
#include "exhandlerinfo.h"
#include "symbolinfo.h"
#include "exception.h"
#include "TraceRecord.h"
#include "dbghelp_safe.h"
#include <threading.h>

bool cbInstrAnalyse(int argc, char* argv[])
{
    SELECTIONDATA sel;
    GuiSelectionGet(GUI_DISASSEMBLY, &sel);
    duint size = 0;
    duint base = MemFindBaseAddr(sel.start, &size);
    LinearAnalysis anal(base, size);
    anal.Analyse();
    anal.SetMarkers();
    GuiUpdateAllViews();
    return true;
}

bool cbInstrExanalyse(int argc, char* argv[])
{
    SELECTIONDATA sel;
    GuiSelectionGet(GUI_DISASSEMBLY, &sel);
    duint size = 0;
    duint base = MemFindBaseAddr(sel.start, &size);
    ExceptionDirectoryAnalysis anal(base, size);
    anal.Analyse();
    anal.SetMarkers();
    GuiUpdateAllViews();
    return true;
}

bool cbInstrCfanalyse(int argc, char* argv[])
{
    bool exceptionDirectory = false;
    if(argc > 1)
        exceptionDirectory = true;
    SELECTIONDATA sel;
    GuiSelectionGet(GUI_DISASSEMBLY, &sel);
    duint size = 0;
    duint base = MemFindBaseAddr(sel.start, &size);
    ControlFlowAnalysis anal(base, size, exceptionDirectory);
    anal.Analyse();
    anal.SetMarkers();
    GuiUpdateAllViews();
    return true;
}

bool cbInstrAnalyseNukem(int argc, char* argv[])
{
    SELECTIONDATA sel;
    GuiSelectionGet(GUI_DISASSEMBLY, &sel);
    duint size = 0;
    duint base = MemFindBaseAddr(sel.start, &size);
    Analyse_nukem(base, size);
    GuiUpdateAllViews();
    return true;
}

bool cbInstrAnalxrefs(int argc, char* argv[])
{
    SELECTIONDATA sel;
    GuiSelectionGet(GUI_DISASSEMBLY, &sel);
    duint size = 0;
    auto base = MemFindBaseAddr(sel.start, &size);
    XrefsAnalysis anal(base, size);
    anal.Analyse();
    anal.SetMarkers();
    GuiUpdateAllViews();
    return true;
}

bool cbInstrAnalrecur(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint entry;
    if(!valfromstring(argv[1], &entry, false))
        return false;
    duint size;
    auto base = MemFindBaseAddr(entry, &size);
    if(!base)
        return false;
    RecursiveAnalysis analysis(base, size, entry, true);
    analysis.Analyse();
    analysis.SetMarkers();
    return true;
}

bool cbInstrAnalyseadv(int argc, char* argv[])
{
    SELECTIONDATA sel;
    GuiSelectionGet(GUI_DISASSEMBLY, &sel);
    duint size = 0;
    auto base = MemFindBaseAddr(sel.start, &size);
    AdvancedAnalysis anal(base, size);
    anal.Analyse();
    anal.SetMarkers();
    GuiUpdateAllViews();
    return true;
}

bool cbInstrVirtualmod(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint base;
    if(!valfromstring(argv[2], &base))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Invalid parameter [base]!"));
        return false;
    }
    if(!MemIsValidReadPtr(base))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Invalid memory address!"));
        return false;
    }
    duint size;
    if(argc < 4)
        base = MemFindBaseAddr(base, &size);
    else if(!valfromstring(argv[3], &size))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Invalid parameter [size]"));
        return false;
    }
    auto name = String("virtual:\\") + (argv[1]);
    if(!ModLoad(base, size, name.c_str()))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to load module (ModLoad)..."));
        return false;
    }

    char modname[256] = "";
    if(ModNameFromAddr(base, modname, true))
        BpEnumAll(cbSetModuleBreakpoints, modname);

    dprintf(QT_TRANSLATE_NOOP("DBG", "Virtual module \"%s\" loaded on %p[%p]!\n"), argv[1], base, size);
    GuiUpdateAllViews();
    return true;
}

bool cbDebugDownloadSymbol(int argc, char* argv[])
{
    dputs(QT_TRANSLATE_NOOP("DBG", "This may take very long, depending on your network connection and data in the debug directory..."));
    Memory<char*> szDefaultStore(MAX_SETTING_SIZE + 1);
    const char* szSymbolStore = szDefaultStore();
    if(!BridgeSettingGet("Symbols", "DefaultStore", szDefaultStore())) //get default symbol store from settings
    {
        strcpy_s(szDefaultStore(), MAX_SETTING_SIZE, "https://msdl.microsoft.com/download/symbols");
        BridgeSettingSet("Symbols", "DefaultStore", szDefaultStore());
    }
    if(argc < 2) //no arguments
    {
        SymDownloadAllSymbols(szSymbolStore); //download symbols for all modules
        GuiSymbolRefreshCurrent();
        dputs(QT_TRANSLATE_NOOP("DBG", "Done! See symbol log for more information"));
        return true;
    }
    //get some module information
    duint modbase = ModBaseFromName(argv[1]);
    if(!modbase)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid module \"%s\"!\n"), argv[1]);
        return false;
    }
    if(argc > 2)
        szSymbolStore = argv[2];
    if(!SymDownloadSymbol(modbase, szSymbolStore))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Symbol download failed... See symbol log for more information"));
        return false;
    }
    GuiSymbolRefreshCurrent();
    dputs(QT_TRANSLATE_NOOP("DBG", "Done! See symbol log for more information"));
    return true;
}

bool cbInstrImageinfo(int argc, char* argv[])
{
    duint address;
    if(argc < 2)
        address = GetContextDataEx(hActiveThread, UE_CIP);
    else if(!valfromstring(argv[1], &address))
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Invalid argument"));
        return false;
    }
    duint c, dllc, mod;
    {
        SHARED_ACQUIRE(LockModules);
        auto modinfo = ModInfoFromAddr(address);
        if(!modinfo)
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "Invalid argument"));
            return false;
        }
        c = GetPE32DataFromMappedFile(modinfo->fileMapVA, 0, UE_CHARACTERISTICS);
        dllc = GetPE32DataFromMappedFile(modinfo->fileMapVA, 0, UE_DLLCHARACTERISTICS);
        mod = modinfo->base;
    }

    auto pFlag = [](ULONG_PTR value, ULONG_PTR flag, const char* name)
    {
        if((value & flag) == flag)
        {
            dprintf("  ");
            dputs(name);
        }
    };

    char modname[MAX_MODULE_SIZE] = "";
    ModNameFromAddr(mod, modname, true);

    dputs_untranslated("---------------");

    dprintf(QT_TRANSLATE_NOOP("DBG", "Image information for %s\n"), modname);

    dprintf(QT_TRANSLATE_NOOP("DBG", "Characteristics (0x%X):\n"), DWORD(c));
    if(!c)
        dputs(QT_TRANSLATE_NOOP("DBG", "  None\n"));
    pFlag(c, IMAGE_FILE_RELOCS_STRIPPED, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_RELOCS_STRIPPED: Relocation info stripped from file."));
    pFlag(c, IMAGE_FILE_EXECUTABLE_IMAGE, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_EXECUTABLE_IMAGE: File is executable (i.e. no unresolved externel references)."));
    pFlag(c, IMAGE_FILE_LINE_NUMS_STRIPPED, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_LINE_NUMS_STRIPPED: Line numbers stripped from file."));
    pFlag(c, IMAGE_FILE_LOCAL_SYMS_STRIPPED, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_LOCAL_SYMS_STRIPPED: Local symbols stripped from file."));
    pFlag(c, IMAGE_FILE_AGGRESIVE_WS_TRIM, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_AGGRESIVE_WS_TRIM: Agressively trim working set"));
    pFlag(c, IMAGE_FILE_LARGE_ADDRESS_AWARE, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_LARGE_ADDRESS_AWARE: App can handle >2gb addresses"));
    pFlag(c, IMAGE_FILE_BYTES_REVERSED_LO, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_BYTES_REVERSED_LO: Bytes of machine word are reversed."));
    pFlag(c, IMAGE_FILE_32BIT_MACHINE, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_32BIT_MACHINE: 32 bit word machine."));
    pFlag(c, IMAGE_FILE_DEBUG_STRIPPED, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_DEBUG_STRIPPED: Debugging info stripped from file in .DBG file"));
    pFlag(c, IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP: If Image is on removable media, copy and run from the swap file."));
    pFlag(c, IMAGE_FILE_NET_RUN_FROM_SWAP, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_NET_RUN_FROM_SWAP: If Image is on Net, copy and run from the swap file."));
    pFlag(c, IMAGE_FILE_SYSTEM, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_SYSTEM: System File."));
    pFlag(c, IMAGE_FILE_DLL, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_DLL: File is a DLL."));
    pFlag(c, IMAGE_FILE_UP_SYSTEM_ONLY, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_UP_SYSTEM_ONLY: File should only be run on a UP machine"));
    pFlag(c, IMAGE_FILE_BYTES_REVERSED_HI, QT_TRANSLATE_NOOP("DBG", "IMAGE_FILE_BYTES_REVERSED_HI: Bytes of machine word are reversed."));

    dprintf(QT_TRANSLATE_NOOP("DBG", "DLL Characteristics (0x%X):\n"), DWORD(dllc));
    if(!dllc)
        dputs(QT_TRANSLATE_NOOP("DBG", "  None\n"));
    pFlag(dllc, IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE, QT_TRANSLATE_NOOP("DBG", "IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE: DLL can move."));
    pFlag(dllc, IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY, QT_TRANSLATE_NOOP("DBG", "IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY: Code Integrity Image"));
    pFlag(dllc, IMAGE_DLLCHARACTERISTICS_NX_COMPAT, QT_TRANSLATE_NOOP("DBG", "IMAGE_DLLCHARACTERISTICS_NX_COMPAT: Image is NX compatible"));
    pFlag(dllc, IMAGE_DLLCHARACTERISTICS_NO_ISOLATION, QT_TRANSLATE_NOOP("DBG", "IMAGE_DLLCHARACTERISTICS_NO_ISOLATION: Image understands isolation and doesn't want it"));
    pFlag(dllc, IMAGE_DLLCHARACTERISTICS_NO_SEH, QT_TRANSLATE_NOOP("DBG", "IMAGE_DLLCHARACTERISTICS_NO_SEH: Image does not use SEH. No SE handler may reside in this image"));
    pFlag(dllc, IMAGE_DLLCHARACTERISTICS_NO_BIND, QT_TRANSLATE_NOOP("DBG", "IMAGE_DLLCHARACTERISTICS_NO_BIND: Do not bind this image."));
    pFlag(dllc, IMAGE_DLLCHARACTERISTICS_WDM_DRIVER, QT_TRANSLATE_NOOP("DBG", "IMAGE_DLLCHARACTERISTICS_WDM_DRIVER: Driver uses WDM model."));
    pFlag(dllc, IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE, QT_TRANSLATE_NOOP("DBG", "IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE: Remote Desktop Services aware."));

    dputs_untranslated("---------------");

    return true;
}

bool cbInstrGetRelocSize(int argc, char* argv[])
{
    //Original tool "GetRelocSize" by Killboy/SND
    if(argc < 2)
    {
        _plugin_logputs(QT_TRANSLATE_NOOP("DBG", "Not enough arguments!"));
        return false;
    }

    duint RelocDirAddr;
    if(!valfromstring(argv[1], &RelocDirAddr, false))
        return false;

    duint RelocSize = 0;
    varset("$result", RelocSize, false);
    IMAGE_RELOCATION RelocDir;
    do
    {
        if(!MemRead(RelocDirAddr, &RelocDir, sizeof(IMAGE_RELOCATION)))
        {
            _plugin_logputs(QT_TRANSLATE_NOOP("DBG", "Invalid relocation table!"));
            return false;
        }
        if(!RelocDir.SymbolTableIndex)
            break;
        RelocSize += RelocDir.SymbolTableIndex;
        RelocDirAddr += RelocDir.SymbolTableIndex;
    }
    while(RelocDir.VirtualAddress);

    if(!RelocSize)
    {
        _plugin_logputs(QT_TRANSLATE_NOOP("DBG", "Invalid relocation table!"));
        return false;
    }

    varset("$result", RelocSize, false);
    _plugin_logprintf(QT_TRANSLATE_NOOP("DBG", "Relocation table size: %X\n"), RelocSize);

    return true;
}

static void printExhandlers(const char* name, const std::vector<duint> & entries)
{
    if(!entries.size())
        return;
    dprintf("%s:\n", name);
    for(auto entry : entries)
    {
        auto symbolic = SymGetSymbolicName(entry);
        if(symbolic.length())
            dprintf_untranslated("%p %s\n", entry, symbolic.c_str());
        else
            dprintf_untranslated("%p\n", entry);
    }
}

bool cbInstrExhandlers(int argc, char* argv[])
{
    std::vector<duint> entries;
#ifndef _WIN64
    if(ExHandlerGetInfo(EX_HANDLER_SEH, entries))
    {
        std::vector<duint> handlers;
        for(auto entry : entries)
        {
            duint handler;
            if(MemRead(entry + sizeof(duint), &handler, sizeof(handler)))
                handlers.push_back(handler);
        }
        printExhandlers("StructuredExceptionHandler (SEH)", handlers);
    }
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to get SEH (disabled?)"));
#endif //_WIN64

    if(ExHandlerGetInfo(EX_HANDLER_VEH, entries))
        printExhandlers("VectoredExceptionHandler (VEH)", entries);
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to get VEH (loaded symbols for ntdll.dll?)"));

    if(IsVistaOrLater())
    {
        if(ExHandlerGetInfo(EX_HANDLER_VCH, entries))
            printExhandlers("VectoredContinueHandler (VCH)", entries);
        else
            dputs(QT_TRANSLATE_NOOP("DBG", "Failed to get VCH (loaded symbols for ntdll.dll?)"));
    }

    if(ExHandlerGetInfo(EX_HANDLER_UNHANDLED, entries))
        printExhandlers("UnhandledExceptionFilter", entries);
    else if(IsVistaOrLater())
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to get UnhandledExceptionFilter (loaded symbols for kernelbase.dll?)"));
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "Failed to get UnhandledExceptionFilter (loaded symbols for kernel32.dll?)"));
    return true;
}

bool cbInstrExinfo(int argc, char* argv[])
{
    const unsigned int MASK_FACILITY_VISUALCPP = 0x006D0000;
    auto info = getLastExceptionInfo();
    const auto & record = info.ExceptionRecord;
    dputs_untranslated("EXCEPTION_DEBUG_INFO:");
    dprintf_untranslated("           dwFirstChance: %X\n", info.dwFirstChance);
    auto exceptionName = ExceptionCodeToName(record.ExceptionCode);
    if(!exceptionName.size()) //if no exception was found, try the error codes (RPC_S_*)
        exceptionName = ErrorCodeToName(record.ExceptionCode);
    if(exceptionName.size())
        dprintf_untranslated("           ExceptionCode: %08X (%s)\n", record.ExceptionCode, exceptionName.c_str());
    else if((record.ExceptionCode & MASK_FACILITY_VISUALCPP) == MASK_FACILITY_VISUALCPP)  //delayhlp.cpp
    {
        auto possibleError = record.ExceptionCode & 0xFFFF;
        exceptionName = ErrorCodeToName(possibleError);
        if(!exceptionName.empty())
            exceptionName = StringUtils::sprintf(" (Visual C++ %s)", exceptionName.c_str());
        dprintf_untranslated("           ExceptionCode: %08X%s\n", record.ExceptionCode, exceptionName.c_str());
    }
    else
        dprintf_untranslated("           ExceptionCode: %08X\n", record.ExceptionCode);
    dprintf_untranslated("          ExceptionFlags: %08X\n", record.ExceptionFlags);
    auto symbolic = SymGetSymbolicName(duint(record.ExceptionAddress));
    if(symbolic.length())
        dprintf_untranslated("        ExceptionAddress: %p %s\n", record.ExceptionAddress, symbolic.c_str());
    else
        dprintf_untranslated("        ExceptionAddress: %p\n", record.ExceptionAddress);
    dprintf_untranslated("        NumberParameters: %u\n", record.NumberParameters);
    if(record.NumberParameters)
        for(DWORD i = 0; i < record.NumberParameters; i++)
        {
            symbolic = SymGetSymbolicName(duint(record.ExceptionInformation[i]));
            if(symbolic.length())
                dprintf_untranslated("ExceptionInformation[%02u]: %p %s", i, record.ExceptionInformation[i], symbolic.c_str());
            else
                dprintf_untranslated("ExceptionInformation[%02u]: %p", i, record.ExceptionInformation[i]);
            //https://msdn.microsoft.com/en-us/library/windows/desktop/aa363082(v=vs.85).aspx
            if(record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
                    record.ExceptionCode == EXCEPTION_IN_PAGE_ERROR ||
                    record.ExceptionCode == EXCEPTION_GUARD_PAGE)
            {
                if(i == 0)
                {
                    if(record.ExceptionInformation[i] == 0)
                        dprintf_untranslated(" Read");
                    else if(record.ExceptionInformation[i] == 1)
                        dprintf_untranslated(" Write");
                    else if(record.ExceptionInformation[i] == 8)
                        dprintf_untranslated(" DEP Violation");
                }
                else if(i == 1)
                    dprintf_untranslated(" Inaccessible Address");
                else if(record.ExceptionCode == EXCEPTION_IN_PAGE_ERROR && i == 2)
                    dprintf_untranslated(" %s", ExceptionCodeToName((unsigned int)record.ExceptionInformation[i]).c_str());
            }
            dprintf_untranslated("\n");
        }
    return true;
}

bool cbInstrTraceexecute(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint addr;
    if(!valfromstring(argv[1], &addr, false))
        return false;
    _dbg_dbgtraceexecute(addr);
    return true;
}