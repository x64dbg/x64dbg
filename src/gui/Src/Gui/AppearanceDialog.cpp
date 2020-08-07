#include "AppearanceDialog.h"
#include "ui_AppearanceDialog.h"
#include <QColorDialog>
#include <QFontDialog>
#include <QMessageBox>
#include <memory>
#include "Configuration.h"
#include "StringUtil.h"
#include "MiscUtil.h"

AppearanceDialog::AppearanceDialog(QWidget* parent) : QDialog(parent), ui(new Ui::AppearanceDialog)
{
    ui->setupUi(this);
    //set window flags
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    //Colors
    colorMap = &Config()->Colors;
    colorBackupMap = *colorMap;
    ui->groupColor->setEnabled(false);
    ui->groupBackgroundColor->setEnabled(false);
    colorInfoListInit();
    //Fonts
    fontMap = &Config()->Fonts;
    fontBackupMap = *fontMap;
    fontInit();
    connect(this, SIGNAL(rejected()), this, SLOT(rejectedSlot()));
}

AppearanceDialog::~AppearanceDialog()
{
    delete ui;
}

//Colors
void AppearanceDialog::on_button000000_clicked()
{
    ui->editColor->setText("#000000");
}

void AppearanceDialog::on_button000080_clicked()
{
    ui->editColor->setText("#000080");
}

void AppearanceDialog::on_button008000_clicked()
{
    ui->editColor->setText("#008000");
}

void AppearanceDialog::on_button008080_clicked()
{
    ui->editColor->setText("#008080");
}

void AppearanceDialog::on_button800000_clicked()
{
    ui->editColor->setText("#800000");
}

void AppearanceDialog::on_button800080_clicked()
{
    ui->editColor->setText("#800080");
}

void AppearanceDialog::on_button808000_clicked()
{
    ui->editColor->setText("#808000");
}

void AppearanceDialog::on_buttonC0C0C0_clicked()
{
    ui->editColor->setText("#C0C0C0");
}

void AppearanceDialog::on_button808080_clicked()
{
    ui->editColor->setText("#808080");
}

void AppearanceDialog::on_button0000FF_clicked()
{
    ui->editColor->setText("#0000FF");
}

void AppearanceDialog::on_button00FF00_clicked()
{
    ui->editColor->setText("#00FF00");
}

void AppearanceDialog::on_button00FFFF_clicked()
{
    ui->editColor->setText("#00FFFF");
}

void AppearanceDialog::on_buttonFF0000_clicked()
{
    ui->editColor->setText("#FF0000");
}

void AppearanceDialog::on_buttonFF00FF_clicked()
{
    ui->editColor->setText("#FF00FF");
}

void AppearanceDialog::on_buttonFFFF00_clicked()
{
    ui->editColor->setText("#FFFF00");
}

void AppearanceDialog::on_buttonFFFFFF_clicked()
{
    ui->editColor->setText("#FFFBF0");
}

void AppearanceDialog::on_buttonBackground000000_clicked()
{
    ui->editBackgroundColor->setText("#000000");
}

void AppearanceDialog::on_buttonBackgroundC0C0C0_clicked()
{
    ui->editBackgroundColor->setText("#C0C0C0");
}

void AppearanceDialog::on_buttonBackgroundFFFFFF_clicked()
{
    ui->editBackgroundColor->setText("#FFFBF0");
}

void AppearanceDialog::on_buttonBackground00FFFF_clicked()
{
    ui->editBackgroundColor->setText("#00FFFF");
}

void AppearanceDialog::on_buttonBackground00FF00_clicked()
{
    ui->editBackgroundColor->setText("#00FF00");
}

void AppearanceDialog::on_buttonBackgroundFF0000_clicked()
{
    ui->editBackgroundColor->setText("#FF0000");
}

void AppearanceDialog::on_buttonBackgroundFFFF00_clicked()
{
    ui->editBackgroundColor->setText("#FFFF00");
}

void AppearanceDialog::on_buttonBackgroundNone_clicked()
{
    ui->editBackgroundColor->setText("#XXXXXX");
}

void AppearanceDialog::on_editBackgroundColor_textChanged(const QString & arg1)
{
    QString text = arg1;
    if(!arg1.length())
    {
        ui->editBackgroundColor->setText("#");
        text = ui->editBackgroundColor->text();
        return;
    }
    if(arg1.at(0) != '#')
    {
        ui->editBackgroundColor->setText("#" + arg1);
        text = ui->editBackgroundColor->text();
    }
    QString styleSheet;
    QString id = colorInfoList.at(colorInfoIndex).backgroundColorName;
    if(text == "#XXXXXX")
    {
        styleSheet = "border: 2px solid black; background-color: #C0C0C0";
        ui->buttonBackgroundColor->setText("X");
        if(colorMap->contains(id))
        {
            (*colorMap)[id] = Qt::transparent;
            ui->buttonSave->setEnabled(true);
            emit Config()->colorsUpdated();
            GuiUpdateAllViews();
        }
    }
    else
    {
        ui->buttonBackgroundColor->setText("");
        if(QColor(text).isValid())
        {
            styleSheet = "border: 2px solid black; background-color: " + text;
            if(colorMap->contains(id))
            {
                (*colorMap)[id] = QColor(text);
                ui->buttonSave->setEnabled(true);
                emit Config()->colorsUpdated();
                GuiUpdateAllViews();
            }
        }
        else
        {
            styleSheet = "border: 2px solid red; background-color: #FFFFFF";
            if(colorMap->contains(id))
                ui->buttonSave->setEnabled(false); //we cannot save with an invalid color
        }
    }
    ui->buttonBackgroundColor->setStyleSheet(styleSheet);
}

