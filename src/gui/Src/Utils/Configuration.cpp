#include "Configuration.h"
#include <QApplication>
#include <QFontInfo>
#include <QMessageBox>
#include <QIcon>
#include <QScreen>
#include <QGuiApplication>
#include <QWheelEvent>
#include "AbstractTableView.h"

Configuration* Configuration::mPtr = nullptr;

inline void insertMenuBuilderBools(QMap<QString, bool>* config, const char* id, size_t count)
{
    for(size_t i = 0; i < count; i++)
        config->insert(QString("Menu%1Hidden%2").arg(id).arg(i), false);
}

Configuration::Configuration() : QObject(), noMoreMsgbox(false)
{
    mPtr = this;
    //setup default color map
    defaultColors.clear();
    defaultColors.insert("AbstractTableViewSeparatorColor", QColor("#808080"));
    defaultColors.insert("AbstractTableViewBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("AbstractTableViewTextColor", QColor("#000000"));
    defaultColors.insert("AbstractTableViewHeaderTextColor", QColor("#000000"));
    defaultColors.insert("AbstractTableViewSelectionColor", QColor("#C0C0C0"));

    defaultColors.insert("DisassemblyCipColor", QColor("#FFFFFF"));
    defaultColors.insert("DisassemblyCipBackgroundColor", QColor("#000000"));
    defaultColors.insert("DisassemblyBreakpointColor", QColor("#000000"));
    defaultColors.insert("DisassemblyBreakpointBackgroundColor", QColor("#FF0000"));
    defaultColors.insert("DisassemblyHardwareBreakpointColor", QColor("#000000"));
    defaultColors.insert("DisassemblyHardwareBreakpointBackgroundColor", QColor("#FF8080"));
    defaultColors.insert("DisassemblyBookmarkColor", QColor("#000000"));
    defaultColors.insert("DisassemblyBookmarkBackgroundColor", QColor("#FEE970"));
    defaultColors.insert("DisassemblyLabelColor", QColor("#FF0000"));
    defaultColors.insert("DisassemblyLabelBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblyBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("DisassemblySelectionColor", QColor("#C0C0C0"));
    defaultColors.insert("DisassemblyTracedBackgroundColor", QColor("#C0FFC0"));
    defaultColors.insert("DisassemblyAddressColor", QColor("#808080"));
    defaultColors.insert("DisassemblyAddressBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblySelectedAddressColor", QColor("#000000"));
    defaultColors.insert("DisassemblySelectedAddressBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblyConditionalJumpLineTrueColor", QColor("#FF0000"));
    defaultColors.insert("DisassemblyConditionalJumpLineFalseColor", QColor("#808080"));
    defaultColors.insert("DisassemblyUnconditionalJumpLineColor", QColor("#FF0000"));
    defaultColors.insert("DisassemblyBytesColor", QColor("#000000"));
    defaultColors.insert("DisassemblyBytesBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblyModifiedBytesColor", QColor("#FF0000"));
    defaultColors.insert("DisassemblyModifiedBytesBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblyRestoredBytesColor", QColor("#808080"));
    defaultColors.insert("DisassemblyRestoredBytesBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblyRelocationUnderlineColor", QColor("#000000"));
    defaultColors.insert("DisassemblyCommentColor", QColor("#000000"));
    defaultColors.insert("DisassemblyCommentBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblyAutoCommentColor", QColor("#AA5500"));
    defaultColors.insert("DisassemblyAutoCommentBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblyMnemonicBriefColor", QColor("#717171"));
    defaultColors.insert("DisassemblyMnemonicBriefBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblyFunctionColor", QColor("#000000"));
    defaultColors.insert("DisassemblyLoopColor", QColor("#000000"));

    defaultColors.insert("SideBarBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("SideBarCipLabelColor", QColor("#FFFFFF"));
    defaultColors.insert("SideBarCipLabelBackgroundColor", QColor("#4040FF"));
    defaultColors.insert("SideBarBulletColor", QColor("#808080"));
    defaultColors.insert("SideBarBulletBreakpointColor", QColor("#FF0000"));
    defaultColors.insert("SideBarBulletDisabledBreakpointColor", QColor("#00AA00"));
    defaultColors.insert("SideBarBulletBookmarkColor", QColor("#FEE970"));
    defaultColors.insert("SideBarCheckBoxForeColor", QColor("#000000"));
    defaultColors.insert("SideBarCheckBoxBackColor", QColor("#FFFFFF"));
    defaultColors.insert("SideBarConditionalJumpLineTrueColor", QColor("#FF0000"));
    defaultColors.insert("SideBarConditionalJumpLineTrueBackwardsColor", QColor("#FF0000"));
    defaultColors.insert("SideBarConditionalJumpLineFalseColor", QColor("#00BBFF"));
    defaultColors.insert("SideBarConditionalJumpLineFalseBackwardsColor", QColor("#FFA500"));
    defaultColors.insert("SideBarUnconditionalJumpLineTrueColor", QColor("#FF0000"));
    defaultColors.insert("SideBarUnconditionalJumpLineTrueBackwardsColor", QColor("#FF0000"));
    defaultColors.insert("SideBarUnconditionalJumpLineFalseColor", QColor("#00BBFF"));
    defaultColors.insert("SideBarUnconditionalJumpLineFalseBackwardsColor", QColor("#FFA500"));

    defaultColors.insert("RegistersBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("RegistersColor", QColor("#000000"));
    defaultColors.insert("RegistersModifiedColor", QColor("#FF0000"));
    defaultColors.insert("RegistersSelectionColor", QColor("#EEEEEE"));
    defaultColors.insert("RegistersLabelColor", QColor("#000000"));
    defaultColors.insert("RegistersArgumentLabelColor", Qt::darkGreen);
    defaultColors.insert("RegistersExtraInfoColor", QColor("#000000"));
    defaultColors.insert("RegistersHighlightReadColor", QColor("#00A000"));
    defaultColors.insert("RegistersHighlightWriteColor", QColor("#B00000"));
    defaultColors.insert("RegistersHighlightReadWriteColor", QColor("#808000"));

    defaultColors.insert("InstructionHighlightColor", QColor("#FFFFFF"));
    defaultColors.insert("InstructionHighlightBackgroundColor", QColor("#CC0000"));
    defaultColors.insert("InstructionCommaColor", QColor("#000000"));
    defaultColors.insert("InstructionCommaBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionPrefixColor", QColor("#000000"));
    defaultColors.insert("InstructionPrefixBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionUncategorizedColor", QColor("#000000"));
    defaultColors.insert("InstructionUncategorizedBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionAddressColor", QColor("#000000"));
    defaultColors.insert("InstructionAddressBackgroundColor", QColor("#FFFF00"));
    defaultColors.insert("InstructionValueColor", QColor("#828200"));
    defaultColors.insert("InstructionValueBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionMnemonicColor", QColor("#000000"));
    defaultColors.insert("InstructionMnemonicBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionPushPopColor", QColor("#0000FF"));
    defaultColors.insert("InstructionPushPopBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionCallColor", QColor("#000000"));
    defaultColors.insert("InstructionCallBackgroundColor", QColor("#00FFFF"));
    defaultColors.insert("InstructionRetColor", QColor("#000000"));
    defaultColors.insert("InstructionRetBackgroundColor", QColor("#00FFFF"));
    defaultColors.insert("InstructionConditionalJumpColor", QColor("#FF0000"));
    defaultColors.insert("InstructionConditionalJumpBackgroundColor", QColor("#FFFF00"));
    defaultColors.insert("InstructionUnconditionalJumpColor", QColor("#000000"));
    defaultColors.insert("InstructionUnconditionalJumpBackgroundColor", QColor("#FFFF00"));
    defaultColors.insert("InstructionUnusualColor", QColor("#000000"));
    defaultColors.insert("InstructionUnusualBackgroundColor", QColor("#C00000"));
    defaultColors.insert("InstructionNopColor", QColor("#808080"));
    defaultColors.insert("InstructionNopBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionFarColor", QColor("#000000"));
    defaultColors.insert("InstructionFarBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionInt3Color", QColor("#000000"));
    defaultColors.insert("InstructionInt3BackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionMemorySizeColor", QColor("#000080"));
    defaultColors.insert("InstructionMemorySizeBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionMemorySegmentColor", QColor("#FF00FF"));
    defaultColors.insert("InstructionMemorySegmentBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionMemoryBracketsColor", QColor("#000000"));
    defaultColors.insert("InstructionMemoryBracketsBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionMemoryStackBracketsColor", QColor("#000000"));
    defaultColors.insert("InstructionMemoryStackBracketsBackgroundColor", QColor("#00FFFF"));
    defaultColors.insert("InstructionMemoryBaseRegisterColor", QColor("#B03434"));
    defaultColors.insert("InstructionMemoryBaseRegisterBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionMemoryIndexRegisterColor", QColor("#3838BC"));
    defaultColors.insert("InstructionMemoryIndexRegisterBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionMemoryScaleColor", QColor("#B30059"));
    defaultColors.insert("InstructionMemoryScaleBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionMemoryOperatorColor", QColor("#F27711"));
    defaultColors.insert("InstructionMemoryOperatorBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionGeneralRegisterColor", QColor("#008300"));
    defaultColors.insert("InstructionGeneralRegisterBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionFpuRegisterColor", QColor("#000080"));
    defaultColors.insert("InstructionFpuRegisterBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionMmxRegisterColor", QColor("#000080"));
    defaultColors.insert("InstructionMmxRegisterBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionXmmRegisterColor", QColor("#000080"));
    defaultColors.insert("InstructionXmmRegisterBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionYmmRegisterColor", QColor("#000080"));
    defaultColors.insert("InstructionYmmRegisterBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionZmmRegisterColor", QColor("#000080"));
    defaultColors.insert("InstructionZmmRegisterBackgroundColor", Qt::transparent);

    defaultColors.insert("HexDumpTextColor", QColor("#000000"));
    defaultColors.insert("HexDumpModifiedBytesColor", QColor("#FF0000"));
    defaultColors.insert("HexDumpModifiedBytesBackgroundColor", Qt::transparent);
    defaultColors.insert("HexDumpRestoredBytesColor", QColor("#808080"));
    defaultColors.insert("HexDumpRestoredBytesBackgroundColor", Qt::transparent);
    defaultColors.insert("HexDumpByte00Color", QColor("#008000"));
    defaultColors.insert("HexDumpByte00BackgroundColor", Qt::transparent);
    defaultColors.insert("HexDumpByte7FColor", QColor("#808000"));
    defaultColors.insert("HexDumpByte7FBackgroundColor", Qt::transparent);
    defaultColors.insert("HexDumpByteFFColor", QColor("#800000"));
    defaultColors.insert("HexDumpByteFFBackgroundColor", Qt::transparent);
    defaultColors.insert("HexDumpByteIsPrintColor", QColor("#800080"));
    defaultColors.insert("HexDumpByteIsPrintBackgroundColor", Qt::transparent);
    defaultColors.insert("HexDumpBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("HexDumpSelectionColor", QColor("#C0C0C0"));
    defaultColors.insert("HexDumpAddressColor", QColor("#000000"));
    defaultColors.insert("HexDumpAddressBackgroundColor", Qt::transparent);
    defaultColors.insert("HexDumpLabelColor", QColor("#FF0000"));
    defaultColors.insert("HexDumpLabelBackgroundColor", Qt::transparent);
    defaultColors.insert("HexDumpUserModuleCodePointerHighlightColor", QColor("#00FF00"));
    defaultColors.insert("HexDumpUserModuleDataPointerHighlightColor", QColor("#008000"));
    defaultColors.insert("HexDumpSystemModuleCodePointerHighlightColor", QColor("#FF0000"));
    defaultColors.insert("HexDumpSystemModuleDataPointerHighlightColor", QColor("#800000"));
    defaultColors.insert("HexDumpUnknownCodePointerHighlightColor", QColor("#0000FF"));
    defaultColors.insert("HexDumpUnknownDataPointerHighlightColor", QColor("#000080"));

    defaultColors.insert("StackTextColor", QColor("#000000"));
    defaultColors.insert("StackInactiveTextColor", QColor("#808080"));
    defaultColors.insert("StackBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("StackSelectionColor", QColor("#C0C0C0"));
    defaultColors.insert("StackCspColor", QColor("#FFFFFF"));
    defaultColors.insert("StackCspBackgroundColor", QColor("#000000"));
    defaultColors.insert("StackAddressColor", QColor("#808080"));
    defaultColors.insert("StackAddressBackgroundColor", Qt::transparent);
    defaultColors.insert("StackSelectedAddressColor", QColor("#000000"));
    defaultColors.insert("StackSelectedAddressBackgroundColor", Qt::transparent);
    defaultColors.insert("StackLabelColor", QColor("#FF0000"));
    defaultColors.insert("StackLabelBackgroundColor", Qt::transparent);
    defaultColors.insert("StackReturnToColor", QColor("#FF0000"));
    defaultColors.insert("StackSEHChainColor", QColor("#AE81FF"));
    defaultColors.insert("StackFrameColor", QColor("#000000"));
    defaultColors.insert("StackFrameSystemColor", QColor("#0000FF"));

    defaultColors.insert("HexEditTextColor", QColor("#000000"));
    defaultColors.insert("HexEditWildcardColor", QColor("#FF0000"));
    defaultColors.insert("HexEditBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("HexEditSelectionColor", QColor("#C0C0C0"));

    defaultColors.insert("GraphJmpColor", QColor("#0148FB"));
    defaultColors.insert("GraphBrtrueColor", QColor("#387804"));
    defaultColors.insert("GraphBrfalseColor", QColor("#ED4630"));
    defaultColors.insert("GraphCurrentShadowColor", QColor("#473a3a"));
    defaultColors.insert("GraphRetShadowColor", QColor("#900000"));
    defaultColors.insert("GraphIndirectcallShadowColor", QColor("#008080"));
    defaultColors.insert("GraphBackgroundColor", Qt::transparent);
    defaultColors.insert("GraphNodeColor", QColor("#000000"));
    defaultColors.insert("GraphNodeBackgroundColor", Qt::transparent);
    defaultColors.insert("GraphCipColor", QColor("#000000"));
    defaultColors.insert("GraphBreakpointColor", QColor("#FF0000"));
    defaultColors.insert("GraphDisabledBreakpointColor", QColor("#00AA00"));

    defaultColors.insert("ThreadCurrentColor", QColor("#FFFFFF"));
    defaultColors.insert("ThreadCurrentBackgroundColor", QColor("#000000"));
    defaultColors.insert("WatchTriggeredColor", QColor("#FF0000"));
    defaultColors.insert("WatchTriggeredBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("MemoryMapBreakpointColor", QColor("#000000"));
    defaultColors.insert("MemoryMapBreakpointBackgroundColor", QColor("#FF0000"));
    defaultColors.insert("MemoryMapCipColor", QColor("#FFFFFF"));
    defaultColors.insert("MemoryMapCipBackgroundColor", QColor("#000000"));
    defaultColors.insert("MemoryMapSectionTextColor", QColor("#8B671F"));
    defaultColors.insert("SearchListViewHighlightColor", QColor("#FF0000"));
    defaultColors.insert("SearchListViewHighlightBackgroundColor", Qt::transparent);
    defaultColors.insert("StructTextColor", QColor("#000000"));
    defaultColors.insert("StructBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("StructAlternateBackgroundColor", QColor("#DCD9CF"));
    defaultColors.insert("LogLinkColor", QColor("#00CC00"));
    defaultColors.insert("LogLinkBackgroundColor", Qt::transparent);
    defaultColors.insert("BreakpointSummaryParenColor", Qt::red);
    defaultColors.insert("BreakpointSummaryKeywordColor", QColor("#8B671F"));
    defaultColors.insert("BreakpointSummaryStringColor", QColor("#008000"));
    defaultColors.insert("PatchRelocatedByteHighlightColor", QColor("#0000DD"));
    defaultColors.insert("SymbolUserTextColor", QColor("#000000"));
    defaultColors.insert("SymbolSystemTextColor", QColor("#000000"));
    defaultColors.insert("SymbolUnloadedTextColor", QColor("#000000"));
    defaultColors.insert("SymbolLoadingTextColor", QColor("#8B671F"));
    defaultColors.insert("SymbolLoadedTextColor", QColor("#008000"));
    defaultColors.insert("BackgroundFlickerColor", QColor("#ff6961"));
    defaultColors.insert("LinkColor", QColor("#0000ff"));
    defaultColors.insert("LogColor", QColor("#000000"));
    defaultColors.insert("LogBackgroundColor", QColor("#FFF8F0"));

    //bool settings
    QMap<QString, bool> disassemblyBool;
    disassemblyBool.insert("ArgumentSpaces", false);
    disassemblyBool.insert("HidePointerSizes", false);
    disassemblyBool.insert("HideNormalSegments", false);
    disassemblyBool.insert("MemorySpaces", false);
    disassemblyBool.insert("KeepSize", false);
    disassemblyBool.insert("FillNOPs", false);
    disassemblyBool.insert("Uppercase", false);
    disassemblyBool.insert("FindCommandEntireBlock", false);
    disassemblyBool.insert("OnlyCipAutoComments", false);
    disassemblyBool.insert("TabbedMnemonic", false);
    disassemblyBool.insert("LongDataInstruction", false);
    disassemblyBool.insert("NoHighlightOperands", false);
    disassemblyBool.insert("PermanentHighlightingMode", false);
    disassemblyBool.insert("0xPrefixValues", false);
    disassemblyBool.insert("NoBranchDisasmPreview", false);
    disassemblyBool.insert("NoCurrentModuleText", false);
    defaultBools.insert("Disassembler", disassemblyBool);

    QMap<QString, bool> engineBool;
    engineBool.insert("ListAllPages", false);
    engineBool.insert("ShowSuspectedCallStack", false);
    defaultBools.insert("Engine", engineBool);

    QMap<QString, bool> miscBool;
    miscBool.insert("TransparentExceptionStepping", true);
    miscBool.insert("CheckForAntiCheatDrivers", true);
    defaultBools.insert("Misc", miscBool);

    QMap<QString, bool> guiBool;
    guiBool.insert("FpuRegistersLittleEndian", false);
    guiBool.insert("SaveColumnOrder", true);
    guiBool.insert("NoCloseDialog", false);
    guiBool.insert("PidTidInHex", false);
    guiBool.insert("SidebarWatchLabels", true);
    guiBool.insert("LoadSaveTabOrder", true);
    guiBool.insert("ShowGraphRva", false);
    guiBool.insert("GraphZoomMode", true);
    guiBool.insert("ShowExitConfirmation", false);
    guiBool.insert("DisableAutoComplete", false);
    guiBool.insert("CaseSensitiveAutoComplete", false);
    guiBool.insert("AutoRepeatOnEnter", false);
    guiBool.insert("AutoFollowInStack", true);
    guiBool.insert("EnableQtHighDpiScaling", true);
    guiBool.insert("Topmost", false);
    //Named menu settings
    insertMenuBuilderBools(&guiBool, "CPUDisassembly", 50); //CPUDisassembly
    insertMenuBuilderBools(&guiBool, "CPUDump", 50); //CPUDump
    insertMenuBuilderBools(&guiBool, "WatchView", 50); //Watch
    insertMenuBuilderBools(&guiBool, "CallStackView", 50); //CallStackView
    insertMenuBuilderBools(&guiBool, "ThreadView", 50); //Thread
    insertMenuBuilderBools(&guiBool, "CPUStack", 50); //Stack
    insertMenuBuilderBools(&guiBool, "SourceView", 50); //Source
    insertMenuBuilderBools(&guiBool, "DisassemblerGraphView", 50); //Graph
    insertMenuBuilderBools(&guiBool, "XrefBrowseDialog", 50); //XrefBrowseDialog
    insertMenuBuilderBools(&guiBool, "StructWidget", 50); //StructWidget
    insertMenuBuilderBools(&guiBool, "File", 50); //Main Menu : File
    insertMenuBuilderBools(&guiBool, "Debug", 50); //Main Menu : Debug
    insertMenuBuilderBools(&guiBool, "Option", 50); //Main Menu : Option
    //"Favourites" menu cannot be customized for item hiding.
    insertMenuBuilderBools(&guiBool, "Help", 50); //Main Menu : Help
    insertMenuBuilderBools(&guiBool, "View", 50); //Main Menu : View
    defaultBools.insert("Gui", guiBool);

    QMap<QString, duint> guiUint;
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "CPUDisassembly", 4);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "CPUStack", 3);
    for(int i = 1; i <= 5; i++)
        AbstractTableView::setupColumnConfigDefaultValue(guiUint, QString("CPUDump%1").arg(i), 4);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "Watch1", 6);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "BreakpointsView", 7);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "MemoryMap", 8);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "CallStack", 7);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "SEH", 4);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "Script", 3);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "Thread", 14);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "Handle", 5);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "Window", 10);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "TcpConnection", 3);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "Privilege", 2);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "LocalVarsView", 3);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "Module", 5);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "Symbol", 5);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "SourceView", 4);
    AbstractTableView::setupColumnConfigDefaultValue(guiUint, "Trace", 7);
    guiUint.insert("SIMDRegistersDisplayMode", 0);
    guiUint.insert("EditFloatRegisterDefaultMode", 0);
    defaultUints.insert("Gui", guiUint);

    //uint settings
    QMap<QString, duint> hexdumpUint;
    hexdumpUint.insert("DefaultView", 0);
    hexdumpUint.insert("CopyDataType", 0);
    defaultUints.insert("HexDump", hexdumpUint);
    QMap<QString, bool> hexdumpBool;
    hexdumpBool.insert("KeepSize", false);
    defaultBools.insert("HexDump", hexdumpBool);

    QMap<QString, duint> disasmUint;
    disasmUint.insert("MaxModuleSize", -1);
    defaultUints.insert("Disassembler", disasmUint);

    //font settings
    QFont font("Lucida Console", 8, QFont::Normal, false);
    defaultFonts.insert("AbstractTableView", font);
    defaultFonts.insert("Disassembly", font);
    defaultFonts.insert("HexDump", font);
    defaultFonts.insert("Stack", font);
    defaultFonts.insert("Registers", font);
    defaultFonts.insert("HexEdit", font);
    defaultFonts.insert("Application", QApplication::font());
    defaultFonts.insert("Log", QFont("Courier New", 8, QFont::Normal, false));

    // hotkeys settings
    defaultShortcuts.insert("FileOpen", Shortcut({tr("File"), tr("Open")}, "F3", true));
    defaultShortcuts.insert("FileAttach", Shortcut({tr("File"), tr("Attach")}, "Alt+A", true));
    defaultShortcuts.insert("FileDetach", Shortcut({tr("File"), tr("Detach")}, "Ctrl+Alt+F2", true));
    defaultShortcuts.insert("FileDbsave", Shortcut({tr("File"), tr("Save database")}, "", true));
    defaultShortcuts.insert("FileDbrecovery", Shortcut({tr("File"), tr("Restore backup database")}, "", true));
    defaultShortcuts.insert("FileDbload", Shortcut({tr("File"), tr("Reload database")}, "", true));
    defaultShortcuts.insert("FileDbclear", Shortcut({tr("File"), tr("Clear database")}, "", true));
    defaultShortcuts.insert("FileImportDatabase", Shortcut({tr("File"), tr("Import database")}, "", true));
    defaultShortcuts.insert("FileExportDatabase", Shortcut({tr("File"), tr("Export database")}, "", true));
    defaultShortcuts.insert("FileRestartAdmin", Shortcut({tr("File"), tr("Restart as Admin")}, "", true));
    defaultShortcuts.insert("FileExit", Shortcut({tr("File"), tr("Exit")}, "Alt+X", true));

    defaultShortcuts.insert("ViewCpu", Shortcut({tr("View"), tr("CPU")}, "Alt+C", true));
    defaultShortcuts.insert("ViewLog", Shortcut({tr("View"), tr("Log")}, "Alt+L", true));
    defaultShortcuts.insert("ViewBreakpoints", Shortcut({tr("View"), tr("Breakpoints")}, "Alt+B", true));
    defaultShortcuts.insert("ViewMemoryMap", Shortcut({tr("View"), tr("Memory Map")}, "Alt+M", true));
    defaultShortcuts.insert("ViewCallStack", Shortcut({tr("View"), tr("Call Stack")}, "Alt+K", true));
    defaultShortcuts.insert("ViewNotes", Shortcut({tr("View"), tr("Notes")}, "Alt+N", true));
    defaultShortcuts.insert("ViewSEHChain", Shortcut({tr("View"), tr("SEH")}, "", true));
    defaultShortcuts.insert("ViewScript", Shortcut({tr("View"), tr("Script")}, "Alt+S", true));
    defaultShortcuts.insert("ViewSymbolInfo", Shortcut({tr("View"), tr("Symbol Info")}, "Ctrl+Alt+S", true));
    defaultShortcuts.insert("ViewModules", Shortcut({tr("View"), tr("Modules")}, "Alt+E", true));
    defaultShortcuts.insert("ViewSource", Shortcut({tr("View"), tr("Source")}, "Ctrl+Shift+S", true));
    defaultShortcuts.insert("ViewReferences", Shortcut({tr("View"), tr("References")}, "Alt+R", true));
    defaultShortcuts.insert("ViewThreads", Shortcut({tr("View"), tr("Threads")}, "Alt+T", true));
    defaultShortcuts.insert("ViewPatches", Shortcut({tr("View"), tr("Patches")}, "Ctrl+P", true));
    defaultShortcuts.insert("ViewComments", Shortcut({tr("View"), tr("Comments")}, "Ctrl+Alt+C", true));
    defaultShortcuts.insert("ViewLabels", Shortcut({tr("View"), tr("Labels")}, "Ctrl+Alt+L", true));
    defaultShortcuts.insert("ViewBookmarks", Shortcut({tr("View"), tr("Bookmarks")}, "Ctrl+Alt+B", true));
    defaultShortcuts.insert("ViewFunctions", Shortcut({tr("View"), tr("Functions")}, "Ctrl+Alt+F", true));
    defaultShortcuts.insert("ViewVariables", Shortcut({tr("View"), tr("Variables")}, "", true));
    defaultShortcuts.insert("ViewHandles", Shortcut({tr("View"), tr("Handles")}, "", true));
    defaultShortcuts.insert("ViewGraph", Shortcut({tr("View"), tr("Graph")}, "Alt+G", true));
    defaultShortcuts.insert("ViewPreviousTab", Shortcut({tr("View"), tr("Previous Tab")}, "Alt+Left"));
    defaultShortcuts.insert("ViewNextTab", Shortcut({tr("View"), tr("Next Tab")}, "Alt+Right"));
    defaultShortcuts.insert("ViewPreviousHistory", Shortcut({tr("View"), tr("Previous View")}, "Ctrl+Shift+Tab"));
    defaultShortcuts.insert("ViewNextHistory", Shortcut({tr("View"), tr("Next View")}, "Ctrl+Tab"));
    defaultShortcuts.insert("ViewHideTab", Shortcut({tr("View"), tr("Hide Tab")}, "Ctrl+W"));

    defaultShortcuts.insert("DebugRun", Shortcut({tr("Debug"), tr("Run")}, "F9", true));
    defaultShortcuts.insert("DebugeRun", Shortcut({tr("Debug"), tr("Run (pass exception)")}, "Shift+F9", true));
    defaultShortcuts.insert("DebugseRun", Shortcut({tr("Debug"), tr("Run (swallow exception)")}, "Ctrl+Alt+Shift+F9", true));
    defaultShortcuts.insert("DebugRunSelection", Shortcut({tr("Debug"), tr("Run until selection")}, "F4", true));
    defaultShortcuts.insert("DebugRunExpression", Shortcut({tr("Debug"), tr("Run until expression")}, "Shift+F4", true));
    defaultShortcuts.insert("DebugPause", Shortcut({tr("Debug"), tr("Pause")}, "F12", true));
    defaultShortcuts.insert("DebugRestart", Shortcut({tr("Debug"), tr("Restart")}, "Ctrl+F2", true));
    defaultShortcuts.insert("DebugClose", Shortcut({tr("Debug"), tr("Close")}, "Alt+F2", true));
    defaultShortcuts.insert("DebugStepInto", Shortcut({tr("Debug"), tr("Step into")}, "F7", true));
    defaultShortcuts.insert("DebugeStepInto", Shortcut({tr("Debug"), tr("Step into (pass exception)")}, "Shift+F7", true));
    defaultShortcuts.insert("DebugseStepInto", Shortcut({tr("Debug"), tr("Step into (swallow exception)")}, "Ctrl+Alt+Shift+F7", true));
    defaultShortcuts.insert("DebugStepIntoSource", Shortcut({tr("Debug"), tr("Step into (source)")}, "F11", true));
    defaultShortcuts.insert("DebugStepOver", Shortcut({tr("Debug"), tr("Step over")}, "F8", true));
    defaultShortcuts.insert("DebugeStepOver", Shortcut({tr("Debug"), tr("Step over (pass exception)")}, "Shift+F8", true));
    defaultShortcuts.insert("DebugseStepOver", Shortcut({tr("Debug"), tr("Step over (swallow exception)")}, "Ctrl+Alt+Shift+F8", true));
    defaultShortcuts.insert("DebugStepOverSource", Shortcut({tr("Debug"), tr("Step over (source)")}, "F10", true));
    defaultShortcuts.insert("DebugRtr", Shortcut({tr("Debug"), tr("Execute till return")}, "Ctrl+F9", true));
    defaultShortcuts.insert("DebugeRtr", Shortcut({tr("Debug"), tr("Execute till return (pass exception)")}, "Ctrl+Shift+F9", true));
    defaultShortcuts.insert("DebugRtu", Shortcut({tr("Debug"), tr("Run to user code")}, "Alt+F9", true));
    defaultShortcuts.insert("DebugSkipNextInstruction", Shortcut({tr("Debug"), tr("Skip next instruction")}, "", true));
    defaultShortcuts.insert("DebugCommand", Shortcut({tr("Debug"), tr("Command")}, "Ctrl+Return", true));
    defaultShortcuts.insert("DebugTraceIntoConditional", Shortcut({tr("Debug"), tr("Trace into...")}, "Ctrl+Alt+F7", true));
    defaultShortcuts.insert("DebugTraceOverConditional", Shortcut({tr("Debug"), tr("Trace over...")}, "Ctrl+Alt+F8", true));
    defaultShortcuts.insert("DebugEnableTraceRecordBit", Shortcut({tr("Debug"), tr("Trace coverage"), tr("Bit")}, "", true));
    defaultShortcuts.insert("DebugTraceRecordNone", Shortcut({tr("Debug"), tr("Trace coverage"), tr("None")}, "", true));
    defaultShortcuts.insert("DebugInstrUndo", Shortcut({tr("Debug"), tr("Undo instruction")}, "Alt+U", true));
    defaultShortcuts.insert("DebugAnimateInto", Shortcut({tr("Debug"), tr("Animate into")}, "Ctrl+F7", true));
    defaultShortcuts.insert("DebugAnimateOver", Shortcut({tr("Debug"), tr("Animate over")}, "Ctrl+F8", true));
    defaultShortcuts.insert("DebugAnimateCommand", Shortcut({tr("Debug"), tr("Animate command")}, "", true));
    defaultShortcuts.insert("DebugTraceIntoIntoTracerecord", Shortcut({tr("Debug"), tr("Step into until reaching uncovered code")}, "", true));
    defaultShortcuts.insert("DebugTraceOverIntoTracerecord", Shortcut({tr("Debug"), tr("Step over until reaching uncovered code")}, "", true));
    defaultShortcuts.insert("DebugTraceIntoBeyondTracerecord", Shortcut({tr("Debug"), tr("Step into until reaching covered code")}, "", true));
    defaultShortcuts.insert("DebugTraceOverBeyondTracerecord", Shortcut({tr("Debug"), tr("Step over until reaching covered code")}, "", true));

    defaultShortcuts.insert("PluginsScylla", Shortcut({tr("Plugins"), tr("Scylla")}, "Ctrl+I", true));

    defaultShortcuts.insert("FavouritesManage", Shortcut({tr("Favourites"), tr("Manage Favourite Tools")}, "", true));

    defaultShortcuts.insert("OptionsPreferences", Shortcut({tr("Options"), tr("Preferences")}, "", true));
    defaultShortcuts.insert("OptionsAppearance", Shortcut({tr("Options"), tr("Appearance")}, "", true));
    defaultShortcuts.insert("OptionsShortcuts", Shortcut({tr("Options"), tr("Hotkeys")}, "", true));
    defaultShortcuts.insert("OptionsTopmost", Shortcut({tr("Options"), tr("Topmost")}, "Ctrl+F5", true));
    defaultShortcuts.insert("OptionsReloadStylesheet", Shortcut({tr("Options"), tr("Reload style.css")}, "", true));

    defaultShortcuts.insert("HelpAbout", Shortcut({tr("Help"), tr("About")}, "", true));
    defaultShortcuts.insert("HelpBlog", Shortcut({tr("Help"), tr("Blog")}, "", true));
    defaultShortcuts.insert("HelpDonate", Shortcut({tr("Help"), tr("Donate")}, "", true));
    defaultShortcuts.insert("HelpCalculator", Shortcut({tr("Help"), tr("Calculator")}, "?"));
    defaultShortcuts.insert("HelpReportBug", Shortcut({tr("Help"), tr("Report Bug")}, "", true));
    defaultShortcuts.insert("HelpManual", Shortcut({tr("Help"), tr("Manual")}, "F1", true));
    defaultShortcuts.insert("HelpCrashDump", Shortcut({tr("Help"), tr("Generate Crash Dump")}, "", true));

    defaultShortcuts.insert("ActionFindStrings", Shortcut({tr("Actions"), tr("Find Strings")}, "", true));
    defaultShortcuts.insert("ActionFindStringsModule", Shortcut({tr("Actions"), tr("Find Strings in Current Module")}, "Shift+D", true));
    defaultShortcuts.insert("ActionFindIntermodularCalls", Shortcut({tr("Actions"), tr("Find Intermodular Calls")}, "", true));
    defaultShortcuts.insert("ActionToggleBreakpoint", Shortcut({tr("Actions"), tr("Toggle Breakpoint")}, "F2"));
    defaultShortcuts.insert("ActionEditBreakpoint", Shortcut({tr("Actions"), tr("Set Conditional Breakpoint")}, "Shift+F2"));
    defaultShortcuts.insert("ActionToggleBookmark", Shortcut({tr("Actions"), tr("Toggle Bookmark")}, "Ctrl+D"));
    defaultShortcuts.insert("ActionDeleteBreakpoint", Shortcut({tr("Actions"), tr("Delete Breakpoint")}, "Delete"));
    defaultShortcuts.insert("ActionEnableDisableBreakpoint", Shortcut({tr("Actions"), tr("Enable/Disable Breakpoint")}, "Space"));
    defaultShortcuts.insert("ActionResetHitCountBreakpoint", Shortcut({tr("Actions"), tr("Reset breakpoint hit count")}));
    defaultShortcuts.insert("ActionEnableAllBreakpoints", Shortcut({tr("Actions"), tr("Enable all breakpoints")}));
    defaultShortcuts.insert("ActionDisableAllBreakpoints", Shortcut({tr("Actions"), tr("Disable all breakpoints")}));
    defaultShortcuts.insert("ActionRemoveAllBreakpoints", Shortcut({tr("Actions"), tr("Remove all breakpoints")}));

    defaultShortcuts.insert("ActionBinaryEdit", Shortcut({tr("Actions"), tr("Binary Edit")}, "Ctrl+E"));
    defaultShortcuts.insert("ActionBinaryFill", Shortcut({tr("Actions"), tr("Binary Fill")}, "F"));
    defaultShortcuts.insert("ActionBinaryFillNops", Shortcut({tr("Actions"), tr("Binary Fill NOPs")}, "Ctrl+9"));
    defaultShortcuts.insert("ActionBinaryCopy", Shortcut({tr("Actions"), tr("Binary Copy")}, "Shift+C"));
    defaultShortcuts.insert("ActionBinaryPaste", Shortcut({tr("Actions"), tr("Binary Paste")}, "Shift+V"));
    defaultShortcuts.insert("ActionBinaryPasteIgnoreSize", Shortcut({tr("Actions"), tr("Binary Paste (Ignore Size)")}, "Ctrl+Shift+V"));
    defaultShortcuts.insert("ActionBinarySave", Shortcut({tr("Actions"), tr("Binary Save")}));
    defaultShortcuts.insert("ActionUndoSelection", Shortcut({tr("Actions"), tr("Undo Selection")}, "Ctrl+Backspace"));
    defaultShortcuts.insert("ActionSetLabel", Shortcut({tr("Actions"), tr("Set Label")}, ":"));
    defaultShortcuts.insert("ActionSetLabelOperand", Shortcut({tr("Actions"), tr("Set Label for the Operand")}, "Alt+;"));
    defaultShortcuts.insert("ActionSetComment", Shortcut({tr("Actions"), tr("Set Comment")}, ";"));
    defaultShortcuts.insert("ActionToggleFunction", Shortcut({tr("Actions"), tr("Toggle Function")}, "Shift+F"));
    defaultShortcuts.insert("ActionAddLoop", Shortcut({tr("Actions"), tr("Add Loop")}, "Shift+L"));
    defaultShortcuts.insert("ActionDeleteLoop", Shortcut({tr("Actions"), tr("Delete Loop")}, "Ctrl+Shift+L"));
    defaultShortcuts.insert("ActionToggleArgument", Shortcut({tr("Actions"), tr("Toggle Argument")}, "Shift+A"));
    defaultShortcuts.insert("ActionAssemble", Shortcut({tr("Actions"), tr("Assemble")}, "Space"));
    defaultShortcuts.insert("ActionSetNewOriginHere", Shortcut({tr("Actions"), tr("Set %1 Here").arg(ArchValue("EIP", "RIP"))}, "Ctrl+*"));
    defaultShortcuts.insert("ActionGotoOrigin", Shortcut({tr("Actions"), tr("Goto Origin")}, "*"));
    defaultShortcuts.insert("ActionGotoCBP", Shortcut({tr("Actions"), tr("Goto EBP/RBP")}));
    defaultShortcuts.insert("ActionGotoPrevious", Shortcut({tr("Actions"), tr("Goto Previous")}, "-"));
    defaultShortcuts.insert("ActionGotoNext", Shortcut({tr("Actions"), tr("Goto Next")}, "+"));
    defaultShortcuts.insert("ActionGotoExpression", Shortcut({tr("Actions"), tr("Goto Expression")}, "Ctrl+G"));
    defaultShortcuts.insert("ActionGotoStart", Shortcut({tr("Actions"), tr("Goto Start of Page")}, "Home"));
    defaultShortcuts.insert("ActionGotoEnd", Shortcut({tr("Actions"), tr("Goto End of Page")}, "End"));
    defaultShortcuts.insert("ActionGotoFunctionStart", Shortcut({tr("Actions"), tr("Goto Start of Function")}, "Ctrl+Home"));
    defaultShortcuts.insert("ActionGotoFunctionEnd", Shortcut({tr("Actions"), tr("Goto End of Function")}, "Ctrl+End"));
    defaultShortcuts.insert("ActionGotoFileOffset", Shortcut({tr("Actions"), tr("Goto File Offset")}, "Ctrl+Shift+G"));
    defaultShortcuts.insert("ActionFindReferencesToSelectedAddress", Shortcut({tr("Actions"), tr("Find References to Selected Address")}, "Ctrl+R"));
    defaultShortcuts.insert("ActionFindPattern", Shortcut({tr("Actions"), tr("Find Pattern")}, "Ctrl+B"));
    defaultShortcuts.insert("ActionFindPatternInModule", Shortcut({tr("Actions"), tr("Find Pattern in Current Module")}, "Ctrl+Shift+B"));
    defaultShortcuts.insert("ActionFindNamesInModule", Shortcut({tr("Actions"), tr("Find Names in Current Module")}, "Ctrl+N"));
    defaultShortcuts.insert("ActionFindReferences", Shortcut({tr("Actions"), tr("Find References")}, "Ctrl+R"));
    defaultShortcuts.insert("ActionXrefs", Shortcut({tr("Actions"), tr("xrefs...")}, "X"));
    defaultShortcuts.insert("ActionAnalyzeSingleFunction", Shortcut({tr("Actions"), tr("Analyze Single Function")}, "A"));
    defaultShortcuts.insert("ActionAnalyzeModule", Shortcut({tr("Actions"), tr("Analyze Module")}, "Ctrl+A"));
    defaultShortcuts.insert("ActionHelpOnMnemonic", Shortcut({tr("Actions"), tr("Help on Mnemonic")}, "Ctrl+F1"));
    defaultShortcuts.insert("ActionToggleMnemonicBrief", Shortcut({tr("Actions"), tr("Toggle Mnemonic Brief")}, "Ctrl+Shift+F1"));
    defaultShortcuts.insert("ActionHighlightingMode", Shortcut({tr("Actions"), tr("Highlighting Mode")}, "H"));
    defaultShortcuts.insert("ActionToggleDestinationPreview", Shortcut({tr("Actions"), tr("Enable/Disable Branch Destination Preview")}, "P"));
    defaultShortcuts.insert("ActionFind", Shortcut({tr("Actions"), tr("Find")}, "Ctrl+F"));
    defaultShortcuts.insert("ActionFindInModule", Shortcut({tr("Actions"), tr("Find in Current Module")}, "Ctrl+Shift+F"));
    defaultShortcuts.insert("ActionToggleLogging", Shortcut({tr("Actions"), tr("Enable/Disable Logging")}, ""));
    defaultShortcuts.insert("ActionAllocateMemory", Shortcut({tr("Actions"), tr("Allocate Memory")}, ""));
    defaultShortcuts.insert("ActionFreeMemory", Shortcut({tr("Actions"), tr("Free Memory")}, ""));
    defaultShortcuts.insert("ActionSync", Shortcut({tr("Actions"), tr("Sync")}, "S"));
    defaultShortcuts.insert("ActionCopyAllRegisters", Shortcut({tr("Actions"), tr("Copy All Registers")}, ""));
    defaultShortcuts.insert("ActionMarkAsUser", Shortcut({tr("Actions"), tr("Mark As User Module")}, ""));
    defaultShortcuts.insert("ActionMarkAsSystem", Shortcut({tr("Actions"), tr("Mark As System Module")}, ""));
    defaultShortcuts.insert("ActionMarkAsParty", Shortcut({tr("Actions"), tr("Mark As Party")}, ""));
    defaultShortcuts.insert("ActionSetHwBpE", Shortcut({tr("Actions"), tr("Set Hardware Breakpoint (Execute)")}, ""));
    defaultShortcuts.insert("ActionRemoveHwBp", Shortcut({tr("Actions"), tr("Remove Hardware Breakpoint")}, ""));
    defaultShortcuts.insert("ActionRemoveTypeAnalysisFromModule", Shortcut({tr("Actions"), tr("Remove Type Analysis From Module")}, "Ctrl+Shift+U"));
    defaultShortcuts.insert("ActionRemoveTypeAnalysisFromSelection", Shortcut({tr("Actions"), tr("Remove Type Analysis From Selection")}, "U"));
    defaultShortcuts.insert("ActionTreatSelectionAsCode", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("Code")}, "C"));
    defaultShortcuts.insert("ActionTreatSelectionAsByte", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("Byte")}, "B"));
    defaultShortcuts.insert("ActionTreatSelectionAsWord", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("Word")}, "W"));
    defaultShortcuts.insert("ActionTreatSelectionAsDword", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("Dword")}, "D"));
    defaultShortcuts.insert("ActionTreatSelectionAsFword", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("Fword")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionAsQword", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("Qword")}, "Q"));
    defaultShortcuts.insert("ActionTreatSelectionAsTbyte", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("Tbyte")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionAsOword", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("Oword")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionAsFloat", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("Float")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionAsDouble", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("Double")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionAsLongDouble", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("LongDouble")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionAsASCII", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("ASCII")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionAsUNICODE", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("UNICODE")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionAsMMWord", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("MMWord")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionAsXMMWord", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("XMMWord")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionAsYMMWord", Shortcut({tr("Actions"), tr("Treat Selection As"), tr("YMMWord")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsCode", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("Code")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsByte", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("Byte")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsWord", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("Word")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsDword", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("Dword")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsFword", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("Fword")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsQword", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("Qword")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsTbyte", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("Tbyte")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsOword", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("Oword")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsFloat", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("Float")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsDouble", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("Double")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsLongDouble", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("LongDouble")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsASCII", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("ASCII")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsUNICODE", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("UNICODE")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsMMWord", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("MMWord")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsXMMWord", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("XMMWord")}, ""));
    defaultShortcuts.insert("ActionTreatSelectionHeadAsYMMWord", Shortcut({tr("Actions"), tr("Treat Selection Head As"), tr("YMMWord")}, ""));
    defaultShortcuts.insert("ActionToggleRegisterValue", Shortcut({tr("Actions"), tr("Toggle Register Value")}, "Space"));
    defaultShortcuts.insert("ActionClear", Shortcut({tr("Actions"), tr("Clear")}, "Ctrl+L"));
    defaultShortcuts.insert("ActionCopy", Shortcut({tr("Actions"), tr("Copy")}, "Ctrl+C"));
    defaultShortcuts.insert("ActionCopyAddress", Shortcut({tr("Actions"), tr("Copy Address")}, "Alt+INS"));
    defaultShortcuts.insert("ActionCopyRva", Shortcut({tr("Actions"), tr("Copy RVA")}, ""));
    defaultShortcuts.insert("ActionCopySymbol", Shortcut({tr("Actions"), tr("Copy Symbol")}, "Ctrl+S"));
    defaultShortcuts.insert("ActionCopyLine", Shortcut({tr("Actions"), tr("Copy Line")}, ""));
    defaultShortcuts.insert("ActionLoadScript", Shortcut({tr("Actions"), tr("Load Script")}, "Ctrl+O"));
    defaultShortcuts.insert("ActionReloadScript", Shortcut({tr("Actions"), tr("Reload Script")}, "Ctrl+R"));
    defaultShortcuts.insert("ActionUnloadScript", Shortcut({tr("Actions"), tr("Unload Script")}, "Ctrl+U"));
    defaultShortcuts.insert("ActionEditScript", Shortcut({tr("Actions"), tr("Edit Script")}, ""));
    defaultShortcuts.insert("ActionRunScript", Shortcut({tr("Actions"), tr("Run Script")}, "Space"));
    defaultShortcuts.insert("ActionToggleBreakpointScript", Shortcut({tr("Actions"), tr("Toggle Script Breakpoint")}, "F2"));
    defaultShortcuts.insert("ActionRunToCursorScript", Shortcut({tr("Actions"), tr("Run Script to Cursor")}, "Shift+F4"));
    defaultShortcuts.insert("ActionStepScript", Shortcut({tr("Actions"), tr("Step Script")}, "Tab"));
    defaultShortcuts.insert("ActionAbortScript", Shortcut({tr("Actions"), tr("Abort Script")}, "Esc"));
    defaultShortcuts.insert("ActionExecuteCommandScript", Shortcut({tr("Actions"), tr("Execute Script Command")}, "X"));
    defaultShortcuts.insert("ActionRefresh", Shortcut({tr("Actions"), tr("Refresh")}, "F5"));
    defaultShortcuts.insert("ActionGraph", Shortcut({tr("Actions"), tr("Graph")}, "G"));
    defaultShortcuts.insert("ActionGraphZoomToCursor", Shortcut({tr("Actions"), tr("Graph"), tr("Zoom to cursor")}, "Z"));
    defaultShortcuts.insert("ActionGraphFitToWindow", Shortcut({tr("Actions"), tr("Graph"), tr("Fit To Window")}, "Shift+Z"));
    defaultShortcuts.insert("ActionGraphFollowDisassembler", Shortcut({tr("Actions"), tr("Graph"), tr("Follow in disassembler")}, "Shift+Return"));
    defaultShortcuts.insert("ActionGraphSaveImage", Shortcut({tr("Actions"), tr("Graph"), tr("Save as image")}, "I"));
    defaultShortcuts.insert("ActionGraphToggleOverview", Shortcut({tr("Actions"), tr("Graph"), tr("Toggle overview")}, "O"));
    defaultShortcuts.insert("ActionGraphToggleSummary", Shortcut({tr("Actions"), tr("Graph"), tr("Toggle summary")}, "U"));
    defaultShortcuts.insert("ActionIncrementx87Stack", Shortcut({tr("Actions"), tr("Increment x87 Stack")}));
    defaultShortcuts.insert("ActionDecrementx87Stack", Shortcut({tr("Actions"), tr("Decrement x87 Stack")}));
    defaultShortcuts.insert("ActionRedirectLog", Shortcut({tr("Actions"), tr("Redirect Log")}));
    defaultShortcuts.insert("ActionBrowseInExplorer", Shortcut({tr("Actions"), tr("Browse in Explorer")}));
    defaultShortcuts.insert("ActionDownloadSymbol", Shortcut({tr("Actions"), tr("Download Symbols for This Module")}));
    defaultShortcuts.insert("ActionDownloadAllSymbol", Shortcut({tr("Actions"), tr("Download Symbols for All Modules")}));
    defaultShortcuts.insert("ActionCreateNewThreadHere", Shortcut({tr("Actions"), tr("Create New Thread Here")}));
    defaultShortcuts.insert("ActionOpenSourceFile", Shortcut({tr("Actions"), tr("Open Source File")}));
    defaultShortcuts.insert("ActionFollowMemMap", Shortcut({tr("Actions"), tr("Follow in Memory Map")}));
    defaultShortcuts.insert("ActionFollowStack", Shortcut({tr("Actions"), tr("Follow in Stack")}));
    defaultShortcuts.insert("ActionFollowDisasm", Shortcut({tr("Actions"), tr("Follow in Disassembler")}));
    defaultShortcuts.insert("ActionFollowDwordQwordDisasm", Shortcut({tr("Actions"), tr("Follow DWORD/QWORD in Disassembler")}));
    defaultShortcuts.insert("ActionFollowDwordQwordDump", Shortcut({tr("Actions"), tr("Follow DWORD/QWORD in Dump")}));
    defaultShortcuts.insert("ActionFreezeStack", Shortcut({tr("Actions"), tr("Freeze the stack")}));
    defaultShortcuts.insert("ActionGotoBaseOfStackFrame", Shortcut({tr("Actions"), tr("Go to Base of Stack Frame")}));
    defaultShortcuts.insert("ActionGotoPrevStackFrame", Shortcut({tr("Actions"), tr("Go to Previous Stack Frame")}));
    defaultShortcuts.insert("ActionGotoNextStackFrame", Shortcut({tr("Actions"), tr("Go to Next Stack Frame")}));
    defaultShortcuts.insert("ActionGotoPreviousReference", Shortcut({tr("Actions"), tr("Go to Previous Reference")}, "Ctrl+K"));
    defaultShortcuts.insert("ActionGotoNextReference", Shortcut({tr("Actions"), tr("Go to Next Reference")}, "Ctrl+L"));
    defaultShortcuts.insert("ActionModifyValue", Shortcut({tr("Actions"), tr("Modify value")}, "Space"));
    defaultShortcuts.insert("ActionWatchDwordQword", Shortcut({tr("Actions"), tr("Watch DWORD/QWORD")}));
    defaultShortcuts.insert("ActionCopyFileOffset", Shortcut({tr("Actions"), tr("Copy File Offset")}));
    defaultShortcuts.insert("ActionToggleRunTrace", Shortcut({tr("Actions"), tr("Start/Stop trace recording")}));

    defaultShortcuts.insert("ActionCopyCroppedTable", Shortcut({tr("Actions"), tr("Copy -> Cropped Table")}));
    defaultShortcuts.insert("ActionCopyTable", Shortcut({tr("Actions"), tr("Copy -> Table")}));
    defaultShortcuts.insert("ActionCopyLineToLog", Shortcut({tr("Actions"), tr("Copy -> Line, To Log")}));
    defaultShortcuts.insert("ActionCopyCroppedTableToLog", Shortcut({tr("Actions"), tr("Copy -> Cropped Table, To Log")}));
    defaultShortcuts.insert("ActionCopyTableToLog", Shortcut({tr("Actions"), tr("Copy -> Table, To Log")}));
    defaultShortcuts.insert("ActionExport", Shortcut({tr("Actions"), tr("Copy -> Export Table")}));

    Shortcuts = defaultShortcuts;

    load();

    //because we changed the default this needs special handling for old configurations
    if(Shortcuts["ViewPreviousTab"].Hotkey.toString() == Shortcuts["ViewPreviousHistory"].Hotkey.toString())
    {
        Shortcuts["ViewPreviousTab"].Hotkey = defaultShortcuts["ViewPreviousTab"].Hotkey;
        save();
    }
    if(Shortcuts["ViewNextTab"].Hotkey.toString() == Shortcuts["ViewNextHistory"].Hotkey.toString())
    {
        Shortcuts["ViewNextTab"].Hotkey = defaultShortcuts["ViewNextTab"].Hotkey;
        save();
    }
}

