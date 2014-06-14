#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QFile>
#include <QString>
#include <QColor>
#include <QMap>
#include <QDebug>
#include <QObject>
#include <QMessageBox>

#define ConfigColor(x) (Configuration::instance()->getColor(x))
#define ConfigBool(x,y) (Configuration::instance()->getBool(x,y))

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
    const QList<QString> ApiFingerprints();
    const QColor getColor(const QString id);
    const bool getBool(const QString category, const QString id);

    //default setting maps
    QMap<QString, QColor> defaultColors;
    QMap<QString, QMap<QString, bool>> defaultBools;

    //public variables
    QMap<QString, QColor> Colors;
    QMap<QString, QMap<QString, bool>> Bools;

signals:
    void colorsUpdated();

private:
    QColor colorFromConfig(const QString id);
    bool colorToConfig(const QString id, const QColor color);
    bool boolFromConfig(const QString category, const QString id);
    bool boolToConfig(const QString category, const QString id, bool bBool);
};

#endif // CONFIGURATION_H