void AppearanceDialog::on_editColor_textChanged(const QString & arg1)
{
    QString text = arg1;
    if(!arg1.length())
    {
        ui->editColor->setText("#");
        text = ui->editColor->text();
        return;
    }
    if(arg1.at(0) != '#')
    {
        ui->editColor->setText("#" + arg1);
        text = ui->editColor->text();
    }

    QString id = colorInfoList.at(colorInfoIndex).colorName;
    QString styleSheet;
    if(QColor(text).isValid())
    {
        styleSheet = "border: 2px solid black; background-color: " + text;
        if(colorMap->contains(id))
        {
            (*colorMap)[id] = QColor(text);
            ui->buttonSave->setEnabled(true);
            emit Config()->colorsUpdated();
            GuiUpdateAllViews();
        }
    }
    else
    {
        styleSheet = "border: 2px solid red; background-color: #FFFFFF";
        if(colorMap->contains(id))
            ui->buttonSave->setEnabled(false); //we cannot save with an invalid color
    }
    ui->buttonColor->setStyleSheet(styleSheet);
}

void AppearanceDialog::on_buttonColor_clicked()
{
    selectColor(ui->editColor);
}

void AppearanceDialog::on_buttonBackgroundColor_clicked()
{
    selectColor(ui->editBackgroundColor);
}

void AppearanceDialog::on_listColorNames_itemSelectionChanged()
{
    colorInfoIndex = ui->listColorNames->row(ui->listColorNames->selectedItems().at(0));
    ColorInfo info = colorInfoList.at(colorInfoIndex);
    defaultValueAction->setEnabled(false);
    currentSettingAction->setEnabled(false);
    ui->buttonSave->setEnabled(false);
    ui->groupColor->setEnabled(false);
    ui->groupBackgroundColor->setEnabled(false);

    if(info.colorName.length())
    {
        QString id = info.colorName;
        if(colorMap->contains(id))
        {
            ui->groupColor->setEnabled(true);
            ui->buttonSave->setEnabled(true);
            defaultValueAction->setEnabled(true);
            currentSettingAction->setEnabled(true);

            QColor color = (*colorMap)[id];
            QString colorText = color.name().toUpper();
            if(!color.alpha())
                colorText = "#XXXXXX";
            ui->editColor->setText(colorText);
        }
        else
            ui->editColor->setText("#FFFFFF");
    }
    else
        ui->editColor->setText("#FFFFFF");

    if(info.backgroundColorName.length())
    {
        QString id = info.backgroundColorName;
        if(colorMap->contains(id))
        {
            ui->groupBackgroundColor->setEnabled(true);
            ui->buttonSave->setEnabled(true);
            defaultValueAction->setEnabled(true);
            currentSettingAction->setEnabled(true);

            QColor color = (*colorMap)[id];
            QString colorText = color.name().toUpper();
            if(!color.alpha())
                colorText = "#XXXXXX";
            ui->editBackgroundColor->setText(colorText);
        }
        else
            ui->editBackgroundColor->setText("#FFFFFF");
    }
    else
        ui->editBackgroundColor->setText("#FFFFFF");
}

void AppearanceDialog::on_buttonSave_clicked()
{
    Config()->writeColors();
    Config()->writeFonts();
    GuiUpdateAllViews();
    BridgeSettingFlush();
    GuiAddStatusBarMessage(tr("Settings saved!\n").toUtf8().constData());
}

void AppearanceDialog::defaultValueSlot()
{
    ColorInfo info = colorInfoList.at(colorInfoIndex);
    if(info.colorName.length())
    {
        QString id = info.colorName;
        if(Config()->defaultColors.contains(id))
        {
            QColor color = Config()->defaultColors[id];
            QString colorText = color.name().toUpper();
            if(!color.alpha())
                colorText = "#XXXXXX";
            ui->editColor->setText(colorText);
        }
    }
    if(info.backgroundColorName.length())
    {
        QString id = info.backgroundColorName;
        if(Config()->defaultColors.contains(id))
        {
            QColor color = Config()->defaultColors[id];
            QString colorText = color.name().toUpper();
            if(!color.alpha())
                colorText = "#XXXXXX";
            ui->editBackgroundColor->setText(colorText);
        }
    }
}

void AppearanceDialog::currentSettingSlot()
{
    ColorInfo info = colorInfoList.at(colorInfoIndex);
    if(info.colorName.length())
    {
        QString id = info.colorName;
        if(colorBackupMap.contains(id))
        {
            QColor color = colorBackupMap[id];
            QString colorText = color.name().toUpper();
            if(!color.alpha())
                colorText = "#XXXXXX";
            ui->editColor->setText(colorText);
        }
    }
    if(info.backgroundColorName.length())
    {
        QString id = info.backgroundColorName;
        if(colorBackupMap.contains(id))
        {
            QColor color = colorBackupMap[id];
            QString colorText = color.name().toUpper();
            if(!color.alpha())
                colorText = "#XXXXXX";
            ui->editBackgroundColor->setText(colorText);
        }
    }
}

void AppearanceDialog::colorInfoListAppend(QString propertyName, QString colorName, QString backgroundColorName)
{
    ColorInfo info;
    if(colorName.length() || backgroundColorName.length())
        propertyName = "     " + propertyName;
    else
        propertyName = QString(QChar(0x2022)) + " " + propertyName; //bullet + space
    info.propertyName = propertyName;
    info.colorName = colorName;
    info.backgroundColorName = backgroundColorName;
    colorInfoList.append(info);
    ui->listColorNames->addItem(colorInfoList.last().propertyName);
}