Configuration* Configuration::instance()
{
    return mPtr;
}

void Configuration::load()
{
    readColors();
    readBools();
    readUints();
    readFonts();
    readShortcuts();
}

void Configuration::save()
{
    writeColors();
    writeBools();
    writeUints();
    writeFonts();
    writeShortcuts();
}

void Configuration::readColors()
{
    Colors = defaultColors;
    //read config
    for(auto it = Colors.begin(); it != Colors.end(); ++it)
        it.value() = colorFromConfig(it.key());
}

void Configuration::writeColors()
{
    //write config
    for(auto it = Colors.begin(); it != Colors.end(); ++it)
        colorToConfig(it.key(), it.value());
    emit colorsUpdated();
}

void Configuration::readBools()
{
    Bools = defaultBools;
    //read config
    for(auto itMap = Bools.begin(); itMap != Bools.end(); ++itMap)
    {
        const QString & category = itMap.key();
        for(auto it = itMap.value().begin(); it != itMap.value().end(); it++)
        {
            it.value() = boolFromConfig(category, it.key());
        }
    }
}

void Configuration::writeBools()
{
    //write config
    for(auto itMap = Bools.cbegin(); itMap != Bools.cend(); ++itMap)
    {
        const QString & category = itMap.key();
        for(auto it = itMap.value().cbegin(); it != itMap.value().cend(); it++)
        {
            boolToConfig(category, it.key(), it.value());
        }
    }
}

