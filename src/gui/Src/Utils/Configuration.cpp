#include "Configuration.h"
#include <QApplication>
#include <QFontInfo>
#include <QMessageBox>
#include <QIcon>

Configuration* Configuration::mPtr = NULL;

Configuration::Configuration() : QObject()
{
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
    defaultColors.insert("DisassemblyHardwareBreakpointBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblyBookmarkColor", QColor("#000000"));
    defaultColors.insert("DisassemblyBookmarkBackgroundColor", QColor("#FEE970"));
    defaultColors.insert("DisassemblyLabelColor", QColor("#FF0000"));
    defaultColors.insert("DisassemblyLabelBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblyBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("DisassemblySelectionColor", QColor("#C0C0C0"));
    defaultColors.insert("DisassemblyAddressColor", QColor("#808080"));
    defaultColors.insert("DisassemblyAddressBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblySelectedAddressColor", QColor("#000000"));
    defaultColors.insert("DisassemblySelectedAddressBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblyConditionalJumpLineTrueColor", QColor("#FF0000"));
    defaultColors.insert("DisassemblyConditionalJumpLineFalseColor", QColor("#808080"));
    defaultColors.insert("DisassemblyUnconditionalJumpLineColor", QColor("#FF0000"));
    defaultColors.insert("DisassemblyBytesColor", QColor("#000000"));
    defaultColors.insert("DisassemblyModifiedBytesColor", QColor("#FF0000"));
    defaultColors.insert("DisassemblyCommentColor", QColor("#000000"));
    defaultColors.insert("DisassemblyCommentBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblyAutoCommentColor", QColor("#808080"));
    defaultColors.insert("DisassemblyAutoCommentBackgroundColor", Qt::transparent);
    defaultColors.insert("DisassemblyFunctionColor", QColor("#000000"));
    defaultColors.insert("DisassemblyLoopColor", QColor("#000000"));

    defaultColors.insert("SideBarCipLabelColor", QColor("#FFFFFF"));
    defaultColors.insert("SideBarCipLabelBackgroundColor", QColor("#4040FF"));
    defaultColors.insert("SideBarBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("SideBarConditionalJumpLineTrueColor", QColor("#FF0000"));
    defaultColors.insert("SideBarConditionalJumpLineFalseColor", QColor("#808080"));
    defaultColors.insert("SideBarUnconditionalJumpLineTrueColor", QColor("#FF0000"));
    defaultColors.insert("SideBarUnconditionalJumpLineFalseColor", QColor("#808080"));
    defaultColors.insert("SideBarBulletColor", QColor("#808080"));
    defaultColors.insert("SideBarBulletBreakpointColor", QColor("#FF0000"));
    defaultColors.insert("SideBarBulletDisabledBreakpointColor", QColor("#00AA00"));
    defaultColors.insert("SideBarBulletBookmarkColor", QColor("#FEE970"));

    defaultColors.insert("RegistersBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("RegistersColor", QColor("#000000"));
    defaultColors.insert("RegistersModifiedColor", QColor("#FF0000"));
    defaultColors.insert("RegistersSelectionColor", QColor("#EEEEEE"));
    defaultColors.insert("RegistersLabelColor", QColor("#000000"));
    defaultColors.insert("RegistersExtraInfoColor", QColor("#000000"));

    defaultColors.insert("InstructionHighlightColor", QColor("#FF0000"));
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
    defaultColors.insert("HexDumpBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("HexDumpSelectionColor", QColor("#C0C0C0"));
    defaultColors.insert("HexDumpAddressColor", QColor("#000000"));
    defaultColors.insert("HexDumpAddressBackgroundColor", Qt::transparent);
    defaultColors.insert("HexDumpLabelColor", QColor("#FF0000"));
    defaultColors.insert("HexDumpLabelBackgroundColor", Qt::transparent);

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

    defaultColors.insert("HexEditTextColor", QColor("#000000"));
    defaultColors.insert("HexEditWildcardColor", QColor("#FF0000"));
    defaultColors.insert("HexEditBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("HexEditSelectionColor", QColor("#C0C0C0"));

    defaultColors.insert("ThreadCurrentColor", QColor("#FFFFFF"));
    defaultColors.insert("ThreadCurrentBackgroundColor", QColor("#000000"));
    defaultColors.insert("MemoryMapBreakpointColor", QColor("#FFFBF0"));
    defaultColors.insert("MemoryMapBreakpointBackgroundColor", QColor("#FF0000"));
    defaultColors.insert("MemoryMapSectionTextColor", QColor("#8B671F"));
    defaultColors.insert("SearchListViewHighlightColor", QColor("#FF0000"));

    //bool settings
    QMap<QString, bool> disassemblyBool;
    disassemblyBool.insert("ArgumentSpaces", false);
    disassemblyBool.insert("MemorySpaces", false);
    disassemblyBool.insert("FillNOPs", false);
    disassemblyBool.insert("Uppercase", false);
    disassemblyBool.insert("FindCommandEntireBlock", false);
    disassemblyBool.insert("OnlyCipAutoComments", false);
    disassemblyBool.insert("TabbedMnemonic", false);
    defaultBools.insert("Disassembler", disassemblyBool);

    QMap<QString, bool> engineBool;
    engineBool.insert("ListAllPages", false);
    defaultBools.insert("Engine", engineBool);

    //uint settings
    QMap<QString, duint> hexdumpUint;
    hexdumpUint.insert("DefaultView", 0);
    defaultUints.insert("HexDump", hexdumpUint);
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
    defaultFonts.insert("Log", QFont("Courier", 8, QFont::Normal, false));

    // hotkeys settings
    defaultShortcuts.insert("FileOpen", Shortcut(tr("File -> Open"), "F3", true));
    defaultShortcuts.insert("FileAttach", Shortcut(tr("File -> Attach"), "Alt+A", true));
    defaultShortcuts.insert("FileDetach", Shortcut(tr("File -> Detach"), "Ctrl+Alt+F2", true));
    defaultShortcuts.insert("FileExit", Shortcut(tr("File -> Exit"), "Alt+X", true));

    defaultShortcuts.insert("ViewCpu", Shortcut(tr("View -> CPU"), "Alt+C", true));
    defaultShortcuts.insert("ViewLog", Shortcut(tr("View -> Log"), "Alt+L", true));
    defaultShortcuts.insert("ViewBreakpoints", Shortcut(tr("View -> Breakpoints"), "Alt+B", true));
    defaultShortcuts.insert("ViewMemoryMap", Shortcut(tr("View -> Memory Map"), "Alt+M", true));
    defaultShortcuts.insert("ViewCallStack", Shortcut(tr("View -> Call Stack"), "Alt+K", true));
    defaultShortcuts.insert("ViewScript", Shortcut(tr("View -> Script"), "Alt+S", true));
    defaultShortcuts.insert("ViewSymbolInfo", Shortcut(tr("View -> Symbol Info"), "Ctrl+Alt+S", true));
    defaultShortcuts.insert("ViewSource", Shortcut(tr("View -> Source"), "Ctrl+Shift+S", true));
    defaultShortcuts.insert("ViewReferences", Shortcut(tr("View -> References"), "Alt+R", true));
    defaultShortcuts.insert("ViewThreads", Shortcut(tr("View -> Threads"), "Alt+T", true));
    defaultShortcuts.insert("ViewPatches", Shortcut(tr("View -> Patches"), "Ctrl+P", true));
    defaultShortcuts.insert("ViewComments", Shortcut(tr("View -> Comments"), "Ctrl+Alt+C", true));
    defaultShortcuts.insert("ViewLabels", Shortcut(tr("View -> Labels"), "Ctrl+Alt+L", true));
    defaultShortcuts.insert("ViewBookmarks", Shortcut(tr("View -> Bookmarks"), "Ctrl+Alt+B", true));
    defaultShortcuts.insert("ViewFunctions", Shortcut(tr("View -> Functions"), "Alt+F", true));

    defaultShortcuts.insert("DebugRun", Shortcut(tr("Debug -> Run"), "F9", true));
    defaultShortcuts.insert("DebugeRun", Shortcut(tr("Debug -> Run (skip exceptions)"), "Shift+F9", true));
    defaultShortcuts.insert("DebugRunSelection", Shortcut(tr("Debug -> Run until selection"), "F4", true));
    defaultShortcuts.insert("DebugPause", Shortcut(tr("Debug -> Pause"), "F12", true));
    defaultShortcuts.insert("DebugRestart", Shortcut(tr("Debug -> Restart"), "Ctrl+F2", true));
    defaultShortcuts.insert("DebugClose", Shortcut(tr("Debug -> Close"), "Alt+F2", true));
    defaultShortcuts.insert("DebugStepInto", Shortcut(tr("Debug -> Step into"), "F7", true));
    defaultShortcuts.insert("DebugeStepInfo", Shortcut(tr("Debug -> Step into (skip execptions)"), "Shift+F7", true));
    defaultShortcuts.insert("DebugStepOver", Shortcut(tr("Debug -> Step over"), "F8", true));
    defaultShortcuts.insert("DebugeStepOver", Shortcut(tr("Debug -> Step over (skip execptions)"), "Shift+F8", true));
    defaultShortcuts.insert("DebugRtr", Shortcut(tr("Debug -> Execute till return"), "Ctrl+F9", true));
    defaultShortcuts.insert("DebugeRtr", Shortcut(tr("Debug -> Execute till return (skip exceptions)"), "Ctrl+Shift+F9", true));
    defaultShortcuts.insert("DebugSkipNextInstruction", Shortcut(tr("Debug -> Skip next instruction"), "Ctrl+F8", true));
    defaultShortcuts.insert("DebugCommand", Shortcut(tr("Debug -> Command"), "Ctrl+Return", true));

    defaultShortcuts.insert("PluginsScylla", Shortcut(tr("Plugins -> Scylla"), "Ctrl+I", true));

    defaultShortcuts.insert("OptionsPreferences", Shortcut(tr("Options -> Preferences"), "", true));
    defaultShortcuts.insert("OptionsAppearance", Shortcut(tr("Options -> Appearance"), "", true));
    defaultShortcuts.insert("OptionsShortcuts", Shortcut(tr("Options -> Shortcuts"), "", true));
    defaultShortcuts.insert("OptionsTopmost", Shortcut(tr("Options -> Topmost"), "Ctrl+F5", true));

    defaultShortcuts.insert("HelpAbout", Shortcut(tr("Help -> About"), "", true));
    defaultShortcuts.insert("HelpDonate", Shortcut(tr("Help -> Donate"), "", true));
    defaultShortcuts.insert("HelpCheckForUpdates", Shortcut(tr("Help -> Check for Updates"), "", true));
    defaultShortcuts.insert("HelpCalculator", Shortcut(tr("Help -> Calculator"), "?"));
    defaultShortcuts.insert("HelpReportBug", Shortcut(tr("Help -> Report Bug"), "", true));
    defaultShortcuts.insert("HelpManual", Shortcut(tr("Help -> Manual"), "F1", true));

    defaultShortcuts.insert("ActionFindStrings", Shortcut(tr("Actions -> Find Strings"), "", true));
    defaultShortcuts.insert("ActionFindIntermodularCalls", Shortcut(tr("Actions -> Find Intermodular Calls"), "", true));
    defaultShortcuts.insert("ActionToggleBreakpoint", Shortcut(tr("Actions -> Toggle Breakpoint"), "F2"));
    defaultShortcuts.insert("ActionEditBreakpoint", Shortcut(tr("Actions -> Set Conditional Breakpoint"), "Shift+F2"));
    defaultShortcuts.insert("ActionToggleBookmark", Shortcut(tr("Actions -> Toggle Bookmark"), "Ctrl+D"));
    defaultShortcuts.insert("ActionDeleteBreakpoint", Shortcut(tr("Actions -> Delete Breakpoint"), "Delete"));
    defaultShortcuts.insert("ActionEnableDisableBreakpoint", Shortcut(tr("Actions -> Enable/Disable Breakpoint"), "Space"));

    defaultShortcuts.insert("ActionBinaryEdit", Shortcut(tr("Actions -> Binary Edit"), "Ctrl+E"));
    defaultShortcuts.insert("ActionBinaryFill", Shortcut(tr("Actions -> Binary Fill"), "F"));
    defaultShortcuts.insert("ActionBinaryFillNops", Shortcut(tr("Actions -> Binary Fill NOPs"), "Ctrl+9"));
    defaultShortcuts.insert("ActionBinaryCopy", Shortcut(tr("Actions -> Binary Copy"), "Shift+C"));
    defaultShortcuts.insert("ActionBinaryPaste", Shortcut(tr("Actions -> Binary Paste"), "Shift+V"));
    defaultShortcuts.insert("ActionBinaryPasteIgnoreSize", Shortcut(tr("Actions -> Binary Paste (Ignore Size)"), "Ctrl+Shift+V"));
    defaultShortcuts.insert("ActionUndoSelection", Shortcut(tr("Actions -> Undo Selection"), "Ctrl+Backspace"));
    defaultShortcuts.insert("ActionSetLabel", Shortcut(tr("Actions -> Set Label"), ":"));
    defaultShortcuts.insert("ActionSetComment", Shortcut(tr("Actions -> Set Comment"), ";"));
    defaultShortcuts.insert("ActionToggleFunction", Shortcut(tr("Actions -> Toggle Function"), "Shift+F"));
    defaultShortcuts.insert("ActionAssemble", Shortcut(tr("Actions -> Assemble"), "Space"));
    defaultShortcuts.insert("ActionYara", Shortcut(tr("Actions -> Yara"), "Ctrl+Y"));
    defaultShortcuts.insert("ActionSetNewOriginHere", Shortcut(tr("Actions -> Set New Origin Here"), "Ctrl+*"));
    defaultShortcuts.insert("ActionGotoOrigin", Shortcut(tr("Actions -> Goto Origin"), "*"));
    defaultShortcuts.insert("ActionGotoPrevious", Shortcut(tr("Actions -> Goto Previous"), "-"));
    defaultShortcuts.insert("ActionGotoNext", Shortcut(tr("Actions -> Goto Next"), "+"));
    defaultShortcuts.insert("ActionGotoExpression", Shortcut(tr("Actions -> Goto Expression"), "Ctrl+G"));
    defaultShortcuts.insert("ActionGotoStart", Shortcut(tr("Actions -> Goto Start of Page"), "Home"));
    defaultShortcuts.insert("ActionGotoEnd", Shortcut(tr("Actions -> Goto End of Page"), "End"));
    defaultShortcuts.insert("ActionGotoFileOffset", Shortcut(tr("Actions -> Goto File Offset"), "Ctrl+Shift+G"));
    defaultShortcuts.insert("ActionFindReferencesToSelectedAddress", Shortcut(tr("Actions -> Find References to Selected Address"), "Ctrl+R"));
    defaultShortcuts.insert("ActionFindPattern", Shortcut(tr("Actions -> Find Pattern"), "Ctrl+B"));
    defaultShortcuts.insert("ActionFindReferences", Shortcut(tr("Actions -> Find References"), "Ctrl+R"));
    defaultShortcuts.insert("ActionHighlightingMode", Shortcut(tr("Actions -> Highlighting Mode"), "Ctrl+H"));
    defaultShortcuts.insert("ActionFind", Shortcut(tr("Actions -> Find"), "Ctrl+F"));
    defaultShortcuts.insert("ActionDecompileFunction", Shortcut(tr("Actions -> Decompile Function"), "F5"));
    defaultShortcuts.insert("ActionDecompileSelection", Shortcut(tr("Actions -> Decompile Selection"), "Shift+F5"));

    defaultShortcuts.insert("ActionIncreaseRegister", Shortcut(tr("Actions -> Increase Register"), "+"));
    defaultShortcuts.insert("ActionDecreaseRegister", Shortcut(tr("Actions -> Decrease Register"), "-"));
    defaultShortcuts.insert("ActionZeroRegister", Shortcut(tr("Actions -> Zero Register"), "0"));
    defaultShortcuts.insert("ActionSetOneRegister", Shortcut(tr("Actions -> Set Register to One"), "1"));
    defaultShortcuts.insert("ActionToggleRegisterValue", Shortcut(tr("Actions -> Toggle Register Value"), "Space"));
    defaultShortcuts.insert("ActionCopy", Shortcut(tr("Actions -> Copy"), "Ctrl+C"));
    defaultShortcuts.insert("ActionCopyAddress", Shortcut(tr("Actions -> Copy Address"), "Alt+INS"));
    defaultShortcuts.insert("ActionCopySymbol", Shortcut(tr("Actions -> Copy Symbol"), "Ctrl+S"));
    defaultShortcuts.insert("ActionLoadScript", Shortcut(tr("Actions -> Load Script"), "Ctrl+O"));
    defaultShortcuts.insert("ActionReloadScript", Shortcut(tr("Actions -> Reload Script"), "Ctrl+R"));
    defaultShortcuts.insert("ActionUnloadScript", Shortcut(tr("Actions -> Unload Script"), "Ctrl+U"));
    defaultShortcuts.insert("ActionRunScript", Shortcut(tr("Actions -> Run Script"), "Space"));
    defaultShortcuts.insert("ActionToggleBreakpointScript", Shortcut(tr("Actions -> Toggle Script Breakpoint"), "F2"));
    defaultShortcuts.insert("ActionRunToCursorScript", Shortcut(tr("Actions -> Run Script to Cursor"), "Shift+F4"));
    defaultShortcuts.insert("ActionStepScript", Shortcut(tr("Actions -> Step Script"), "Tab"));
    defaultShortcuts.insert("ActionAbortScript", Shortcut(tr("Actions -> Abort Script"), "Esc"));
    defaultShortcuts.insert("ActionExecuteCommandScript", Shortcut(tr("Actions -> Execute Script Command"), "X"));

    Shortcuts = defaultShortcuts;

    load();
    mPtr = this;
}

Configuration* Config()
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
    for(int i = 0; i < Colors.size(); i++)
    {
        QString id = Colors.keys().at(i);
        Colors[id] = colorFromConfig(id);
    }
}

void Configuration::writeColors()
{
    //write config
    for(int i = 0; i < Colors.size(); i++)
    {
        QString id = Colors.keys().at(i);
        colorToConfig(id, Colors[id]);
    }
    emit colorsUpdated();
}

void Configuration::emitColorsUpdated()
{
    emit colorsUpdated();
}

void Configuration::readBools()
{
    Bools = defaultBools;
    //read config
    for(int i = 0; i < Bools.size(); i++)
    {
        QString category = Bools.keys().at(i);
        QMap<QString, bool>* currentBool = &Bools[category];
        for(int j = 0; j < currentBool->size(); j++)
        {
            QString id = (*currentBool).keys().at(j);
            (*currentBool)[id] = boolFromConfig(category, id);
        }
    }
}

void Configuration::writeBools()
{
    //write config
    for(int i = 0; i < Bools.size(); i++)
    {
        QString category = Bools.keys().at(i);
        QMap<QString, bool>* currentBool = &Bools[category];
        for(int j = 0; j < currentBool->size(); j++)
        {
            QString id = (*currentBool).keys().at(j);
            boolToConfig(category, id, (*currentBool)[id]);
        }
    }
}

void Configuration::readUints()
{
    Uints = defaultUints;
    //read config
    for(int i = 0; i < Uints.size(); i++)
    {
        QString category = Uints.keys().at(i);
        QMap<QString, duint>* currentUint = &Uints[category];
        for(int j = 0; j < currentUint->size(); j++)
        {
            QString id = (*currentUint).keys().at(j);
            (*currentUint)[id] = uintFromConfig(category, id);
        }
    }
}

void Configuration::writeUints()
{
    //write config
    for(int i = 0; i < Uints.size(); i++)
    {
        QString category = Uints.keys().at(i);
        QMap<QString, duint>* currentUint = &Uints[category];
        for(int j = 0; j < currentUint->size(); j++)
        {
            QString id = (*currentUint).keys().at(j);
            uintToConfig(category, id, (*currentUint)[id]);
        }
    }
}

void Configuration::readFonts()
{
    Fonts = defaultFonts;
    //read config
    for(int i = 0; i < Fonts.size(); i++)
    {
        QString id = Fonts.keys().at(i);
        QFont font = fontFromConfig(id);
        QFontInfo fontInfo(font);
        if(id == "Application" || fontInfo.fixedPitch())
            Fonts[id] = font;
    }
}

void Configuration::writeFonts()
{
    //write config
    for(int i = 0; i < Fonts.size(); i++)
    {
        QString id = Fonts.keys().at(i);
        fontToConfig(id, Fonts[id]);
    }
    emit fontsUpdated();
}

void Configuration::emitFontsUpdated()
{
    emit fontsUpdated();
}

void Configuration::readShortcuts()
{
    Shortcuts = defaultShortcuts;
    QMap<QString, Shortcut>::const_iterator it = Shortcuts.begin();

    while(it != Shortcuts.end())
    {
        const QString id = it.key();
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

const QColor Configuration::getColor(const QString id) const
{
    if(Colors.contains(id))
        return Colors.constFind(id).value();
    QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", id);
    msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
    return Qt::black;
}

const bool Configuration::getBool(const QString category, const QString id) const
{
    if(Bools.contains(category))
    {
        if(Bools[category].contains(id))
            return Bools[category][id];
        QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category + ":" + id);
        msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return false;
    }
    QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category);
    msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
    return false;
}

void Configuration::setBool(const QString category, const QString id, const bool b)
{
    if(Bools.contains(category))
    {
        if(Bools[category].contains(id))
        {
            Bools[category][id] = b;
            return;
        }
        QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category + ":" + id);
        msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category);
    msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

const duint Configuration::getUint(const QString category, const QString id) const
{
    if(Uints.contains(category))
    {
        if(Uints[category].contains(id))
            return Uints[category][id];
        QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category + ":" + id);
        msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return 0;
    }
    QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category);
    msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
    return 0;
}

void Configuration::setUint(const QString category, const QString id, const duint i)
{
    if(Uints.contains(category))
    {
        if(Uints[category].contains(id))
        {
            Uints[category][id] = i;
            return;
        }
        QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category + ":" + id);
        msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category);
    msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

const QFont Configuration::getFont(const QString id) const
{
    if(Fonts.contains(id))
        return Fonts.constFind(id).value();
    QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", id);
    msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
    QFont ret("Lucida Console", 8, QFont::Normal, false);
    ret.setFixedPitch(true);
    ret.setStyleHint(QFont::Monospace);
    return ret;
}

const Configuration::Shortcut Configuration::getShortcut(const QString key_id) const
{
    if(Shortcuts.contains(key_id))
        return Shortcuts.constFind(key_id).value();
    QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", key_id);
    msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
    return Shortcut();
}

void Configuration::setShortcut(const QString key_id, const QKeySequence key_sequence)
{
    if(Shortcuts.contains(key_id))
    {
        Shortcuts[key_id].Hotkey = key_sequence;
        return;
    }
    QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", key_id);
    msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

QColor Configuration::colorFromConfig(const QString id)
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

bool Configuration::colorToConfig(const QString id, const QColor color)
{
    QString colorName = color.name().toUpper();
    if(!color.alpha())
        colorName = "#XXXXXX";
    return BridgeSettingSet("Colors", id.toUtf8().constData(), colorName.toUtf8().constData());
}

bool Configuration::boolFromConfig(const QString category, const QString id)
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

bool Configuration::boolToConfig(const QString category, const QString id, const bool bBool)
{
    return BridgeSettingSetUint(category.toUtf8().constData(), id.toUtf8().constData(), bBool);
}

duint Configuration::uintFromConfig(const QString category, const QString id)
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

bool Configuration::uintToConfig(const QString category, const QString id, duint i)
{
    return BridgeSettingSetUint(category.toUtf8().constData(), id.toUtf8().constData(), i);
}

QFont Configuration::fontFromConfig(const QString id)
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

bool Configuration::fontToConfig(const QString id, const QFont font)
{
    return BridgeSettingSet("Fonts", id.toUtf8().constData(), font.toString().toUtf8().constData());
}

QString Configuration::shortcutFromConfig(const QString id)
{
    QString _id = QString("%1").arg(id);
    char setting[MAX_SETTING_SIZE] = "";
    if(BridgeSettingGet("Shortcuts", _id.toUtf8().constData(), setting))
    {
        return QString(setting);
    }
    return "";
}

bool Configuration::shortcutToConfig(const QString id, const QKeySequence shortcut)
{
    QString _id = QString("%1").arg(id);
    QString _key = "";
    if(!shortcut.isEmpty())
        _key = shortcut.toString(QKeySequence::NativeText);
    else
        _key = "NOT_SET";
    return BridgeSettingSet("Shortcuts", _id.toUtf8().constData(), _key.toUtf8().constData());
}
