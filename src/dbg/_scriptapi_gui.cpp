#include "_scriptapi_gui.h"
#include "_scriptapi_misc.h"

SCRIPT_EXPORT bool Script::Gui::Disassembly::SelectionGet(duint* start, duint* end)
{
    return Gui::SelectionGet(DisassemblyWindow, start, end);
}

SCRIPT_EXPORT bool Script::Gui::Disassembly::SelectionSet(duint start, duint end)
{
    return Gui::SelectionSet(DisassemblyWindow, start, end);
}

SCRIPT_EXPORT duint Script::Gui::Disassembly::SelectionGetStart()
{
    return Gui::SelectionGetStart(DisassemblyWindow);
}

SCRIPT_EXPORT duint Script::Gui::Disassembly::SelectionGetEnd()
{
    return Gui::SelectionGetEnd(DisassemblyWindow);
}

SCRIPT_EXPORT bool Script::Gui::Dump::SelectionGet(duint* start, duint* end)
{
    return Gui::SelectionGet(DumpWindow, start, end);
}

SCRIPT_EXPORT bool Script::Gui::Dump::SelectionSet(duint start, duint end)
{
    return Gui::SelectionSet(DumpWindow, start, end);
}

SCRIPT_EXPORT duint Script::Gui::Dump::SelectionGetStart()
{
    return Gui::SelectionGetStart(DumpWindow);
}

SCRIPT_EXPORT duint Script::Gui::Dump::SelectionGetEnd()
{
    return Gui::SelectionGetEnd(DumpWindow);
}

SCRIPT_EXPORT bool Script::Gui::Stack::SelectionGet(duint* start, duint* end)
{
    return Gui::SelectionGet(StackWindow, start, end);
}

SCRIPT_EXPORT bool Script::Gui::Stack::SelectionSet(duint start, duint end)
{
    return Gui::SelectionSet(StackWindow, start, end);
}

SCRIPT_EXPORT duint Script::Gui::Stack::SelectionGetStart()
{
    return Gui::SelectionGetStart(StackWindow);
}

SCRIPT_EXPORT duint Script::Gui::Stack::SelectionGetEnd()
{
    return Gui::SelectionGetEnd(StackWindow);
}

SCRIPT_EXPORT duint Script::Gui::Graph::SelectionGetStart()
{
    return SelectionGetStart(GraphWindow);
}

SCRIPT_EXPORT duint Script::Gui::MemMap::SelectionGetStart()
{
    return SelectionGetStart(MemMapWindow);
}

SCRIPT_EXPORT duint Script::Gui::SymMod::SelectionGetStart()
{
    return SelectionGetStart(SymModWindow);
}

static inline GUISELECTIONTYPE windowToBridge(Script::Gui::Window window)
{
    switch(window)
    {
    case Script::Gui::DisassemblyWindow:
        return GUI_DISASSEMBLY;
    case Script::Gui::DumpWindow:
        return GUI_DUMP;
    case Script::Gui::StackWindow:
        return GUI_STACK;
    case Script::Gui::GraphWindow:
        return GUI_GRAPH;
    case Script::Gui::MemMapWindow:
        return GUI_MEMMAP;
    case Script::Gui::SymModWindow:
        return GUI_SYMMOD;
    default:
        return GUI_DISASSEMBLY;
    }
}

SCRIPT_EXPORT bool Script::Gui::SelectionGet(Script::Gui::Window window, duint* start, duint* end)
{
    SELECTIONDATA selection;
    if(!GuiSelectionGet(windowToBridge(window), &selection))
        return false;
    if(start)
        *start = selection.start;
    if(end)
        *end = selection.end;
    return true;
}

SCRIPT_EXPORT bool Script::Gui::SelectionSet(Script::Gui::Window window, duint start, duint end)
{
    SELECTIONDATA selection;
    selection.start = start;
    selection.end = end;
    return GuiSelectionSet(windowToBridge(window), &selection);
}

SCRIPT_EXPORT duint Script::Gui::SelectionGetStart(Script::Gui::Window window)
{
    duint start;
    return Gui::SelectionGet(window, &start, nullptr) ? start : 0;
}

SCRIPT_EXPORT duint Script::Gui::SelectionGetEnd(Script::Gui::Window window)
{
    duint end;
    return Gui::SelectionGet(window, nullptr, &end) ? end : 0;
}

SCRIPT_EXPORT void Script::Gui::Message(const char* message)
{
    GuiScriptMessage(message);
}

SCRIPT_EXPORT bool Script::Gui::MessageYesNo(const char* message)
{
    return !!GuiScriptMsgyn(message);
}

SCRIPT_EXPORT bool Script::Gui::InputLine(const char* title, char* text)
{
    return GuiGetLineWindow(title, text);
}

SCRIPT_EXPORT bool Script::Gui::InputValue(const char* title, duint* value)
{
    Memory<char*> line(GUI_MAX_LINE_SIZE);
    if(!GuiGetLineWindow(title, line()))
        return false;
    return Misc::ParseExpression(line(), value);
}

SCRIPT_EXPORT void Script::Gui::Refresh()
{
    GuiUpdateAllViews();
}

SCRIPT_EXPORT void Script::Gui::AddQWidgetTab(void* qWidget)
{
    GuiAddQWidgetTab(qWidget);
}

SCRIPT_EXPORT void Script::Gui::ShowQWidgetTab(void* qWidget)
{
    GuiShowQWidgetTab(qWidget);
}

SCRIPT_EXPORT void Script::Gui::CloseQWidgetTab(void* qWidget)
{
    GuiCloseQWidgetTab(qWidget);
}