void AppearanceDialog::colorInfoListInit()
{
    //clear list
    colorInfoIndex = 0;
    colorInfoList.clear();
    //list entries
    colorInfoListAppend(tr("General Tables:"), "", "");
    colorInfoListAppend(tr("Text"), "AbstractTableViewTextColor", "");
    colorInfoListAppend(tr("Header Text"), "AbstractTableViewHeaderTextColor", "");
    colorInfoListAppend(tr("Background"), "AbstractTableViewBackgroundColor", "");
    colorInfoListAppend(tr("Selection"), "AbstractTableViewSelectionColor", "");
    colorInfoListAppend(tr("Separators"), "AbstractTableViewSeparatorColor", "");

    colorInfoListAppend(tr("Disassembly:"), "", "");
    colorInfoListAppend(tr("Background"), "DisassemblyBackgroundColor", "");
    colorInfoListAppend(tr("Selection"), "DisassemblySelectionColor", "");
    colorInfoListAppend(tr("Bytes"), "DisassemblyBytesColor", "DisassemblyBytesBackgroundColor");
    colorInfoListAppend(tr("Modified Bytes"), "DisassemblyModifiedBytesColor", "DisassemblyModifiedBytesBackgroundColor");
    colorInfoListAppend(tr("Restored Bytes"), "DisassemblyRestoredBytesColor", "DisassemblyRestoredBytesBackgroundColor");
    colorInfoListAppend(tr("Relocation underline"), "DisassemblyRelocationUnderlineColor", "");
    colorInfoListAppend(ArchValue(tr("EIP"), tr("RIP")), "DisassemblyCipColor", "DisassemblyCipBackgroundColor");
    colorInfoListAppend(tr("Breakpoints"), "DisassemblyBreakpointColor", "DisassemblyBreakpointBackgroundColor");
    colorInfoListAppend(tr("Hardware Breakpoints"), "DisassemblyHardwareBreakpointColor", "DisassemblyHardwareBreakpointBackgroundColor");
    colorInfoListAppend(tr("Bookmarks"), "DisassemblyBookmarkColor", "DisassemblyBookmarkBackgroundColor");
    colorInfoListAppend(tr("Comments"), "DisassemblyCommentColor", "DisassemblyCommentBackgroundColor");
    colorInfoListAppend(tr("Automatic Comments"), "DisassemblyAutoCommentColor", "DisassemblyAutoCommentBackgroundColor");
    colorInfoListAppend(tr("Mnemonic Brief Comments"), "DisassemblyMnemonicBriefColor", "DisassemblyMnemonicBriefBackgroundColor");
    colorInfoListAppend(tr("Labels"), "DisassemblyLabelColor", "DisassemblyLabelBackgroundColor");
    colorInfoListAppend(tr("Addresses"), "DisassemblyAddressColor", "DisassemblyAddressBackgroundColor");
    colorInfoListAppend(tr("Selected Addresses"), "DisassemblySelectedAddressColor", "DisassemblySelectedAddressBackgroundColor");
    colorInfoListAppend(tr("Conditional Jump Lines (jump)"), "DisassemblyConditionalJumpLineTrueColor", "");
    colorInfoListAppend(tr("Conditional Jump Lines (no jump)"), "DisassemblyConditionalJumpLineFalseColor", "");
    colorInfoListAppend(tr("Unconditional Jump Lines"), "DisassemblyUnconditionalJumpLineColor", "");
    colorInfoListAppend(tr("Traced line"), "DisassemblyTracedBackgroundColor", "");
    colorInfoListAppend(tr("Function Lines"), "DisassemblyFunctionColor", "");
    colorInfoListAppend(tr("Loop Lines"), "DisassemblyLoopColor", "");

    colorInfoListAppend(tr("SideBar:"), "", "");
    colorInfoListAppend(tr("Register Labels"), "SideBarCipLabelColor", "SideBarCipLabelBackgroundColor");
    colorInfoListAppend(tr("Bullets"), "SideBarBulletColor", "");
    colorInfoListAppend(tr("Breakpoint bullets"), "SideBarBulletBreakpointColor", "");
    colorInfoListAppend(tr("Disabled Breakpoint bullets"), "SideBarBulletDisabledBreakpointColor", "");
    colorInfoListAppend(tr("Bookmark bullets"), "SideBarBulletBookmarkColor", "");
    colorInfoListAppend(tr("Conditional Jump Lines (jump)"), "SideBarConditionalJumpLineTrueColor", "");
    colorInfoListAppend(tr("Conditional Jump Lines (no jump)"), "SideBarConditionalJumpLineFalseColor", "");
    colorInfoListAppend(tr("Unconditional Jump Lines (jump)"), "SideBarUnconditionalJumpLineTrueColor", "");
    colorInfoListAppend(tr("Unconditional Jump Lines (no jump)"), "SideBarUnconditionalJumpLineFalseColor", "");
    colorInfoListAppend(tr("Conditional Jump Backwards Lines (jump)"), "SideBarConditionalJumpLineTrueBackwardsColor", "");
    colorInfoListAppend(tr("Conditional Jump Backwards Lines (no jump)"), "SideBarConditionalJumpLineFalseBackwardsColor", "");
    colorInfoListAppend(tr("Unconditional Jump Backwards Lines (jump)"), "SideBarUnconditionalJumpLineTrueBackwardsColor", "");
    colorInfoListAppend(tr("Unconditional Jump Backwards Lines (no jump)"), "SideBarUnconditionalJumpLineFalseBackwardsColor", "");
    colorInfoListAppend(tr("Jump Lines (executing)"), "SideBarJumpLineExecuteColor", "");
    colorInfoListAppend(tr("Code Folding Checkbox Color"), "SideBarCheckBoxForeColor", "SideBarCheckBoxBackColor");
    colorInfoListAppend(tr("Background"), "SideBarBackgroundColor", "");

    colorInfoListAppend(tr("Registers:"), "", "");
    colorInfoListAppend(tr("Text"), "RegistersColor", "");
    colorInfoListAppend(tr("Background"), "RegistersBackgroundColor", "");
    colorInfoListAppend(tr("Selection"), "RegistersSelectionColor", "");
    colorInfoListAppend(tr("Modified Registers"), "RegistersModifiedColor", "");
    colorInfoListAppend(tr("Register Names"), "RegistersLabelColor", "");
    colorInfoListAppend(tr("Argument Register Names"), "RegistersArgumentLabelColor", "");
    colorInfoListAppend(tr("Extra Information"), "RegistersExtraInfoColor", "");
    colorInfoListAppend(tr("Highlight Read"), "RegistersHighlightReadColor", "");
    colorInfoListAppend(tr("Highlight Write"), "RegistersHighlightWriteColor", "");
    colorInfoListAppend(tr("Highlight Read+Write"), "RegistersHighlightReadWriteColor", "");

    colorInfoListAppend(tr("Instructions:"), "", "");
    colorInfoListAppend(tr("Text"), "InstructionUncategorizedColor", "InstructionUncategorizedBackgroundColor");
    colorInfoListAppend(tr("Highlighting"), "InstructionHighlightColor", "InstructionHighlightBackgroundColor");
    colorInfoListAppend(tr("Commas"), "InstructionCommaColor", "InstructionCommaBackgroundColor");
    colorInfoListAppend(tr("Prefixes"), "InstructionPrefixColor", "InstructionPrefixBackgroundColor");
    colorInfoListAppend(tr("Addresses"), "InstructionAddressColor", "InstructionAddressBackgroundColor");
    colorInfoListAppend(tr("Values"), "InstructionValueColor", "InstructionValueBackgroundColor");
    colorInfoListAppend(tr("Mnemonics"), "InstructionMnemonicColor", "InstructionMnemonicBackgroundColor");
    colorInfoListAppend(tr("Push/Pops"), "InstructionPushPopColor", "InstructionPushPopBackgroundColor");
    colorInfoListAppend(tr("Calls"), "InstructionCallColor", "InstructionCallBackgroundColor");
    colorInfoListAppend(tr("Returns"), "InstructionRetColor", "InstructionRetBackgroundColor");
    colorInfoListAppend(tr("Conditional Jumps"), "InstructionConditionalJumpColor", "InstructionConditionalJumpBackgroundColor");
    colorInfoListAppend(tr("Unconditional Jumps"), "InstructionUnconditionalJumpColor", "InstructionUnconditionalJumpBackgroundColor");
    colorInfoListAppend(tr("NOPs"), "InstructionNopColor", "InstructionNopBackgroundColor");
    colorInfoListAppend(tr("FAR"), "InstructionFarColor", "InstructionFarBackgroundColor");
    colorInfoListAppend(tr("INT3s"), "InstructionInt3Color", "InstructionInt3BackgroundColor");
    colorInfoListAppend(tr("Unusual Instructions"), "InstructionUnusualColor", "InstructionUnusualBackgroundColor");
    colorInfoListAppend(tr("General Registers"), "InstructionGeneralRegisterColor", "InstructionGeneralRegisterBackgroundColor");
    colorInfoListAppend(tr("FPU Registers"), "InstructionFpuRegisterColor", "InstructionFpuRegisterBackgroundColor");
    colorInfoListAppend(tr("MMX Registers"), "InstructionMmxRegisterColor", "InstructionMmxRegisterBackgroundColor");
    colorInfoListAppend(tr("XMM Registers"), "InstructionXmmRegisterColor", "InstructionXmmRegisterBackgroundColor");
    colorInfoListAppend(tr("YMM Registers"), "InstructionYmmRegisterColor", "InstructionYmmRegisterBackgroundColor");
    colorInfoListAppend(tr("ZMM Registers"), "InstructionZmmRegisterColor", "InstructionZmmRegisterBackgroundColor");
    colorInfoListAppend(tr("Memory Sizes"), "InstructionMemorySizeColor", "InstructionMemorySizeBackgroundColor");
    colorInfoListAppend(tr("Memory Segments"), "InstructionMemorySegmentColor", "InstructionMemorySegmentBackgroundColor");
    colorInfoListAppend(tr("Memory Brackets"), "InstructionMemoryBracketsColor", "InstructionMemoryBracketsBackgroundColor");
    colorInfoListAppend(tr("Memory Stack Brackets"), "InstructionMemoryStackBracketsColor", "InstructionMemoryStackBracketsBackgroundColor");
    colorInfoListAppend(tr("Memory Base Registers"), "InstructionMemoryBaseRegisterColor", "InstructionMemoryBaseRegisterBackgroundColor");
    colorInfoListAppend(tr("Memory Index Registers"), "InstructionMemoryIndexRegisterColor", "InstructionMemoryIndexRegisterBackgroundColor");
    colorInfoListAppend(tr("Memory Scales"), "InstructionMemoryScaleColor", "InstructionMemoryScaleBackgroundColor");
    colorInfoListAppend(tr("Memory Operators (+/-/*)"), "InstructionMemoryOperatorColor", "InstructionMemoryOperatorBackgroundColor");

    colorInfoListAppend(tr("HexDump:"), "", "");
    colorInfoListAppend(tr("Text"), "HexDumpTextColor", "");
    colorInfoListAppend(tr("Modified Bytes"), "HexDumpModifiedBytesColor", "HexDumpModifiedBytesBackgroundColor");
    colorInfoListAppend(tr("Restored Bytes"), "HexDumpRestoredBytesColor", "HexDumpRestoredBytesBackgroundColor");
    colorInfoListAppend(tr("0x00 Bytes"), "HexDumpByte00Color", "HexDumpByte00BackgroundColor");
    colorInfoListAppend(tr("0x7F Bytes"), "HexDumpByte7FColor", "HexDumpByte7FBackgroundColor");
    colorInfoListAppend(tr("0xFF Bytes"), "HexDumpByteFFColor", "HexDumpByteFFBackgroundColor");
    colorInfoListAppend(tr("IsPrint Bytes"), "HexDumpByteIsPrintColor", "HexDumpByteIsPrintBackgroundColor");
    colorInfoListAppend(tr("Background"), "HexDumpBackgroundColor", "");
    colorInfoListAppend(tr("Selection"), "HexDumpSelectionColor", "");
    colorInfoListAppend(tr("Addresses"), "HexDumpAddressColor", "HexDumpAddressBackgroundColor");
    colorInfoListAppend(tr("Labels"), "HexDumpLabelColor", "HexDumpLabelBackgroundColor");
    colorInfoListAppend(tr("User Code Pointer Highlight Color"), "HexDumpUserModuleCodePointerHighlightColor", "");
    colorInfoListAppend(tr("User Data Pointer Highlight Color"), "HexDumpUserModuleDataPointerHighlightColor", "");
    colorInfoListAppend(tr("System Code Pointer Highlight Color"), "HexDumpSystemModuleCodePointerHighlightColor", "");
    colorInfoListAppend(tr("System Data Pointer Highlight Color"), "HexDumpSystemModuleDataPointerHighlightColor", "");
    colorInfoListAppend(tr("Unknown Code Pointer Highlight Color"), "HexDumpUnknownCodePointerHighlightColor", "");
    colorInfoListAppend(tr("Unknown Data Pointer Highlight Color"), "HexDumpUnknownDataPointerHighlightColor", "");

    colorInfoListAppend(tr("Stack:"), "", "");
    colorInfoListAppend(tr("Text"), "StackTextColor", "");
    colorInfoListAppend(tr("Inactive Text"), "StackInactiveTextColor", "");
    colorInfoListAppend(tr("Background"), "StackBackgroundColor", "");
    colorInfoListAppend(tr("Selection"), "StackSelectionColor", "");
    colorInfoListAppend(ArchValue(tr("ESP"), tr("RSP")), "StackCspColor", "StackCspBackgroundColor");
    colorInfoListAppend(tr("Addresses"), "StackAddressColor", "StackAddressBackgroundColor");
    colorInfoListAppend(tr("Selected Addresses"), "StackSelectedAddressColor", "StackSelectedAddressBackgroundColor");
    colorInfoListAppend(tr("Labels"), "StackLabelColor", "StackLabelBackgroundColor");
    colorInfoListAppend(tr("Return To Comment"), "StackReturnToColor", "");
    colorInfoListAppend(tr("SEH Chain Comment"), "StackSEHChainColor", "");
    colorInfoListAppend(tr("User Stack Frame Line"), "StackFrameColor", "");
    colorInfoListAppend(tr("System Stack Frame Line"), "StackFrameSystemColor", "");

    colorInfoListAppend(tr("HexEdit:"), "", "");
    colorInfoListAppend(tr("Text"), "HexEditTextColor", "");
    colorInfoListAppend(tr("Wildcards"), "HexEditWildcardColor", "");
    colorInfoListAppend(tr("Background"), "HexEditBackgroundColor", "");
    colorInfoListAppend(tr("Selection"), "HexEditSelectionColor", "");

    colorInfoListAppend(tr("Graph:"), "", "");
    colorInfoListAppend(tr("Background"), "GraphBackgroundColor", "");
    colorInfoListAppend(tr("Node"), "GraphNodeColor", "GraphNodeBackgroundColor");
    colorInfoListAppend(tr("Current node shadow"), "GraphCurrentShadowColor", "");
    colorInfoListAppend(tr("Terminal node shadow"), "GraphRetShadowColor", "");
    colorInfoListAppend(tr("Indirect call shadow"), "GraphIndirectcallShadowColor", "");
    colorInfoListAppend(tr("Unconditional branch line"), "GraphJmpColor", "");
    colorInfoListAppend(tr("True branch line"), "GraphBrtrueColor", "");
    colorInfoListAppend(tr("False branch line"), "GraphBrfalseColor", "");
    colorInfoListAppend(ArchValue(tr("EIP"), tr("RIP")), "GraphCipColor", "");
    colorInfoListAppend(tr("Breakpoint"), "GraphBreakpointColor", "");
    colorInfoListAppend(tr("Disabled Breakpoint"), "GraphDisabledBreakpointColor", "");

    colorInfoListAppend(tr("Other:"), "", "");
    colorInfoListAppend(tr("Current Thread"), "ThreadCurrentColor", "ThreadCurrentBackgroundColor");
    colorInfoListAppend(tr("Watch (When Watchdog is Triggered)"), "WatchTriggeredColor", "WatchTriggeredBackgroundColor");
    colorInfoListAppend(tr("Memory Map Breakpoint"), "MemoryMapBreakpointColor", "MemoryMapBreakpointBackgroundColor");
    colorInfoListAppend(tr("Memory Map %1").arg(ArchValue(tr("EIP"), tr("RIP"))), "MemoryMapCipColor", "MemoryMapCipBackgroundColor");
    colorInfoListAppend(tr("Memory Map Section Text"), "MemoryMapSectionTextColor", "");
    colorInfoListAppend(tr("Search Highlight Color"), "SearchListViewHighlightColor", "SearchListViewHighlightBackgroundColor");
    colorInfoListAppend(tr("Struct primary background"), "StructBackgroundColor", "");
    colorInfoListAppend(tr("Struct secondary background"), "StructAlternateBackgroundColor", "");
    colorInfoListAppend(tr("Log Link Color") + "*", "LogLinkColor", "LogLinkBackgroundColor");
    colorInfoListAppend(tr("Breakpoint Summary Parentheses"), "BreakpointSummaryParenColor", "");
    colorInfoListAppend(tr("Breakpoint Summary Keywords"), "BreakpointSummaryKeywordColor", "");
    colorInfoListAppend(tr("Breakpoint Summary Strings"), "BreakpointSummaryStringColor", "");
    colorInfoListAppend(tr("Patch located in relocation region"), "PatchRelocatedByteHighlightColor", "");
    colorInfoListAppend(tr("Symbol User Module Text"), "SymbolUserTextColor", "");
    colorInfoListAppend(tr("Symbol System Module Text"), "SymbolSystemTextColor", "");
    colorInfoListAppend(tr("Symbol Unloaded Text"), "SymbolUnloadedTextColor", "");
    colorInfoListAppend(tr("Symbol Loading Text"), "SymbolLoadingTextColor", "");
    colorInfoListAppend(tr("Symbol Loaded Text"), "SymbolLoadedTextColor", "");
    colorInfoListAppend(tr("Background Flicker Color"), "BackgroundFlickerColor", "");

    //dev helper
    const QMap<QString, QColor>* Colors = &Config()->defaultColors;
    QString notFound;
    for(int i = 0; i < Colors->size(); i++)
    {
        QString id = Colors->keys().at(i);
        bool bFound = false;
        for(int j = 0; j < colorInfoList.size(); j++)
        {
            if(colorInfoList.at(j).colorName == id || colorInfoList.at(j).backgroundColorName == id)
            {
                bFound = true;
                break;
            }
        }
        if(!bFound) //color not found in info list
            notFound += id + "\n";
    }
    if(notFound.length())
        SimpleWarningBox(this, tr("NOT FOUND IN CONFIG!"), notFound);

    //setup context menu
    ui->listColorNames->setContextMenuPolicy(Qt::ActionsContextMenu);
    defaultValueAction = new QAction(tr("&Default Value"), this);
    defaultValueAction->setEnabled(false);
    connect(defaultValueAction, SIGNAL(triggered()), this, SLOT(defaultValueSlot()));
    currentSettingAction = new QAction(tr("&Current Setting"), this);
    currentSettingAction->setEnabled(false);
    connect(currentSettingAction, SIGNAL(triggered()), this, SLOT(currentSettingSlot()));
    ui->listColorNames->addAction(defaultValueAction);
    ui->listColorNames->addAction(currentSettingAction);
}

