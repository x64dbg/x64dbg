#ifndef APIFINGERPRINTS_H
#define APIFINGERPRINTS_H

#include <QThread>
#include <QString>
#include <QList>
#include <QMap>
#include "Bridge.h"

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
    bool invalid;

    APIFunction(){
        invalid=false;
    }

};

class ApiFingerprints
{

    static ApiFingerprints* mPtr;
    QMap<QString,QMap<QString, APIFunction>> Library;
public:
    ApiFingerprints();
    bool findFunction(QString dllname, QString functionname, const APIFunction *function);
    const APIFunction function(QString dllname, QString functionname) const;
    static ApiFingerprints *instance();


    bool findFunction(QString functionname, const APIFunction *function);
    APIFunction findFunction(QString functionname);
};

#endif // APIFINGERPRINTS_H
