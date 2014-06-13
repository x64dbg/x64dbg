#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QFile>
#include <QString>
#include <QColor>
#include <QMap>
#include <QDebug>
#include <QObject>

#define ConfigColor(x) (Configuration::instance()->color(x))

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
    const QList<QString> ApiFingerprints();
    const QColor color(QString id);

    //public variables
    QMap<QString, QColor> Colors;
    QMap<QString, QColor> defaultColors;

signals:
    void colorsUpdated();

private:
    QColor colorFromConfig(const QString id);
    bool colorToConfig(const QString id, const QColor color);
};

#endif // CONFIGURATION_H
