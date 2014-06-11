#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QFile>
#include <QString>
#include <QColor>
#include <QMap>
#include <QDebug>
#include "Bridge.h"

#define ConfigColor(x) (Configuration::instance()->color(x))

class Configuration
{
    static Configuration* mPtr;
public:
    Configuration();
    static Configuration* instance();
    void load();
    void save();
    void readColors();
    void writeColors();
    const QList<QString> ApiFingerprints();
    const QColor color(QString id);

    //public variables
    QMap<QString, QColor> Colors;
    QMap<QString, QColor> defaultColors;

private:
    QColor colorFromConfig(const QString id);
    bool colorToConfig(const QString id, const QColor color);
};

#endif // CONFIGURATION_H
