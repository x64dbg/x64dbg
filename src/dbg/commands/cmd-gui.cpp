#include "cmd-gui.h"
#include "debugger.h"
#include "console.h"
#include "memory.h"
#include "recursiveanalysis.h"
#include "function.h"
#include "stringformat.h"
#include "value.h"

CMDRESULT cbDebugDisasm(int argc, char* argv[])
{
    duint addr = 0;
    if(argc > 1)
    {
        if(!valfromstring(argv[1], &addr))
            addr = GetContextDataEx(hActiveThread, UE_CIP);
    }
    else
    {
        addr = GetContextDataEx(hActiveThread, UE_CIP);
    }
    DebugUpdateGui(addr, false);
    GuiShowCpu();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugDump(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Not enough arguments!"));
        return STATUS_ERROR;
    }
    duint addr = 0;
    if(!valfromstring(argv[1], &addr))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid address \"%s\"!\n"), argv[1]);
        return STATUS_ERROR;
    }
    if(argc > 2)
    {
        duint index = 0;
        if(!valfromstring(argv[2], &index))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid address \"%s\"!\n"), argv[2]);
            return STATUS_ERROR;
        }
        GuiDumpAtN(addr, int(index));
    }
    else
        GuiDumpAt(addr);
    GuiShowCpu();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugStackDump(int argc, char* argv[])
{
    duint addr = 0;
    if(argc < 2)
        addr = GetContextDataEx(hActiveThread, UE_CSP);
    else if(!valfromstring(argv[1], &addr))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid address \"%s\"!\n"), argv[1]);
        return STATUS_ERROR;
    }
    duint csp = GetContextDataEx(hActiveThread, UE_CSP);
    duint size = 0;
    duint base = MemFindBaseAddr(csp, &size);
    if(base && addr >= base && addr < (base + size))
        DebugUpdateStack(addr, csp, true);
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "Invalid stack address!"));
    GuiShowCpu();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugMemmapdump(int argc, char* argv[])
{
    if(argc < 2)
        return STATUS_ERROR;
    duint addr;
    if(!valfromstring(argv[1], &addr, false) || !MemIsValidReadPtr(addr, true))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid address \"%s\"!\n"), argv[1]);
        return STATUS_ERROR;
    }
    GuiSelectInMemoryMap(addr);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrGraph(int argc, char* argv[])
{
    duint entry;
    if(argc < 2 || !valfromstring(argv[1], &entry))
        entry = GetContextDataEx(hActiveThread, UE_CIP);
    duint start, size, sel = entry;
    if(FunctionGet(entry, &start))
        entry = start;
    auto base = MemFindBaseAddr(entry, &size);
    if(!base || !MemIsValidReadPtr(entry))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid memory address %p!\n"), entry);
        return STATUS_ERROR;
    }
    if(!GuiGraphAt(sel))
    {
        auto modbase = ModBaseFromAddr(base);
        if(modbase)
            base = modbase, size = ModSizeFromAddr(modbase);
        RecursiveAnalysis analysis(base, size, entry, 0);
        analysis.Analyse();
        auto graph = analysis.GetFunctionGraph(entry);
        if(!graph)
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "No graph generated..."));
            return STATUS_ERROR;
        }
        auto graphList = graph->ToGraphList();
        GuiLoadGraph(&graphList, sel);
    }
    GuiUpdateAllViews();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrEnableGuiUpdate(int argc, char* argv[])
{
    if(GuiIsUpdateDisabled())
        GuiUpdateEnable(false);
    duint value;
    //default: update gui
    if(argc > 1 && valfromstring(argv[1], &value) && value == 0)
        return STATUS_CONTINUE;
    duint cip = GetContextDataEx(hActiveThread, UE_CIP);
    DebugUpdateGuiAsync(cip, false);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrDisableGuiUpdate(int argc, char* argv[])
{
    if(!GuiIsUpdateDisabled())
        GuiUpdateDisable();
    return STATUS_CONTINUE;
}

CMDRESULT cbDebugSetfreezestack(int argc, char* argv[])
{
    if(argc < 2)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Not enough arguments!"));
        return STATUS_ERROR;
    }
    bool freeze = *argv[1] != '0';
    dbgsetfreezestack(freeze);
    if(freeze)
        dputs(QT_TRANSLATE_NOOP("DBG", "Stack is now freezed\n"));
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "Stack is now unfreezed\n"));
    return STATUS_CONTINUE;
}

static bool bRefinit = false;

CMDRESULT cbInstrRefinit(int argc, char* argv[])
{
    GuiReferenceInitialize(GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Script")));
    GuiReferenceAddColumn(sizeof(duint) * 2, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Address")));
    GuiReferenceAddColumn(0, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Data")));
    GuiReferenceSetRowCount(0);
    GuiReferenceReloadData();
    bRefinit = true;
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrRefadd(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return STATUS_ERROR;
    if(!bRefinit)
        cbInstrRefinit(argc, argv);
    int index = GuiReferenceGetRowCount();
    GuiReferenceSetRowCount(index + 1);
    char addr_text[32] = "";
    sprintf_s(addr_text, "%p", addr);
    GuiReferenceSetCellContent(index, 0, addr_text);
    GuiReferenceSetCellContent(index, 1, stringformatinline(argv[2]).c_str());
    GuiReferenceReloadData();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrEnableLog(int argc, char* argv[])
{
    GuiEnableLog();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrDisableLog(int argc, char* argv[])
{
    GuiDisableLog();
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAddFavTool(int argc, char* argv[])
{
    // filename, description
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;

    if(argc == 2)
        GuiAddFavouriteTool(argv[1], nullptr);
    else
        GuiAddFavouriteTool(argv[1], argv[2]);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrAddFavCmd(int argc, char* argv[])
{
    // command, shortcut
    if(IsArgumentsLessThan(argc, 2))
        return STATUS_ERROR;

    if(argc == 2)
        GuiAddFavouriteCommand(argv[1], nullptr);
    else
        GuiAddFavouriteCommand(argv[1], argv[2]);
    return STATUS_CONTINUE;
}

CMDRESULT cbInstrSetFavToolShortcut(int argc, char* argv[])
{
    // filename, shortcut
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;

    GuiSetFavouriteToolShortcut(argv[1], argv[2]);
    return STATUS_CONTINUE;

}

CMDRESULT cbInstrFoldDisassembly(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return STATUS_ERROR;

    duint start, length;
    if(!valfromstring(argv[1], &start))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid argument 1 : %s\n"), argv[1]);
        return STATUS_ERROR;
    }
    if(!valfromstring(argv[2], &length))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid argument 2 : %s\n"), argv[2]);
        return STATUS_ERROR;
    }
    GuiFoldDisassembly(start, length);
    return STATUS_CONTINUE;
}