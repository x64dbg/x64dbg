#include "Configuration.h"
#include "Bridge.h"
#include <QMessageBox>

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
    defaultColors.insert("DisassemblyCommentColor", QColor("#000000"));
    defaultColors.insert("DisassemblyCommentBackgroundColor", Qt::transparent);

    defaultColors.insert("SideBarCipLabelColor", QColor("#FFFFFF"));
    defaultColors.insert("SideBarCipLabelBackgroundColor", QColor("#4040FF"));
    defaultColors.insert("SideBarBackgroundColor", QColor("#FFF8F0"));
    defaultColors.insert("SideBarConditionalJumpLineTrueColor", QColor("#FF0000"));
    defaultColors.insert("SideBarConditionalJumpLineFalseColor", QColor("#808080"));
    defaultColors.insert("SideBarUnconditionalJumpLineTrueColor", QColor("#FF0000"));
    defaultColors.insert("SideBarUnconditionalJumpLineFalseColor", QColor("#808080"));
    defaultColors.insert("SideBarBulletColor", QColor("#808080"));
    defaultColors.insert("SideBarBulletBreakpointColor", QColor("#FF0000"));
    defaultColors.insert("SideBarBulletDisabledBreakpointColor", QColor("#FF0000"));
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
    defaultColors.insert("InstructionMemorySizeColor", QColor("#000080"));
    defaultColors.insert("InstructionMemorySizeBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionMemorySegmentColor", QColor("#FF00FF"));
    defaultColors.insert("InstructionMemorySegmentBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionMemoryBracketsColor", QColor("#000000"));
    defaultColors.insert("InstructionMemoryBracketsBackgroundColor", Qt::transparent);
    defaultColors.insert("InstructionMemoryStackBracketsColor", QColor("#000000"));
    defaultColors.insert("InstructionMemoryStackBracketsBackgroundColor",QColor("#00FFFF"));
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
    defaultColors.insert("InstructionSseRegisterColor", QColor("#000080"));
    defaultColors.insert("InstructionSseRegisterBackgroundColor", Qt::transparent);

    defaultColors.insert("HexDumpTextColor", QColor("#000000"));
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

    //bool settings
    QMap<QString, bool> disassemblyBool;
    disassemblyBool.insert("ArgumentSpaces", false);
    disassemblyBool.insert("MemorySpaces", false);
    disassemblyBool.insert("FillNOPs", false);
    defaultBools.insert("Disassembler", disassemblyBool);

    //uint settings
    QMap<QString, uint_t> hexdumpUint;
    hexdumpUint.insert("DefaultView", 0);
    defaultUints.insert("HexDump", hexdumpUint);

    load();
    mPtr = this;
}

Configuration *Config()
{
    return mPtr;
}

void Configuration::load()
{
    readColors();
    readBools();
    readUints();
}

void Configuration::save()
{
    writeColors();
    writeBools();
    writeUints();
}

void Configuration::readColors()
{
    Colors = defaultColors;
    //read config
    for(int i=0; i<Colors.size(); i++)
    {
        QString id=Colors.keys().at(i);
        Colors[id]=colorFromConfig(id);
    }
}

void Configuration::writeColors()
{
    //write config
    for(int i=0; i<Colors.size(); i++)
    {
        QString id=Colors.keys().at(i);
        colorToConfig(id, Colors[id]);
    }
    emit colorsUpdated();
}

void Configuration::readBools()
{
    Bools = defaultBools;
    //read config
    for(int i=0; i<Bools.size(); i++)
    {
        QString category=Bools.keys().at(i);
        QMap<QString, bool>* currentBool=&Bools[category];
        for(int j=0; j<currentBool->size(); j++)
        {
            QString id=(*currentBool).keys().at(j);
            (*currentBool)[id]=boolFromConfig(category, id);
        }
    }
}

void Configuration::writeBools()
{
    //write config
    for(int i=0; i<Bools.size(); i++)
    {
        QString category=Bools.keys().at(i);
        QMap<QString, bool>* currentBool=&Bools[category];
        for(int j=0; j<currentBool->size(); j++)
        {
            QString id=(*currentBool).keys().at(j);
            boolToConfig(category, id, (*currentBool)[id]);
        }
    }
}

void Configuration::readUints()
{
    Uints = defaultUints;
    //read config
    for(int i=0; i<Bools.size(); i++)
    {
        QString category=Uints.keys().at(i);
        QMap<QString, uint_t>* currentUint=&Uints[category];
        for(int j=0; j<currentUint->size(); j++)
        {
            QString id=(*currentUint).keys().at(j);
            (*currentUint)[id]=uintFromConfig(category, id);
        }
    }
}

void Configuration::writeUints()
{
    //write config
    for(int i=0; i<Bools.size(); i++)
    {
        QString category=Uints.keys().at(i);
        QMap<QString, uint_t>* currentUint=&Uints[category];
        for(int j=0; j<currentUint->size(); j++)
        {
            QString id=(*currentUint).keys().at(j);
            uintToConfig(category, id, (*currentUint)[id]);
        }
    }
}

const QColor Configuration::getColor(const QString id)
{
    if(Colors.contains(id))
        return Colors.constFind(id).value();
    QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", id);
    msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
    msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
    msg.exec();
    return Qt::black;
}

const bool Configuration::getBool(const QString category, const QString id)
{
    if(Bools.contains(category))
    {
        if(Bools[category].contains(id))
            return Bools[category][id];
        QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category+":"+id);
        msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
        msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return false;
    }
    QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category);
    msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
    msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
    msg.exec();
    return false;
}

void Configuration::setBool(const QString category, const QString id, const bool b)
{
    if(Bools.contains(category))
    {
        if(Bools[category].contains(id))
        {
            Bools[category][id]=b;
            return;
        }
        QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category+":"+id);
        msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
        msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category);
    msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
    msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

const uint_t Configuration::getUint(const QString category, const QString id)
{
    if(Uints.contains(category))
    {
        if(Uints[category].contains(id))
            return Uints[category][id];
        QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category+":"+id);
        msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
        msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return 0;
    }
    QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category);
    msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
    msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
    msg.exec();
    return 0;
}

void Configuration::setUint(const QString category, const QString id, const uint_t i)
{
    if(Uints.contains(category))
    {
        if(Uints[category].contains(id))
        {
            Uints[category][id]=i;
            return;
        }
        QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category+":"+id);
        msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
        msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    QMessageBox msg(QMessageBox::Warning, "NOT FOUND IN CONFIG!", category);
    msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
    msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

QColor Configuration::colorFromConfig(const QString id)
{
    char setting[MAX_SETTING_SIZE]="";
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
    if(QString(setting).toUpper()=="#XXXXXX") //support custom transparent color name
        return Qt::transparent;
    QColor color(setting);
    if(!color.isValid())
    {
        QColor ret = defaultColors.find(id).value(); //return default
        colorToConfig(id, ret);
        return ret;
    }
    return color;
}

bool Configuration::colorToConfig(const QString id, const QColor color)
{
    QString colorName=color.name().toUpper();
    if(!color.alpha())
        colorName="#XXXXXX";
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

uint_t Configuration::uintFromConfig(const QString category, const QString id)
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

bool Configuration::uintToConfig(const QString category, const QString id, uint_t i)
{
    return BridgeSettingSetUint(category.toUtf8().constData(), id.toUtf8().constData(), i);
}
