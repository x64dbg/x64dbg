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

    const QColor getColor(const QString id);
    const bool getBool(const QString category, const QString id);
    void setBool(const QString category, const QString id, const bool b);
    const uint_t getUint(const QString category, const QString id);
    void setUint(const QString category, const QString id, const uint_t i);

    //default setting maps
    QMap<QString, QColor> defaultColors;
    QMap<QString, QMap<QString, bool>> defaultBools;
    QMap<QString, QMap<QString, uint_t>> defaultUints;

    //public variables
    QMap<QString, QColor> Colors;
    QMap<QString, QMap<QString, bool>> Bools;
    QMap<QString, QMap<QString, uint_t>> Uints;

signals:
    void colorsUpdated();

private:
    QColor colorFromConfig(const QString id);
    bool colorToConfig(const QString id, const QColor color);
    bool boolFromConfig(const QString category, const QString id);
    bool boolToConfig(const QString category, const QString id, bool bBool);
    uint_t uintFromConfig(const QString category, const QString id);
    bool uintToConfig(const QString category, const QString id, uint_t i);
};

#endif // CONFIGURATION_H
