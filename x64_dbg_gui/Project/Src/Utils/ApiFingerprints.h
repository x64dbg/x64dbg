#ifndef APIFINGERPRINTS_H
#define APIFINGERPRINTS_H

#include <QString>
#include <QList>
#include <QMap>

struct APIArgument
{
    QString Type;
    QString Name;
};

struct APIFunction
{
    QString DLLName;
    QString ReturnType;
    QString Name;
    QList<APIArgument> Arguments;
};

class ApiFingerprints
{
    static ApiFingerprints* mPtr;
    QMap<QString,QMap<QString, APIFunction>> Library;
public:
    ApiFingerprints();
    const APIFunction function(QString dllname, QString functionname) const;
    static ApiFingerprints *instance();
};

#endif // APIFINGERPRINTS_H
