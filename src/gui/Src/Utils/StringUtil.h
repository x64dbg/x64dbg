#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <sstream>
#include <iomanip>
#include <QString>
#include <QDateTime>
#include <QLocale>
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

template<typename T>
static QString ToFloatingString(void* buffer)
{
    auto value = *(T*)buffer;
    std::stringstream wFloatingStr;
    wFloatingStr << std::setprecision(std::numeric_limits<T>::digits10) << value;
    return QString::fromStdString(wFloatingStr.str());
}

static QString ToFloatString(void* buffer)
{
    return ToFloatingString<float>(buffer);
}

static QString ToDoubleString(void* buffer)
{
    return ToFloatingString<double>(buffer);
}

QString ToLongDoubleString(void* buffer);

QString ToDateString(const QDate & date);

static QDate GetCompileDate()
{
    return QLocale("en_US").toDate(QString(__DATE__).simplified(), "MMM d yyyy");
}

#endif // STRINGUTIL_H
