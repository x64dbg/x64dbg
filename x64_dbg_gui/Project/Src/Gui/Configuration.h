#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QFile>
#include <QString>
#include <QColor>
#include <QMap>
#include <QDebug>

class Configuration
{
    QJsonObject Config;

    QMap<QString,QColor> Colors;

    static Configuration* mPtr;
public:
    Configuration();

    static Configuration* instance();

    void readColors();
    const QColor color(QString id) const;
    void load(QString filename);
    QList<QString> ApiFingerprints();
};

#endif // CONFIGURATION_H
