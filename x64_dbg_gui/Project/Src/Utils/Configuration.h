#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QFile>
#include <QString>
#include <QColor>
#include <QMap>
#include <QDebug>
#include <QObject>
#include "Bridge.h"

#define Config() (Configuration::instance())
#define ConfigColor(x) (Config()->getColor(x))
#define ConfigBool(x,y) (Config()->getBool(x,y))
#define ConfigUint(x,y) (Config()->getUint(x,y))
#define ConfigFont(x) (Config()->getFont(x))

class Configuration : public QObject
{
    Q_OBJECT
public:
    static Configuration* mPtr;
    Configuration();
    static Configuration* instance();
    void load();
    void save();
    void readColors();
    void writeColors();
    void readBools();
    void writeBools();
    void readUints();
    void writeUints();
    void readFonts();
    void writeFonts();

    const QColor getColor(const QString id);
    const bool getBool(const QString category, const QString id);
    void setBool(const QString category, const QString id, const bool b);
    const uint_t getUint(const QString category, const QString id);
    void setUint(const QString category, const QString id, const uint_t i);
    const QFont getFont(const QString id);

    //default setting maps
    QMap<QString, QColor> defaultColors;
    QMap<QString, QMap<QString, bool>> defaultBools;
    QMap<QString, QMap<QString, uint_t>> defaultUints;
    QMap<QString, QFont> defaultFonts;

    //public variables
    QMap<QString, QColor> Colors;
    QMap<QString, QMap<QString, bool>> Bools;
    QMap<QString, QMap<QString, uint_t>> Uints;
    QMap<QString, QFont> Fonts;

signals:
    void colorsUpdated();
    void fontsUpdated();

private:
    QColor colorFromConfig(const QString id);
    bool colorToConfig(const QString id, const QColor color);
    bool boolFromConfig(const QString category, const QString id);
    bool boolToConfig(const QString category, const QString id, bool bBool);
    uint_t uintFromConfig(const QString category, const QString id);
    bool uintToConfig(const QString category, const QString id, uint_t i);
    QFont fontFromConfig(const QString id);
    bool fontToConfig(const QString id, const QFont font);
};

#endif // CONFIGURATION_H