void AppearanceDialog::fontInit()
{
    isInit = true;
    //AbstractTableView
    QFont font = fontMap->find("AbstractTableView").value();
    ui->fontAbstractTables->setCurrentFont(QFont(font.family()));
    if(font.bold() && font.italic())
        ui->fontAbstractTablesStyle->setCurrentIndex(3);
    else if(font.italic())
        ui->fontAbstractTablesStyle->setCurrentIndex(2);
    else if(font.bold())
        ui->fontAbstractTablesStyle->setCurrentIndex(1);
    else
        ui->fontAbstractTablesStyle->setCurrentIndex(0);
    int index = ui->fontAbstractTablesSize->findText(QString("%1").arg(font.pointSize()));
    if(index != -1)
        ui->fontAbstractTablesSize->setCurrentIndex(index);
    //Disassembly
    font = fontMap->find("Disassembly").value();
    ui->fontDisassembly->setCurrentFont(QFont(font.family()));
    if(font.bold() && font.italic())
        ui->fontDisassemblyStyle->setCurrentIndex(3);
    else if(font.italic())
        ui->fontDisassemblyStyle->setCurrentIndex(2);
    else if(font.bold())
        ui->fontDisassemblyStyle->setCurrentIndex(1);
    else
        ui->fontDisassemblyStyle->setCurrentIndex(0);
    index = ui->fontDisassemblySize->findText(QString("%1").arg(font.pointSize()));
    if(index != -1)
        ui->fontDisassemblySize->setCurrentIndex(index);
    //HexDump
    font = fontMap->find("HexDump").value();
    ui->fontHexDump->setCurrentFont(QFont(font.family()));
    if(font.bold() && font.italic())
        ui->fontHexDumpStyle->setCurrentIndex(3);
    else if(font.italic())
        ui->fontHexDumpStyle->setCurrentIndex(2);
    else if(font.bold())
        ui->fontHexDumpStyle->setCurrentIndex(1);
    else
        ui->fontHexDumpStyle->setCurrentIndex(0);
    index = ui->fontHexDumpSize->findText(QString("%1").arg(font.pointSize()));
    if(index != -1)
        ui->fontHexDumpSize->setCurrentIndex(index);
    //Stack
    font = fontMap->find("Stack").value();
    ui->fontStack->setCurrentFont(QFont(font.family()));
    if(font.bold() && font.italic())
        ui->fontStackStyle->setCurrentIndex(3);
    else if(font.italic())
        ui->fontStackStyle->setCurrentIndex(2);
    else if(font.bold())
        ui->fontStackStyle->setCurrentIndex(1);
    else
        ui->fontStackStyle->setCurrentIndex(0);
    index = ui->fontStackSize->findText(QString("%1").arg(font.pointSize()));
    if(index != -1)
        ui->fontStackSize->setCurrentIndex(index);
    //Registers
    font = fontMap->find("Registers").value();
    ui->fontRegisters->setCurrentFont(QFont(font.family()));
    if(font.bold() && font.italic())
        ui->fontRegistersStyle->setCurrentIndex(3);
    else if(font.italic())
        ui->fontRegistersStyle->setCurrentIndex(2);
    else if(font.bold())
        ui->fontRegistersStyle->setCurrentIndex(1);
    else
        ui->fontRegistersStyle->setCurrentIndex(0);
    index = ui->fontRegistersSize->findText(QString("%1").arg(font.pointSize()));
    if(index != -1)
        ui->fontRegistersSize->setCurrentIndex(index);
    //HexEdit
    font = fontMap->find("HexEdit").value();
    ui->fontHexEdit->setCurrentFont(QFont(font.family()));
    if(font.bold() && font.italic())
        ui->fontHexEditStyle->setCurrentIndex(3);
    else if(font.italic())
        ui->fontHexEditStyle->setCurrentIndex(2);
    else if(font.bold())
        ui->fontHexEditStyle->setCurrentIndex(1);
    else
        ui->fontHexEditStyle->setCurrentIndex(0);
    index = ui->fontHexEditSize->findText(QString("%1").arg(font.pointSize()));
    if(index != -1)
        ui->fontHexEditSize->setCurrentIndex(index);
    //Log
    font = fontMap->find("Log").value();
    ui->fontLog->setCurrentFont(QFont(font.family()));
    if(font.bold() && font.italic())
        ui->fontLogStyle->setCurrentIndex(3);
    else if(font.italic())
        ui->fontLogStyle->setCurrentIndex(2);
    else if(font.bold())
        ui->fontLogStyle->setCurrentIndex(1);
    else
        ui->fontLogStyle->setCurrentIndex(0);
    index = ui->fontLogSize->findText(QString("%1").arg(font.pointSize()));
    if(index != -1)
        ui->fontLogSize->setCurrentIndex(index);
    //Application
    ui->labelApplicationFont->setText(fontMap->find("Application").value().family());
    isInit = false;
}

