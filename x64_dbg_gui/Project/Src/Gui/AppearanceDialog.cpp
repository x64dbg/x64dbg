#include "AppearanceDialog.h"
#include "ui_AppearanceDialog.h"
#include <QColorDialog>
#include <QFontDialog>
#include <QMessageBox>
#include "Configuration.h"

AppearanceDialog::AppearanceDialog(QWidget* parent) : QDialog(parent), ui(new Ui::AppearanceDialog)
{
    ui->setupUi(this);
    //set window flags
    setModal(true);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
    setFixedSize(this->size()); //fixed size
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
            Config()->emitColorsUpdated();
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
                Config()->emitColorsUpdated();
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
            Config()->emitColorsUpdated();
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
    QColorDialog colorDialog(QColor(ui->editColor->text()), this);
    if(colorDialog.exec() == QDialog::Accepted)
        ui->editColor->setText(colorDialog.selectedColor().name().toUpper());
}

void AppearanceDialog::on_buttonBackgroundColor_clicked()
{
    QColor initialColor;
    if(ui->editBackgroundColor->text().toUpper() == "#XXXXXX")
        initialColor = Qt::black; //transparent will set the alpha channel, which users will forget
    else
        initialColor = QColor(ui->editBackgroundColor->text());
    QColor selectedColor = QColorDialog::getColor(initialColor, this, "Select Color", QColorDialog::ShowAlphaChannel);
    if(selectedColor.isValid())
    {
        if(!selectedColor.alpha())
            ui->editBackgroundColor->setText("#XXXXXX");
        else
            ui->editBackgroundColor->setText(selectedColor.name().toUpper());
    }
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
    GuiAddStatusBarMessage("Settings saved!\n");
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
    colorInfoListAppend("General Tables:", "", "");
    colorInfoListAppend("Text", "AbstractTableViewTextColor", "");
    colorInfoListAppend("Header Text", "AbstractTableViewHeaderTextColor", "");
    colorInfoListAppend("Background", "AbstractTableViewBackgroundColor", "");
    colorInfoListAppend("Selection", "AbstractTableViewSelectionColor", "");
    colorInfoListAppend("Separators", "AbstractTableViewSeparatorColor", "");

    colorInfoListAppend("Disassembly:", "", "");
    colorInfoListAppend("Background", "DisassemblyBackgroundColor", "");
    colorInfoListAppend("Selection", "DisassemblySelectionColor", "");
    colorInfoListAppend("Bytes", "DisassemblyBytesColor", "");
    colorInfoListAppend("Modified Bytes", "DisassemblyModifiedBytesColor", "");
#ifdef _WIN64
    colorInfoListAppend("RIP", "DisassemblyCipColor", "DisassemblyCipBackgroundColor");
#else //x86
    colorInfoListAppend("EIP", "DisassemblyCipColor", "DisassemblyCipBackgroundColor");
#endif //_WIN64
    colorInfoListAppend("Breakpoints", "DisassemblyBreakpointColor", "DisassemblyBreakpointBackgroundColor");
    colorInfoListAppend("Hardware Breakpoints", "DisassemblyHardwareBreakpointColor", "DisassemblyHardwareBreakpointBackgroundColor");
    colorInfoListAppend("Bookmarks", "DisassemblyBookmarkColor", "DisassemblyBookmarkBackgroundColor");
    colorInfoListAppend("Comments", "DisassemblyCommentColor", "DisassemblyCommentBackgroundColor");
    colorInfoListAppend("Automatic Comments", "DisassemblyAutoCommentColor", "DisassemblyAutoCommentBackgroundColor");
    colorInfoListAppend("Labels", "DisassemblyLabelColor", "DisassemblyLabelBackgroundColor");
    colorInfoListAppend("Addresses", "DisassemblyAddressColor", "DisassemblyAddressBackgroundColor");
    colorInfoListAppend("Selected Addresses", "DisassemblySelectedAddressColor", "DisassemblySelectedAddressBackgroundColor");
    colorInfoListAppend("Conditional Jump Lines (jump)", "DisassemblyConditionalJumpLineTrueColor", "");
    colorInfoListAppend("Conditional Jump Lines (no jump)", "DisassemblyConditionalJumpLineFalseColor", "");
    colorInfoListAppend("Unconditional Jump Lines", "DisassemblyUnconditionalJumpLineColor", "");

    colorInfoListAppend("SideBar:", "", "");
#ifdef _WIN64
    colorInfoListAppend("RIP Label", "SideBarCipLabelColor", "SideBarCipLabelBackgroundColor");
#else //x86
    colorInfoListAppend("EIP Label", "SideBarCipLabelColor", "SideBarCipLabelBackgroundColor");
#endif //_WIN64
    colorInfoListAppend("Bullets", "SideBarBulletColor", "");
    colorInfoListAppend("Breakpoints", "SideBarBulletBreakpointColor", "");
    colorInfoListAppend("Disabled Breakpoints", "SideBarBulletDisabledBreakpointColor", "");
    colorInfoListAppend("Bookmarks", "SideBarBulletBookmarkColor", "");
    colorInfoListAppend("Conditional Jump Lines (jump)", "SideBarConditionalJumpLineTrueColor", "");
    colorInfoListAppend("Conditional Jump Lines (no jump)", "SideBarConditionalJumpLineFalseColor", "");
    colorInfoListAppend("Unconditional Jump Lines (jump)", "SideBarUnconditionalJumpLineTrueColor", "");
    colorInfoListAppend("Unconditional Jump Lines (no jump)", "SideBarUnconditionalJumpLineFalseColor", "");
    colorInfoListAppend("Jump Lines (executing)", "SideBarJumpLineExecuteColor", "");
    colorInfoListAppend("Background", "SideBarBackgroundColor", "");

    colorInfoListAppend("Registers:", "", "");
    colorInfoListAppend("Text", "RegistersColor", "");
    colorInfoListAppend("Background", "RegistersBackgroundColor", "");
    colorInfoListAppend("Selection", "RegistersSelectionColor", "");
    colorInfoListAppend("Modified Registers", "RegistersModifiedColor", "");
    colorInfoListAppend("Register Names", "RegistersLabelColor", "");
    colorInfoListAppend("Extra Information", "RegistersExtraInfoColor", "");

    colorInfoListAppend("Instructions:", "", "");
    colorInfoListAppend("Text", "InstructionUncategorizedColor", "InstructionUncategorizedBackgroundColor");
    colorInfoListAppend("Highlighting", "InstructionHighlightColor", "");
    colorInfoListAppend("Commas", "InstructionCommaColor", "InstructionCommaBackgroundColor");
    colorInfoListAppend("Prefixes", "InstructionPrefixColor", "InstructionPrefixBackgroundColor");
    colorInfoListAppend("Addresses", "InstructionAddressColor", "InstructionAddressBackgroundColor");
    colorInfoListAppend("Values", "InstructionValueColor", "InstructionValueBackgroundColor");
    colorInfoListAppend("Mnemonics", "InstructionMnemonicColor", "InstructionMnemonicBackgroundColor");
    colorInfoListAppend("Push/Pops", "InstructionPushPopColor", "InstructionPushPopBackgroundColor");
    colorInfoListAppend("Calls", "InstructionCallColor", "InstructionCallBackgroundColor");
    colorInfoListAppend("Returns", "InstructionRetColor", "InstructionRetBackgroundColor");
    colorInfoListAppend("Conditional Jumps", "InstructionConditionalJumpColor", "InstructionConditionalJumpBackgroundColor");
    colorInfoListAppend("Unconditional Jumps", "InstructionUnconditionalJumpColor", "InstructionUnconditionalJumpBackgroundColor");
    colorInfoListAppend("NOPs", "InstructionNopColor", "InstructionNopBackgroundColor");
    colorInfoListAppend("FAR", "InstructionFarColor", "InstructionFarBackgroundColor");
    colorInfoListAppend("INT3s", "InstructionInt3Color", "InstructionInt3BackgroundColor");
    colorInfoListAppend("General Registers", "InstructionGeneralRegisterColor", "InstructionGeneralRegisterBackgroundColor");
    colorInfoListAppend("FPU Registers", "InstructionFpuRegisterColor", "InstructionFpuRegisterBackgroundColor");
    colorInfoListAppend("SSE Registers", "InstructionSseRegisterColor", "InstructionSseRegisterBackgroundColor");
    colorInfoListAppend("MMX Registers", "InstructionMmxRegisterColor", "InstructionMmxRegisterBackgroundColor");
    colorInfoListAppend("Memory Sizes", "InstructionMemorySizeColor", "InstructionMemorySizeBackgroundColor");
    colorInfoListAppend("Memory Segments", "InstructionMemorySegmentColor", "InstructionMemorySegmentBackgroundColor");
    colorInfoListAppend("Memory Brackets", "InstructionMemoryBracketsColor", "InstructionMemoryBracketsBackgroundColor");
    colorInfoListAppend("Memory Stack Brackets", "InstructionMemoryStackBracketsColor", "InstructionMemoryStackBracketsBackgroundColor");
    colorInfoListAppend("Memory Base Registers", "InstructionMemoryBaseRegisterColor", "InstructionMemoryBaseRegisterBackgroundColor");
    colorInfoListAppend("Memory Index Registers", "InstructionMemoryIndexRegisterColor", "InstructionMemoryIndexRegisterBackgroundColor");
    colorInfoListAppend("Memory Scales", "InstructionMemoryScaleColor", "InstructionMemoryScaleBackgroundColor");
    colorInfoListAppend("Memory Operators (+/-/*)", "InstructionMemoryOperatorColor", "InstructionMemoryOperatorBackgroundColor");

    colorInfoListAppend("HexDump:", "", "");
    colorInfoListAppend("Text", "HexDumpTextColor", "");
    colorInfoListAppend("Modified Bytes", "HexDumpModifiedBytesColor", "");
    colorInfoListAppend("Background", "HexDumpBackgroundColor", "");
    colorInfoListAppend("Selection", "HexDumpSelectionColor", "");
    colorInfoListAppend("Addresses", "HexDumpAddressColor", "HexDumpAddressBackgroundColor");
    colorInfoListAppend("Labels", "HexDumpLabelColor", "HexDumpLabelBackgroundColor");

    colorInfoListAppend("Stack:", "", "");
    colorInfoListAppend("Text", "StackTextColor", "");
    colorInfoListAppend("Inactive Text", "StackInactiveTextColor", "");
    colorInfoListAppend("Background", "StackBackgroundColor", "");
    colorInfoListAppend("Selection", "StackSelectionColor", "");
#ifdef _WIN64
    colorInfoListAppend("RSP", "StackCspColor", "StackCspBackgroundColor");
#else //x86
    colorInfoListAppend("CSP", "StackCspColor", "StackCspBackgroundColor");
#endif //_WIN64
    colorInfoListAppend("Addresses", "StackAddressColor", "StackAddressBackgroundColor");
    colorInfoListAppend("Selected Addresses", "StackSelectedAddressColor", "StackSelectedAddressBackgroundColor");
    colorInfoListAppend("Labels", "StackLabelColor", "StackLabelBackgroundColor");

    colorInfoListAppend("HexEdit:", "", "");
    colorInfoListAppend("Text", "HexEditTextColor", "");
    colorInfoListAppend("Wildcards", "HexEditWildcardColor", "");
    colorInfoListAppend("Background", "HexEditBackgroundColor", "");
    colorInfoListAppend("Selection", "HexEditSelectionColor", "");

    colorInfoListAppend("Other:", "", "");
    colorInfoListAppend("Current Thread", "ThreadCurrentColor", "ThreadCurrentBackgroundColor");
    colorInfoListAppend("Memory Map Breakpoint", "MemoryMapBreakpointColor", "MemoryMapBreakpointBackgroundColor");
    colorInfoListAppend("Memory Map Section Text", "MemoryMapSectionTextColor", "");
    colorInfoListAppend("Search Highlight Color", "SearchListViewHighlightColor", "");

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
    {
        QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", notFound);
        msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
    }

    //setup context menu
    ui->listColorNames->setContextMenuPolicy(Qt::ActionsContextMenu);
    defaultValueAction = new QAction("&Default Value", this);
    defaultValueAction->setEnabled(false);
    connect(defaultValueAction, SIGNAL(triggered()), this, SLOT(defaultValueSlot()));
    currentSettingAction = new QAction("&Current Setting", this);
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
    //Application
    ui->labelApplicationFont->setText(fontMap->find("Application").value().family());
    isInit = false;
}