void Configuration::readUints()
{
    Uints = defaultUints;
    //read config
    for(auto itMap = Uints.begin(); itMap != Uints.end(); ++itMap)
    {
        const QString & category = itMap.key();
        for(auto it = itMap.value().begin(); it != itMap.value().end(); it++)
        {
            it.value() = uintFromConfig(category, it.key());
        }
    }
}

void Configuration::writeUints()
{
    //write config
    for(auto itMap = Uints.cbegin(); itMap != Uints.cend(); ++itMap)
    {
        const QString & category = itMap.key();
        for(auto it = itMap.value().cbegin(); it != itMap.value().cend(); it++)
        {
            uintToConfig(category, it.key(), it.value());
        }
    }
}

void Configuration::readFonts()
{
    Fonts = defaultFonts;
    //read config
    for(auto it = Fonts.begin(); it != Fonts.end(); ++it)
    {
        const QString & id = it.key();
        QFont font = fontFromConfig(id);
        QFontInfo fontInfo(font);
        if(id == "Application" || fontInfo.fixedPitch())
            it.value() = font;
    }
}

void Configuration::writeFonts()
{
    //write config
    for(auto it = Fonts.cbegin(); it != Fonts.cend(); ++it)
        fontToConfig(it.key(), it.value());
    emit fontsUpdated();
}