void AppearanceDialog::selectColor(QLineEdit* lineEdit, QColorDialog::ColorDialogOptions options)
{
    colorLineEdit = lineEdit;
    auto oldText = lineEdit->text();
    QColor initialColor;
    if(oldText.toUpper() == "#XXXXXX")
        initialColor = Qt::black; //transparent will set the alpha channel, which users will forget
    else
        initialColor = QColor(oldText);
    QColorDialog dialog(initialColor, this);
    dialog.setWindowTitle(tr("Select Color"));
    dialog.setOptions(options);
    connect(&dialog, &QColorDialog::currentColorChanged, this, &AppearanceDialog::colorSelectionChangedSlot);
    duint customColorCount = 0;
    BridgeSettingGetUint("Colors", "CustomColorCount", &customColorCount);
    if(customColorCount > 0)
    {
        for(duint i = 0; i < customColorCount; i++)
        {
            char customColorText[MAX_SETTING_SIZE] = "";
            if(BridgeSettingGet("Colors", QString("CustomColor%1").arg(i).toUtf8().constData(), customColorText))
            {
                QColor customColor;
                if(strcmp(customColorText, "#XXXXXX") == 0)
                    customColor = Qt::transparent;
                else
                    customColor = QColor(customColorText);
                dialog.setCustomColor(i, customColor);
            }
        }
    }
    auto result = dialog.exec();
    for(duint i = 0; i < dialog.customCount(); i++)
    {
        QColor customColor = dialog.customColor(i);
        QString colorName = customColor.name().toUpper();
        if(!customColor.alpha())
            colorName = "#XXXXXX";
        BridgeSettingSet("Colors", QString("CustomColor%1").arg(i).toUtf8().constData(), colorName.toUtf8().constData());
    }
    BridgeSettingSetUint("Colors", "CustomColorCount", dialog.customCount());
    colorLineEdit = nullptr;
    if(result == QDialog::Accepted)
    {
        lineEdit->setText(colorToString(dialog.selectedColor()));
    }
    else
    {
        lineEdit->setText(oldText);
    }
}

