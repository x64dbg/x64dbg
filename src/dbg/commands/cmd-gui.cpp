#include "cmd-gui.h"
#include "debugger.h"
#include "console.h"
#include "memory.h"
#include "recursiveanalysis.h"
#include "function.h"
#include "stringformat.h"
#include "value.h"
#include "variable.h"

bool cbDebugDisasm(int argc, char* argv[])
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
    GuiFocusView(GUI_DISASSEMBLY);
    return true;
}

bool cbDebugDump(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid address \"%s\"!\n"), argv[1]);
        return false;
    }
    if(argc > 2)
    {
        duint index = 0;
        if(!valfromstring(argv[2], &index))
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid address \"%s\"!\n"), argv[2]);
            return false;
        }
        GuiDumpAtN(addr, int(index));
    }
    else
        GuiDumpAt(addr);
    GuiShowCpu();
    GuiFocusView(GUI_DUMP);
    return true;
}

bool cbDebugStackDump(int argc, char* argv[])
{
    duint addr = 0;
    if(argc < 2)
        addr = GetContextDataEx(hActiveThread, UE_CSP);
    else if(!valfromstring(argv[1], &addr))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid address \"%s\"!\n"), argv[1]);
        return false;
    }
    duint csp = GetContextDataEx(hActiveThread, UE_CSP);
    duint size = 0;
    duint base = MemFindBaseAddr(csp, &size);
    if(base && addr >= base && addr < (base + size))
        DebugUpdateStack(addr, csp, true);
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "Invalid stack address!"));
    GuiShowCpu();
    GuiFocusView(GUI_STACK);
    return true;
}

bool cbDebugMemmapdump(int argc, char* argv[])
{
    if(argc < 2)
        return false;
    duint addr;
    if(!valfromstring(argv[1], &addr, false) || !MemIsValidReadPtr(addr, true))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid address \"%s\"!\n"), argv[1]);
        return false;
    }
    GuiSelectInMemoryMap(addr);
    GuiFocusView(GUI_MEMMAP);
    return true;
}

bool cbInstrGraph(int argc, char* argv[])
{
    auto options = argc > 2 ? argv[2] : "";
    auto force = !!strstr(options, "force");
    auto silent = !!strstr(options, "silent");
    duint entry;
    if(argc < 2 || !valfromstring(argv[1], &entry))
        entry = GetContextDataEx(hActiveThread, UE_CIP);
    duint start, size, sel = entry;
    if(FunctionGet(entry, &start))
        entry = start;
    auto base = MemFindBaseAddr(entry, &size);
    if(!base || !MemIsValidReadPtr(entry))
    {
        if(!silent)
            dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid address %p!\n"), entry);
        return false;
    }
    auto curEntry = GuiGraphAt(sel);
    if(curEntry)
        entry = curEntry;
    if(!curEntry || force)
    {
        auto modbase = ModBaseFromAddr(base);
        if(modbase)
            base = modbase, size = ModSizeFromAddr(modbase);
        RecursiveAnalysis analysis(base, size, entry, true);
        analysis.Analyse();
        auto graph = analysis.GetFunctionGraph(entry);
        if(!graph)
        {
            if(!silent)
                dputs(QT_TRANSLATE_NOOP("DBG", "No graph generated..."));
            return false;
        }
        auto graphList = graph->ToGraphList();
        if(!GuiLoadGraph(&graphList, sel))
            return false;
    }
    GuiUpdateAllViews();
    if(!silent)
        GuiFocusView(GUI_GRAPH);
    return true;
}

bool cbInstrEnableGuiUpdate(int argc, char* argv[])
{
    if(GuiIsUpdateDisabled())
        GuiUpdateEnable(false);
    duint value;
    //argv[1]=="1" = update GUI
    if(argc > 1 && valfromstring(argv[1], &value) && value == 1)
        GuiUpdateAllViews();
    return true;
}