void AppearanceDialog::on_fontAbstractTables_currentFontChanged(const QFont & f)
{
    QString id = "AbstractTableView";
    QFont font = fontMap->find(id).value();
    font.setFamily(f.family());
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontAbstractTablesStyle_currentIndexChanged(int index)
{
    QString id = "AbstractTableView";
    QFont font = fontMap->find(id).value();
    font.setBold(false);
    font.setItalic(false);
    if(index == 1 || index == 3)
        font.setBold(true);
    if(index == 2 || index == 3)
        font.setItalic(true);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontAbstractTablesSize_currentIndexChanged(const QString & arg1)
{
    QString id = "AbstractTableView";
    QFont font = fontMap->find(id).value();
    font.setPointSize(arg1.toInt());
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontDisassembly_currentFontChanged(const QFont & f)
{
    QString id = "Disassembly";
    QFont font = fontMap->find(id).value();
    font.setFamily(f.family());
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontDisassemblyStyle_currentIndexChanged(int index)
{
    QString id = "Disassembly";
    QFont font = fontMap->find(id).value();
    font.setBold(false);
    font.setItalic(false);
    if(index == 1 || index == 3)
        font.setBold(true);
    if(index == 2 || index == 3)
        font.setItalic(true);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontDisassemblySize_currentIndexChanged(const QString & arg1)
{
    QString id = "Disassembly";
    QFont font = fontMap->find(id).value();
    font.setPointSize(arg1.toInt());
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontHexDump_currentFontChanged(const QFont & f)
{
    QString id = "HexDump";
    QFont font = fontMap->find(id).value();
    font.setFamily(f.family());
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontHexDumpStyle_currentIndexChanged(int index)
{
    QString id = "HexDump";
    QFont font = fontMap->find(id).value();
    font.setBold(false);
    font.setItalic(false);
    if(index == 1 || index == 3)
        font.setBold(true);
    if(index == 2 || index == 3)
        font.setItalic(true);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontHexDumpSize_currentIndexChanged(const QString & arg1)
{
    QString id = "HexDump";
    QFont font = fontMap->find(id).value();
    font.setPointSize(arg1.toInt());
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontStack_currentFontChanged(const QFont & f)
{
    QString id = "Stack";
    QFont font = fontMap->find(id).value();
    font.setFamily(f.family());
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontStackStyle_currentIndexChanged(int index)
{
    QString id = "Stack";
    QFont font = fontMap->find(id).value();
    font.setBold(false);
    font.setItalic(false);
    if(index == 1 || index == 3)
        font.setBold(true);
    if(index == 2 || index == 3)
        font.setItalic(true);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontStackSize_currentIndexChanged(const QString & arg1)
{
    QString id = "Stack";
    QFont font = fontMap->find(id).value();
    font.setPointSize(arg1.toInt());
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontRegisters_currentFontChanged(const QFont & f)
{
    QString id = "Registers";
    QFont font = fontMap->find(id).value();
    font.setFamily(f.family());
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontRegistersStyle_currentIndexChanged(int index)
{
    QString id = "Registers";
    QFont font = fontMap->find(id).value();
    font.setBold(false);
    font.setItalic(false);
    if(index == 1 || index == 3)
        font.setBold(true);
    if(index == 2 || index == 3)
        font.setItalic(true);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontRegistersSize_currentIndexChanged(const QString & arg1)
{
    QString id = "Registers";
    QFont font = fontMap->find(id).value();
    font.setPointSize(arg1.toInt());
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontHexEdit_currentFontChanged(const QFont & f)
{
    QString id = "HexEdit";
    QFont font = fontMap->find(id).value();
    font.setFamily(f.family());
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontHexEditStyle_currentIndexChanged(int index)
{
    QString id = "HexEdit";
    QFont font = fontMap->find(id).value();
    font.setBold(false);
    font.setItalic(false);
    if(index == 1 || index == 3)
        font.setBold(true);
    if(index == 2 || index == 3)
        font.setItalic(true);
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_fontHexEditSize_currentIndexChanged(const QString & arg1)
{
    QString id = "HexEdit";
    QFont font = fontMap->find(id).value();
    font.setPointSize(arg1.toInt());
    (*fontMap)[id] = font;
    if(isInit)
        return;
    Config()->emitFontsUpdated();
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
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::on_buttonFontDefaults_clicked()
{
    (*fontMap) = Config()->defaultFonts;
    isInit = true;
    fontInit();
    isInit = false;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}

void AppearanceDialog::rejectedSlot()
{
    Config()->Colors = colorBackupMap;
    Config()->emitColorsUpdated();
    Config()->Fonts = fontBackupMap;
    Config()->emitFontsUpdated();
    GuiUpdateAllViews();
}
