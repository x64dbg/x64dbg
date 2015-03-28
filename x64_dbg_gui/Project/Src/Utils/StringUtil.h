#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <QString>
#include "NewTypes.h"

static QString AddressToString(int_t Address)
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
    sprintf_s(temp, "%16llX", Address);
#else
    sprintf_s(temp, "%08X", Address);
#endif // _WIN64

    return QString(temp);
}

#endif // STRINGUTIL_H