bool cbInstrDisableGuiUpdate(int argc, char* argv[])
{
    if(!GuiIsUpdateDisabled())
        GuiUpdateDisable();
    return true;
}

bool cbDebugSetfreezestack(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    bool freeze = *argv[1] != '0';
    dbgsetfreezestack(freeze);
    if(freeze)
        dputs(QT_TRANSLATE_NOOP("DBG", "Stack is now frozen\n"));
    else
        dputs(QT_TRANSLATE_NOOP("DBG", "Stack is now unfrozen\n"));
    return true;
}

static bool bRefinit = false;

bool cbInstrRefinit(int argc, char* argv[])
{
    auto title = argc > 1 ? stringformatinline(argv[1]) : GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Script"));
    GuiReferenceInitialize(title.c_str());
    GuiReferenceAddColumn(sizeof(duint) * 2, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Address")));
    GuiReferenceAddColumn(0, GuiTranslateText(QT_TRANSLATE_NOOP("DBG", "Data")));
    GuiReferenceSetRowCount(0);
    GuiReferenceReloadData();
    bRefinit = true;
    return true;
}

bool cbInstrRefadd(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;
    duint addr = 0;
    if(!valfromstring(argv[1], &addr, false))
        return false;
    if(!bRefinit)
        cbInstrRefinit(argc, argv);
    int index = GuiReferenceGetRowCount();
    GuiReferenceSetRowCount(index + 1);
    char addr_text[32] = "";
    sprintf_s(addr_text, "%p", addr);
    GuiReferenceSetCellContent(index, 0, addr_text);
    GuiReferenceSetCellContent(index, 1, stringformatinline(argv[2]).c_str());
    GuiReferenceReloadData();
    return true;
}

bool cbInstrRefGet(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 2))
        return false;
    duint row;
    if(!valfromstring(argv[1], &row, false))
        return false;
    auto content = GuiReferenceGetCellContent(int(row), 0);
    duint addr = 0;
    valfromstring(content, &addr, false);
    varset("$result", addr, false);
    BridgeFree(content);
    return true;
}

bool cbInstrEnableLog(int argc, char* argv[])
{
    GuiEnableLog();
    return true;
}

bool cbInstrDisableLog(int argc, char* argv[])
{
    GuiDisableLog();
    return true;
}

bool cbInstrAddFavTool(int argc, char* argv[])
{
    // filename, description
    if(IsArgumentsLessThan(argc, 2))
        return false;

    if(argc == 2)
        GuiAddFavouriteTool(argv[1], nullptr);
    else
        GuiAddFavouriteTool(argv[1], argv[2]);
    return true;
}

bool cbInstrAddFavCmd(int argc, char* argv[])
{
    // command, shortcut
    if(IsArgumentsLessThan(argc, 2))
        return false;

    if(argc == 2)
        GuiAddFavouriteCommand(argv[1], nullptr);
    else
        GuiAddFavouriteCommand(argv[1], argv[2]);
    return true;
}

bool cbInstrSetFavToolShortcut(int argc, char* argv[])
{
    // filename, shortcut
    if(IsArgumentsLessThan(argc, 3))
        return false;

    GuiSetFavouriteToolShortcut(argv[1], argv[2]);
    return true;

}

bool cbInstrFoldDisassembly(int argc, char* argv[])
{
    if(IsArgumentsLessThan(argc, 3))
        return false;

    duint start, length;
    if(!valfromstring(argv[1], &start))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid argument 1 : %s\n"), argv[1]);
        return false;
    }
    if(!valfromstring(argv[2], &length))
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Invalid argument 2 : %s\n"), argv[2]);
        return false;
    }
    GuiFoldDisassembly(start, length);
    return true;
}

bool cbDebugUpdateTitle(int argc, char* argv[])
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
    DebugUpdateTitleAsync(addr, false);
    return true;
}

bool cbShowReferences(int argc, char* argv[])
{
    GuiShowReferences();
    return true;
}