QString AppearanceDialog::colorToString(const QColor & color)
{
    if(!color.alpha())
        return "#XXXXXX";
    return color.name();
}

void AppearanceDialog::on_fontAbstractTables_currentFontChanged(const QFont & f)
{
    QString id = "AbstractTableView";
    QFont font = fontMap->find(id).value();
    font.setFamily(f.family());
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontAbstractTablesStyle_currentIndexChanged(int index)
{
    QString id = "AbstractTableView";
    QFont font = fontMap->find(id).value();
    font.setBold(false);
    font.setItalic(false);
    font.setKerning(false);
    if(index == 1 || index == 3)
        font.setBold(true);
    if(index == 2 || index == 3)
        font.setItalic(true);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontAbstractTablesSize_currentIndexChanged(const QString & arg1)
{
    QString id = "AbstractTableView";
    QFont font = fontMap->find(id).value();
    font.setPointSize(arg1.toInt());
    font.setKerning(false);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontDisassembly_currentFontChanged(const QFont & f)
{
    QString id = "Disassembly";
    QFont font = fontMap->find(id).value();
    font.setFamily(f.family());
    font.setKerning(false);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontDisassemblyStyle_currentIndexChanged(int index)
{
    QString id = "Disassembly";
    QFont font = fontMap->find(id).value();
    font.setBold(false);
    font.setItalic(false);
    font.setKerning(false);
    if(index == 1 || index == 3)
        font.setBold(true);
    if(index == 2 || index == 3)
        font.setItalic(true);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontDisassemblySize_currentIndexChanged(const QString & arg1)
{
    QString id = "Disassembly";
    QFont font = fontMap->find(id).value();
    font.setPointSize(arg1.toInt());
    font.setKerning(false);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontHexDump_currentFontChanged(const QFont & f)
{
    QString id = "HexDump";
    QFont font = fontMap->find(id).value();
    font.setFamily(f.family());
    font.setKerning(false);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontHexDumpStyle_currentIndexChanged(int index)
{
    QString id = "HexDump";
    QFont font = fontMap->find(id).value();
    font.setBold(false);
    font.setItalic(false);
    font.setKerning(false);
    if(index == 1 || index == 3)
        font.setBold(true);
    if(index == 2 || index == 3)
        font.setItalic(true);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontHexDumpSize_currentIndexChanged(const QString & arg1)
{
    QString id = "HexDump";
    QFont font = fontMap->find(id).value();
    font.setPointSize(arg1.toInt());
    font.setKerning(false);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontStack_currentFontChanged(const QFont & f)
{
    QString id = "Stack";
    QFont font = fontMap->find(id).value();
    font.setFamily(f.family());
    font.setKerning(false);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontStackStyle_currentIndexChanged(int index)
{
    QString id = "Stack";
    QFont font = fontMap->find(id).value();
    font.setBold(false);
    font.setItalic(false);
    font.setKerning(false);
    if(index == 1 || index == 3)
        font.setBold(true);
    if(index == 2 || index == 3)
        font.setItalic(true);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontStackSize_currentIndexChanged(const QString & arg1)
{
    QString id = "Stack";
    QFont font = fontMap->find(id).value();
    font.setPointSize(arg1.toInt());
    font.setKerning(false);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontRegisters_currentFontChanged(const QFont & f)
{
    QString id = "Registers";
    QFont font = fontMap->find(id).value();
    font.setFamily(f.family());
    font.setKerning(false);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontRegistersStyle_currentIndexChanged(int index)
{
    QString id = "Registers";
    QFont font = fontMap->find(id).value();
    font.setBold(false);
    font.setItalic(false);
    font.setKerning(false);
    if(index == 1 || index == 3)
        font.setBold(true);
    if(index == 2 || index == 3)
        font.setItalic(true);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontRegistersSize_currentIndexChanged(const QString & arg1)
{
    QString id = "Registers";
    QFont font = fontMap->find(id).value();
    font.setPointSize(arg1.toInt());
    font.setKerning(false);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontHexEdit_currentFontChanged(const QFont & f)
{
    QString id = "HexEdit";
    QFont font = fontMap->find(id).value();
    font.setFamily(f.family());
    font.setKerning(false);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontHexEditStyle_currentIndexChanged(int index)
{
    QString id = "HexEdit";
    QFont font = fontMap->find(id).value();
    font.setBold(false);
    font.setItalic(false);
    font.setKerning(false);
    if(index == 1 || index == 3)
        font.setBold(true);
    if(index == 2 || index == 3)
        font.setItalic(true);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontHexEditSize_currentIndexChanged(const QString & arg1)
{
    QString id = "HexEdit";
    QFont font = fontMap->find(id).value();
    font.setPointSize(arg1.toInt());
    font.setKerning(false);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontLog_currentFontChanged(const QFont & f)
{
    QString id = "Log";
    QFont font = fontMap->find(id).value();
    font.setFamily(f.family());
    font.setKerning(false);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontLogStyle_currentIndexChanged(int index)
{
    QString id = "Log";
    QFont font = fontMap->find(id).value();
    font.setBold(false);
    font.setItalic(false);
    font.setKerning(false);
    if(index == 1 || index == 3)
        font.setBold(true);
    if(index == 2 || index == 3)
        font.setItalic(true);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontLogSize_currentIndexChanged(const QString & arg1)
{
    QString id = "Log";
    QFont font = fontMap->find(id).value();
    font.setPointSize(arg1.toInt());
    font.setKerning(false);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_buttonApplicationFont_clicked()
{
    QString id = "Application";
    QFontDialog fontDialog(this);
    fontDialog.setCurrentFont(fontMap->find(id).value());
    if(fontDialog.exec() != QDialog::Accepted)
        return;
    (*fontMap)[id] = fontDialog.currentFont();
    ui->labelApplicationFont->setText(fontDialog.currentFont().family());
    if(isInit)
        return;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_buttonFontDefaults_clicked()
{
    (*fontMap) = Config()->defaultFonts;
    isInit = true;
    fontInit();
    isInit = false;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::rejectedSlot()
{
    Config()->Colors = colorBackupMap;
    emit Config()->colorsUpdated();
    Config()->Fonts = fontBackupMap;
    emit Config()->fontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::colorSelectionChangedSlot(QColor color)
{
    if(colorLineEdit)
        colorLineEdit->setText(colorToString(color));
}
