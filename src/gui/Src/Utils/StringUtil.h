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

template<typename T>
static QString ToIntegralString(void* buffer)
{
    auto value = *(T*)buffer;
    return ToHexString(value);
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

static bool GetCommentFormat(duint addr, QString & comment, bool* autoComment = nullptr)
{
    comment.clear();
    char commentData[MAX_COMMENT_SIZE] = "";
    if(!DbgGetCommentAt(addr, commentData))
        return false;
    auto a = *commentData == '\1';
    if(autoComment)
        *autoComment = a;
    if(!strstr(commentData, "{"))
    {
        comment = commentData + a;
        return true;
    }
    char commentFormat[MAX_SETTING_SIZE] = "";
    if(DbgFunctions()->StringFormatInline(commentData + a, MAX_SETTING_SIZE, commentFormat))
        comment = commentFormat;
    else
        comment = commentData + a;
    return true;
}

#endif // STRINGUTIL_H
