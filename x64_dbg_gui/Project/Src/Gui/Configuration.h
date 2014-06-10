#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QFile>
#include <QString>
#include <QColor>
#include <QMap>
#include <QDebug>
#include "Bridge.h"

class Configuration
{
    QMap<QString,QColor> Colors;
    QMap<QString,QColor> defaultColors;

    static Configuration* mPtr;
public:
    Configuration();
    static Configuration* instance();
    void load();
    void readColors();
    const QList<QString> ApiFingerprints();
    const QColor color(QString id);

private:
    QColor colorFromConfig(const QString id);
    bool colorToConfig(const QString id, const QColor color);
};

#endif // CONFIGURATION_H
