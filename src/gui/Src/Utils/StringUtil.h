#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <sstream>
#include <iomanip>
#include <QString>
#include <QDateTime>
#include <QLocale>
#include "Imports.h"

const QChar HexAlphabet[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

static QString ToPtrString(duint Address)
{
    //
    // This function exists because of how QT handles
    // variables in strings.
    //
    // QString::arg():
    // ((int32)0xFFFF0000) == 0xFFFFFFFFFFFF0000 with sign extension
    //

    duint mask = 0xF;
#ifdef _WIN64
    QChar temp[16];
    for(int i = 0; i < 16; i += 2)
    {
        temp[15 - i] = HexAlphabet[(Address & mask) >> (i << 2)];
        mask <<= 4;
        temp[14 - i] = HexAlphabet[(Address & mask) >> ((i << 2) + 4)];
#else //x86
    QChar temp[8];
    for(int i = 0; i < 8; i++)
    {
        temp[7 - i] = HexAlphabet[(Address & mask) >> (i << 2)];
        mask <<= 4;
        temp[6 - i] = HexAlphabet[(Address & mask) >> ((i << 2) + 4)];
#endif //_WIN64
        mask <<= 4;
    }
#ifdef _WIN64
    return QString(temp, 16);
#else //x86
    return QString(temp, 8);
#endif //_WIN64
}

static QString ToLongLongHexString(unsigned long long Value)
{
    QChar temp[16];
    duint mask = 0xF;
    for(int i = 0; i < 16; i += 2)
    {
        temp[15 - i] = HexAlphabet[(Value & mask) >> (i << 2)];
        mask <<= 4;
        temp[14 - i] = HexAlphabet[(Value & mask) >> ((i << 2) + 4)];
        mask <<= 4;
    }
    return QString(temp, 16);
}

static QString ToByteString(unsigned char Value)
{
    QChar temp[2];
    temp[1] = HexAlphabet[Value & 0xF];
    temp[0] = HexAlphabet[(Value & 0xF0) >> 4];
    return QString(temp, 2);
}

static QString ToWordString(unsigned short Value)
{
    QChar temp[4];
    temp[3] = HexAlphabet[Value & 0xF];
    temp[2] = HexAlphabet[(Value & 0xF0) >> 4];
    temp[1] = HexAlphabet[(Value & 0xF00) >> 8];
    temp[0] = HexAlphabet[(Value & 0xF000) >> 12];
    return QString(temp, 4);
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

template<typename T>
static QString ToIntegralString(void* buffer)
{
    auto value = *(T*)buffer;
    return ToLongLongHexString(value);
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

QString GetDataTypeString(void* buffer, duint size, ENCODETYPE type);

static QDate GetCompileDate()
{
    return QLocale("en_US").toDate(QString(__DATE__).simplified(), "MMM d yyyy");
}

template<typename T>
static T & ArchValue(T & x32value, T & x64value)
{
#ifdef _WIN64
    Q_UNUSED(x32value);
    return x64value;
#else
    Q_UNUSED(x64value);
    return x32value;
#endif //_WIN64
}

// Format : d:hh:mm:ss.1234567
static QString FILETIMEToTime(const FILETIME & time)
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

#endif // STRINGUTIL_H
