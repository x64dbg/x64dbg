#include "Configuration.h"

Configuration* Configuration::mPtr = NULL;


Configuration::Configuration()
{
    load();
    mPtr = this;
}

Configuration *Configuration::instance()
{
    return mPtr;
}

void Configuration::load()
{
    readColors();
}

void Configuration::save()
{
    writeColors();
}

void Configuration::readColors()
{
    //setup default color map
    QMap<QString, QColor> defaultColorMap;
    defaultColorMap.insert("AbstractTableViewSeparatorColor", QColor("#808080"));
    defaultColorMap.insert("AbstractTableViewBackgroundColor", QColor("#FFFBF0"));

    defaultColorMap.insert("DisassemblyCipColor", QColor("#FFFFFF"));
    defaultColorMap.insert("DisassemblyCipBackgroundColor", QColor("#000000"));
    defaultColorMap.insert("DisassemblyBreakpointColor", QColor("#000000"));
    defaultColorMap.insert("DisassemblyBreakpointBackgroundColor", QColor("#FF0000"));
    defaultColorMap.insert("DisassemblyHardwareBreakpointColor", QColor("#000000"));
    defaultColorMap.insert("DisassemblyHardwareBreakpointBackgroundColor", Qt::transparent);
    defaultColorMap.insert("DisassemblyBookmarkColor", QColor("#000000"));
    defaultColorMap.insert("DisassemblyBookmarkBackgroundColor", QColor("#FEE970"));
    defaultColorMap.insert("DisassemblyLabelColor", QColor("#FF0000"));
    defaultColorMap.insert("DisassemblyLabelBackgroundColor", Qt::transparent);
    defaultColorMap.insert("DisassemblyBackgroundColor", QColor("#FFFBF0"));
    defaultColorMap.insert("DisassemblySelectionColor", QColor("#C0C0C0"));
    defaultColorMap.insert("DisassemblyAddressColor", QColor("#808080"));
    defaultColorMap.insert("DisassemblySelectedAddressColor", QColor("#000000"));
    defaultColorMap.insert("DisassemblyJumpLineTrueColor", QColor("#FF0000"));
    defaultColorMap.insert("DisassemblyJumpLineFalseColor", QColor("#808080"));
    defaultColorMap.insert("DisassemblyBytesColor", QColor("#000000"));
    defaultColorMap.insert("DisassemblyCommentColor", QColor("#000000"));
    defaultColorMap.insert("DisassemblyCommentBackgroundColor", Qt::transparent);

    defaultColorMap.insert("SideBarCipLabelColor", QColor("#FFFFFF"));
    defaultColorMap.insert("SideBarCipLabelBackgroundColor", QColor("#4040FF"));
    defaultColorMap.insert("SideBarBackgroundColor", QColor("#FFFBF0"));
    defaultColorMap.insert("SideBarConditionalJumpLineTrueColor", QColor("#FF0000"));
    defaultColorMap.insert("SideBarConditionalJumpLineFalseColor", QColor("#808080"));
    defaultColorMap.insert("SideBarUnconditionalJumpLineTrueColor", QColor("#FF0000"));
    defaultColorMap.insert("SideBarUnconditionalJumpLineFalseColor", QColor("#808080"));
    defaultColorMap.insert("SideBarBulletColor", QColor("#808080"));
    defaultColorMap.insert("SideBarBulletBreakpointColor", QColor("#FF0000"));
    defaultColorMap.insert("SideBarBulletDisabledBreakpointColor", QColor("#FF0000"));
    defaultColorMap.insert("SideBarBulletBookmarkColor", QColor("#FEE970"));

    defaultColorMap.insert("RegistersBackgroundColor", QColor("#FFFBF0"));
    defaultColorMap.insert("RegistersColor", QColor("#000000"));
    defaultColorMap.insert("RegistersModifiedColor", QColor("#FF0000"));
    defaultColorMap.insert("RegistersSelectionColor", QColor("#EEEEEE"));
    defaultColorMap.insert("RegistersLabelColor", QColor("#000000"));
    defaultColorMap.insert("RegistersExtraInfoColor", QColor("#000000"));

    Colors = defaultColors = defaultColorMap;
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
}

const QList<QString> Configuration::ApiFingerprints()
{
    char setting[MAX_SETTING_SIZE]="";
    if(!BridgeSettingGet("Engine", "APIFingerprints", setting))
    {
        strcpy(setting, "gdi32,kernel32,shell32,stdio,user32"); //default setting
        BridgeSettingSet("Engine", "APIFingerprints", setting);
    }
    return QString(setting).split(QChar(','), QString::SkipEmptyParts);
}

const QColor Configuration::color(QString id)
{
    if(Colors.contains(id))
        return Colors.constFind(id).value();
    return Qt::black;
}

bool Configuration::colorToConfig(const QString id, const QColor color)
{
    QString colorName=color.name().toUpper();
    if(!color.alpha())
        colorName="#XXXXXX";
    return BridgeSettingSet("Colors", id.toUtf8().constData(), colorName.toUtf8().constData());
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