void Configuration::readShortcuts()
{
    Shortcuts = defaultShortcuts;
    QMap<QString, Shortcut>::const_iterator it = Shortcuts.begin();

    while(it != Shortcuts.end())
    {
        const QString & id = it.key();
        QString key = shortcutFromConfig(id);
        if(key != "")
        {
            if(key == "NOT_SET")
                Shortcuts[it.key()].Hotkey = QKeySequence();
            else
            {
                QKeySequence KeySequence(key);
                Shortcuts[it.key()].Hotkey = KeySequence;
            }
        }
        it++;
    }
    emit shortcutsUpdated();
}

void Configuration::writeShortcuts()
{
    QMap<QString, Shortcut>::const_iterator it = Shortcuts.begin();

    while(it != Shortcuts.end())
    {
        shortcutToConfig(it.key(), it.value().Hotkey);
        it++;
    }
    emit shortcutsUpdated();
}

const QColor Configuration::getColor(const QString & id) const
{
    if(Colors.contains(id))
        return Colors.constFind(id).value();
    if(noMoreMsgbox)
        return Qt::black;
    QMessageBox msg(QMessageBox::Warning, tr("NOT FOUND IN CONFIG!"), id, QMessageBox::Retry | QMessageBox::Cancel);
    msg.setWindowIcon(DIcon("compile-warning"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    if(msg.exec() == QMessageBox::Cancel)
        noMoreMsgbox = true;
    return Qt::black;
}

const bool Configuration::getBool(const QString & category, const QString & id) const
{
    if(Bools.contains(category))
    {
        if(Bools[category].contains(id))
            return Bools[category][id];
        if(noMoreMsgbox)
            return false;
        QMessageBox msg(QMessageBox::Warning, tr("NOT FOUND IN CONFIG!"), category + ":" + id, QMessageBox::Retry | QMessageBox::Cancel); /* insertMenuBuilderBools */
        msg.setWindowIcon(DIcon("compile-warning"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::Cancel)
            noMoreMsgbox = true;
        return false;
    }
    if(noMoreMsgbox)
        return false;
    QMessageBox msg(QMessageBox::Warning, tr("NOT FOUND IN CONFIG!"), category, QMessageBox::Retry | QMessageBox::Cancel);
    msg.setWindowIcon(DIcon("compile-warning"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    if(msg.exec() == QMessageBox::Cancel)
        noMoreMsgbox = true;
    return false;
}

void Configuration::setBool(const QString & category, const QString & id, const bool b)
{
    if(Bools.contains(category))
    {
        if(Bools[category].contains(id))
        {
            Bools[category][id] = b;
            return;
        }
        if(noMoreMsgbox)
            return;
        QMessageBox msg(QMessageBox::Warning, tr("NOT FOUND IN CONFIG!"), category + ":" + id, QMessageBox::Retry | QMessageBox::Cancel);
        msg.setWindowIcon(DIcon("compile-warning"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::Cancel)
            noMoreMsgbox = true;
        return;
    }
    if(noMoreMsgbox)
        return;
    QMessageBox msg(QMessageBox::Warning, tr("NOT FOUND IN CONFIG!"), category, QMessageBox::Retry | QMessageBox::Cancel);
    msg.setWindowIcon(DIcon("compile-warning"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    if(msg.exec() == QMessageBox::Cancel)
        noMoreMsgbox = true;
}

const duint Configuration::getUint(const QString & category, const QString & id) const
{
    if(Uints.contains(category))
    {
        if(Uints[category].contains(id))
            return Uints[category][id];
        if(noMoreMsgbox)
            return 0;
        QMessageBox msg(QMessageBox::Warning, tr("NOT FOUND IN CONFIG!"), category + ":" + id, QMessageBox::Retry | QMessageBox::Cancel);
        msg.setWindowIcon(DIcon("compile-warning"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::Cancel)
            noMoreMsgbox = true;
        return 0;
    }
    if(noMoreMsgbox)
        return 0;
    QMessageBox msg(QMessageBox::Warning, tr("NOT FOUND IN CONFIG!"), category, QMessageBox::Retry | QMessageBox::Cancel);
    msg.setWindowIcon(DIcon("compile-warning"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    if(msg.exec() == QMessageBox::Cancel)
        noMoreMsgbox = true;
    return 0;
}

void Configuration::setUint(const QString & category, const QString & id, const duint i)
{
    if(Uints.contains(category))
    {
        if(Uints[category].contains(id))
        {
            Uints[category][id] = i;
            return;
        }
        if(noMoreMsgbox)
            return;
        QMessageBox msg(QMessageBox::Warning, tr("NOT FOUND IN CONFIG!"), category + ":" + id, QMessageBox::Retry | QMessageBox::Cancel);
        msg.setWindowIcon(DIcon("compile-warning"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::Cancel)
            noMoreMsgbox = true;
        return;
    }
    if(noMoreMsgbox)
        return;
    QMessageBox msg(QMessageBox::Warning, tr("NOT FOUND IN CONFIG!"), category, QMessageBox::Retry | QMessageBox::Cancel);
    msg.setWindowIcon(DIcon("compile-warning"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    if(msg.exec() == QMessageBox::Cancel)
        noMoreMsgbox = true;
}

const QFont Configuration::getFont(const QString & id) const
{
    if(Fonts.contains(id))
        return Fonts.constFind(id).value();
    QFont ret("Lucida Console", 8, QFont::Normal, false);
    ret.setFixedPitch(true);
    ret.setStyleHint(QFont::Monospace);
    if(noMoreMsgbox)
        return ret;
    QMessageBox msg(QMessageBox::Warning, tr("NOT FOUND IN CONFIG!"), id, QMessageBox::Retry | QMessageBox::Cancel);
    msg.setWindowIcon(DIcon("compile-warning"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    if(msg.exec() == QMessageBox::Cancel)
        noMoreMsgbox = true;
    return ret;
}

const Configuration::Shortcut Configuration::getShortcut(const QString & key_id) const
{
    if(Shortcuts.contains(key_id))
        return Shortcuts.constFind(key_id).value();
    if(!noMoreMsgbox)
    {
        QMessageBox msg(QMessageBox::Warning, tr("NOT FOUND IN CONFIG!"), key_id, QMessageBox::Retry | QMessageBox::Cancel);
        msg.setWindowIcon(DIcon("compile-warning"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::Cancel)
            noMoreMsgbox = true;
    }
    return Shortcut();
}

void Configuration::setShortcut(const QString & key_id, const QKeySequence key_sequence)
{
    if(Shortcuts.contains(key_id))
    {
        Shortcuts[key_id].Hotkey = key_sequence;
        return;
    }
    if(noMoreMsgbox)
        return;
    QMessageBox msg(QMessageBox::Warning, tr("NOT FOUND IN CONFIG!"), key_id, QMessageBox::Retry | QMessageBox::Cancel);
    msg.setWindowIcon(DIcon("compile-warning"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    if(msg.exec() == QMessageBox::Cancel)
        noMoreMsgbox = true;
}

void Configuration::setPluginShortcut(const QString & key_id, QString description, QString defaultShortcut, bool global)
{
    defaultShortcuts[key_id] = Shortcut(description, defaultShortcut, global);
    readShortcuts();
}

QColor Configuration::colorFromConfig(const QString & id)
{
    char setting[MAX_SETTING_SIZE] = "";
    if(!BridgeSettingGet("Colors", id.toUtf8().constData(), setting))
    {
        if(defaultColors.contains(id))
        {
            QColor ret = defaultColors.find(id).value();
            colorToConfig(id, ret);
            return ret;
        }
        return Qt::black; //black is default
    }
    if(QString(setting).toUpper() == "#XXXXXX") //support custom transparent color name
        return Qt::transparent;
    QColor color(setting);
    if(!color.isValid())
    {
        if(defaultColors.contains(id))
        {
            QColor ret = defaultColors.find(id).value();
            colorToConfig(id, ret);
            return ret;
        }
        return Qt::black; //black is default
    }
    return color;
}

bool Configuration::colorToConfig(const QString & id, const QColor color)
{
    QString colorName = color.name().toUpper();
    if(!color.alpha())
        colorName = "#XXXXXX";
    return BridgeSettingSet("Colors", id.toUtf8().constData(), colorName.toUtf8().constData());
}

bool Configuration::boolFromConfig(const QString & category, const QString & id)
{
    duint setting;
    if(!BridgeSettingGetUint(category.toUtf8().constData(), id.toUtf8().constData(), &setting))
    {
        if(defaultBools.contains(category) && defaultBools[category].contains(id))
        {
            bool ret = defaultBools[category][id];
            boolToConfig(category, id, ret);
            return ret;
        }
        return false; //DAFUG
    }
    return (setting != 0);
}

bool Configuration::boolToConfig(const QString & category, const QString & id, const bool bBool)
{
    return BridgeSettingSetUint(category.toUtf8().constData(), id.toUtf8().constData(), bBool);
}

duint Configuration::uintFromConfig(const QString & category, const QString & id)
{
    duint setting;
    if(!BridgeSettingGetUint(category.toUtf8().constData(), id.toUtf8().constData(), &setting))
    {
        if(defaultUints.contains(category) && defaultUints[category].contains(id))
        {
            setting = defaultUints[category][id];
            uintToConfig(category, id, setting);
            return setting;
        }
        return 0; //DAFUG
    }
    return setting;
}

bool Configuration::uintToConfig(const QString & category, const QString & id, duint i)
{
    return BridgeSettingSetUint(category.toUtf8().constData(), id.toUtf8().constData(), i);
}

QFont Configuration::fontFromConfig(const QString & id)
{
    char setting[MAX_SETTING_SIZE] = "";
    if(!BridgeSettingGet("Fonts", id.toUtf8().constData(), setting))
    {
        if(defaultFonts.contains(id))
        {
            QFont ret = defaultFonts.find(id).value();
            fontToConfig(id, ret);
            return ret;
        }
        if(id == "Application")
            return QApplication::font();
        QFont ret("Lucida Console", 8, QFont::Normal, false);
        ret.setFixedPitch(true);
        ret.setStyleHint(QFont::Monospace);
        return ret;
    }
    QFont font;
    if(!font.fromString(setting))
    {
        if(defaultFonts.contains(id))
        {
            QFont ret = defaultFonts.find(id).value();
            fontToConfig(id, ret);
            return ret;
        }
        if(id == "Application")
            return QApplication::font();
        QFont ret("Lucida Console", 8, QFont::Normal, false);
        ret.setFixedPitch(true);
        ret.setStyleHint(QFont::Monospace);
        return ret;
    }
    return font;
}

bool Configuration::fontToConfig(const QString & id, const QFont font)
{
    return BridgeSettingSet("Fonts", id.toUtf8().constData(), font.toString().toUtf8().constData());
}

QString Configuration::shortcutFromConfig(const QString & id)
{
    QString _id = QString("%1").arg(id);
    char setting[MAX_SETTING_SIZE] = "";
    if(BridgeSettingGet("Shortcuts", _id.toUtf8().constData(), setting))
    {
        return QString(setting);
    }
    return QString();
}

bool Configuration::shortcutToConfig(const QString & id, const QKeySequence shortcut)
{
    QString _id = QString("%1").arg(id);
    QString _key = "";
    if(!shortcut.isEmpty())
        _key = shortcut.toString(QKeySequence::NativeText);
    else
        _key = "NOT_SET";
    return BridgeSettingSet("Shortcuts", _id.toUtf8().constData(), _key.toUtf8().constData());
}

void Configuration::registerMenuBuilder(MenuBuilder* menu, size_t count)
{
    QString id = menu->getId();
    for(const auto & i : NamedMenuBuilders)
        if(i.type == 0 && i.builder->getId() == id)
            return; //already exists
    NamedMenuBuilders.append(MenuMap(menu, count));
}

void Configuration::registerMainMenuStringList(QList<QAction*>* menu)
{
    NamedMenuBuilders.append(MenuMap(menu, menu->size() - 1));
}

void Configuration::zoomFont(const QString & fontName, QWheelEvent* event)
{
    QPoint numDegrees = event->angleDelta() / 8;
    int ticks = numDegrees.y() / 15;
    QFont myFont = Fonts[fontName];
    char fontSizes[] = {6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 0}; // The list of font sizes in ApperanceDialog
    char* currentFontSize = strchr(fontSizes, myFont.pointSize() & 127);
    if(currentFontSize)
    {
        currentFontSize += ticks;
        if(currentFontSize > fontSizes + 11)
            currentFontSize = fontSizes + 11;
        else if(currentFontSize < fontSizes)
            currentFontSize = fontSizes;
        myFont.setPointSize(*currentFontSize);
        Fonts[fontName] = myFont;
        writeFonts();
        GuiUpdateAllViews();
    }
}

static bool IsPointVisible(QPoint pos)
{
    for(const auto & i : QGuiApplication::screens())
    {
        QRect rt = i->geometry();
        if(rt.left() <= pos.x() && rt.right() >= pos.x() && rt.top() <= pos.y() && rt.bottom() >= pos.y())
            return true;
    }
    return false;
}

/**
 * @brief Configuration::setupWindowPos Loads the position/size of a dialog.
 * @param window this
 */
void Configuration::loadWindowGeometry(QWidget* window)
{
    QString name = window->metaObject()->className();
    char setting[MAX_SETTING_SIZE] = "";
    if(!BridgeSettingGet("Gui", (name + "Geometry").toUtf8().constData(), setting))
        return;
    auto oldPos = window->pos();
    window->restoreGeometry(QByteArray::fromBase64(QByteArray(setting)));
    if(!IsPointVisible(window->pos()))
        window->move(oldPos);
}

/**
 * @brief Configuration::saveWindowPos Saves the position/size of a dialog.
 * @param window this
 */
void Configuration::saveWindowGeometry(QWidget* window)
{
    QString name = window->metaObject()->className();
    BridgeSettingSet("Gui", (name + "Geometry").toUtf8().constData(), window->saveGeometry().toBase64().data());
}
