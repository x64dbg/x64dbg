#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <sstream>
#include <iomanip>
#include <QString>
#include <QDateTime>
#include <QLocale>
#include "Imports.h"

inline QString ToPtrString(duint Address)
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

inline QString ToLongLongHexString(unsigned long long Value)
{
    char temp[32];
    sprintf_s(temp, "%llX", Value);
    return QString(temp);
}

inline QString ToHexString(duint Value)
{
    char temp[32];
#ifdef _WIN64
    sprintf_s(temp, "%llX", Value);
#else
    sprintf_s(temp, "%X", Value);
#endif // _WIN64
    return QString(temp);
}

inline QString ToDecString(dsint Value)
{
    char temp[32];
#ifdef _WIN64
    sprintf_s(temp, "%lld", Value);
#else
    sprintf_s(temp, "%d", Value);
#endif // _WIN64
    return QString(temp);
}

inline QString ToByteString(unsigned char Value)
{
    char temp[4];
    sprintf_s(temp, "%02X", Value);
    return QString(temp);
}

inline QString ToWordString(unsigned short Value)
{
    char temp[8];
    sprintf_s(temp, "%04X", Value);
    return QString(temp);
}

template<typename T>
inline QString ToFloatingString(const void* buffer, int precision)
{
    auto value = *(const T*)buffer;
    std::stringstream wFloatingStr;
    wFloatingStr << std::setprecision(precision) << value;
    return QString::fromStdString(wFloatingStr.str());
}

template<typename T>
inline QString ToIntegralString(const void* buffer)
{
    auto value = *(const T*)buffer;
    return ToLongLongHexString(value);
}

inline QString ToFloatString(const void* buffer, int precision = std::numeric_limits<float>::digits10)
{
    return ToFloatingString<float>(buffer, precision);
}

inline QString ToDoubleString(const void* buffer, int precision = std::numeric_limits<double>::digits10)
{
    return ToFloatingString<double>(buffer, precision);
}

QString ToLongDoubleString(const void* buffer);

QString ToDateString(const QDate & date);

QString GetDataTypeString(const void* buffer, duint size, ENCODETYPE type);

inline QDate GetCompileDate()
{
    return QLocale("en_US").toDate(QString(__DATE__).simplified(), "MMM d yyyy");
}

#ifdef _WIN64
#define ArchValue(x32value, x64value) x64value
#else
#define ArchValue(x32value, x64value) x32value
#endif //_WIN64

// Format : d:hh:mm:ss.1234567
inline QString FILETIMEToTime(const FILETIME & time)
{
    // FILETIME is in 100ns
    quint64 time100ns = (quint64)time.dwHighDateTime << 32 | (quint64)time.dwLowDateTime;
    quint64 milliseconds = time100ns / 10000;
    quint64 days = milliseconds / (1000 * 60 * 60 * 24);
    QTime qtime = QTime::fromMSecsSinceStartOfDay(milliseconds % (1000 * 60 * 60 * 24));
    if(days == 0) // 0 days
        return QString().sprintf("%02d:%02d:%02d.%07lld", qtime.hour(), qtime.minute(), qtime.second(), time100ns % 10000000);
    else
        return QString().sprintf("%lld:%02d:%02d:%02d.%07lld", days, qtime.hour(), qtime.minute(), qtime.second(), time100ns % 10000000);
}

QString FILETIMEToDate(const FILETIME & date);

bool GetCommentFormat(duint addr, QString & comment, bool* autoComment = nullptr);

QString EscapeCh(QChar ch);

#endif // STRINGUTIL_H
