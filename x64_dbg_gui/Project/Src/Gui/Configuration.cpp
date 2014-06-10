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

void Configuration::readColors()
{
    //setup default color map
    QMap<QString,QColor> defaultColorMap;
    defaultColorMap.insert("DisassemblyCipColor", QColor("#000000"));
    defaultColorMap.insert("DisassemblyMainBpColor", QColor("#FF0000"));
    defaultColorMap.insert("DisassemblyOtherBpColor", QColor("#FFFBF0"));
    defaultColorMap.insert("DisassemblyBookmarkColor", QColor("#FEE970"));
    defaultColorMap.insert("DisassemblyMainLabelColor", QColor("#FF0000"));
    defaultColorMap.insert("blackaddress", QColor("#000000"));
    defaultColorMap.insert("DisassemblySelectedAddressColor", QColor("#000000"));
    defaultColorMap.insert("DisassemblyBytesColor", QColor("#000000"));
    defaultColorMap.insert("DisassemblyCommentColor", QColor("#000000"));
    defaultColorMap.insert("IPLabel", QColor("#FFFFFF"));
    defaultColorMap.insert("IPLabelBG", QColor("#4040FF"));
    Colors = defaultColors = defaultColorMap;
    //read config
    for(int i=0; i<Colors.size(); i++)
    {
        QString id=Colors.keys().at(i);
        Colors[id]=colorFromConfig(id);
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
    else
        return Qt::black;
}

bool Configuration::colorToConfig(const QString id, const QColor color)
{
    return BridgeSettingSet("Colors", id.toUtf8().constData(), color.name().toUtf8().constData());
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
        return QColor("#000000"); //black is default
    }
    QColor color(setting);
    if(!color.isValid())
    {
        QColor ret = defaultColors.find(id).value(); //return default
        colorToConfig(id, ret);
        return ret;
    }
    return color;
}

