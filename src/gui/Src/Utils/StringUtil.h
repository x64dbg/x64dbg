#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <QString>
#include "Imports.h"

static QString ToPtrString(duint Address)
{
    //
    // This function exists because of how QT handles
    // variables in strings.
    //
    // QString::arg():
    // ((int32)0xFFFF0000) == 0xFFFFFFFFFFFF0000 with sign extension
    //
    char temp[32];

#ifdef _WIN64
    sprintf_s(temp, "%016llX", Address);
#else
    sprintf_s(temp, "%08X", Address);
#endif // _WIN64

    return QString(temp);
}

static QString ToHexString(duint Value)
{
    char temp[32];

#ifdef _WIN64
    sprintf_s(temp, "%llX", Value);
#else
    sprintf_s(temp, "%X", Value);
#endif // _WIN64

    return QString(temp);
}

static QString ToDecString(dsint Value)
{
    char temp[32];

#ifdef _WIN64
    sprintf_s(temp, "%lld", Value);
#else
    sprintf_s(temp, "%d", Value);
#endif // _WIN64

    return QString(temp);
}

#endif // STRINGUTIL_H
