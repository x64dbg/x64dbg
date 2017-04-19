#include <stdint.h>
#include "main.h"
#include "StringUtil.h"
#include "MiscUtil.h"
#include "ldconvert.h"


QString ToLongDoubleString(void* buffer)
{
    char str[32];
    ld2str(buffer, str);
    return str;
}

QString GetDataTypeString(void* buffer, duint size, ENCODETYPE type)
{
    switch(type)
    {
    case enc_byte:
        return ToIntegralString<unsigned char>(buffer);
    case enc_word:
        return ToIntegralString<unsigned short>(buffer);
    case enc_dword:
        return ToIntegralString<unsigned int>(buffer);
    case enc_fword:
        return QString(ByteReverse(QByteArray((char*)buffer, 6)).toHex());
    case enc_qword:
        return ToIntegralString<unsigned long long int>(buffer);
    case enc_tbyte:
        return QString(ByteReverse(QByteArray((char*)buffer, 10)).toHex());
    case enc_oword:
        return QString(ByteReverse(QByteArray((char*)buffer, 16)).toHex());
    case enc_mmword:
    case enc_xmmword:
    case enc_ymmword:
        return QString(QByteArray((const char*)buffer, size).toHex());
    case enc_real4:
        return ToFloatString(buffer);
    case enc_real8:
        return ToDoubleString(buffer);
    case enc_real10:
        return ToLongDoubleString(buffer);
    case enc_ascii:
        return QString::fromLocal8Bit((const char*)buffer, size);
    case enc_unicode:
        return QString::fromWCharArray((const wchar_t*)buffer, size / sizeof(wchar_t));
    default:
        return ToIntegralString<unsigned char>(buffer);
    }
}

QString ToDateString(const QDate & date)
{
    static const char* months[] =
    {
        "Jan",
        "Feb",
        "Mar",
        "Apr",
        "May",
        "Jun",
        "Jul",
        "Aug",
        "Sep",
        "Oct",
        "Nov",
        "Dec"
    };
    return QString().sprintf("%s %d %d", months[date.month() - 1], date.day(), date.year());
}

QString FILETIMEToDate(const FILETIME & date)
{
    FILETIME localdate;
    FileTimeToLocalFileTime(&date, &localdate);
    SYSTEMTIME systime;
    FileTimeToSystemTime(&localdate, &systime);
    QDate qdate = QDate(systime.wYear, systime.wMonth, systime.wDay);
    quint64 time100ns = (quint64)localdate.dwHighDateTime << 32 | (quint64)localdate.dwLowDateTime;
    time100ns %= (1000ull * 60ull * 60ull * 24ull * 10000ull);
    localdate.dwHighDateTime = time100ns >> 32;
    localdate.dwLowDateTime = time100ns & 0xFFFFFFFF;
    if(qdate != QDate::currentDate())
        return QLocale(QString(currentLocale)).toString(qdate) + FILETIMEToTime(localdate);
    else // today
        return FILETIMEToTime(localdate);
}

bool GetCommentFormat(duint addr, QString & comment, bool* autoComment)
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
