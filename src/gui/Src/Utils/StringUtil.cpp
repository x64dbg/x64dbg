#include <stdint.h>
#include "main.h"
#include "StringUtil.h"
#include "MiscUtil.h"
#include "ldconvert.h"
#include "Configuration.h"

QString ToLongDoubleString(const void* buffer)
{
    char str[32];
    ld2str(buffer, str);
    return str;
}

QString EscapeCh(QChar ch)
{
    switch(ch.unicode())
    {
    case '\0':
        return "\\0";
    case '\t':
        return "\\t";
    case '\f':
        return "\\f";
    case '\v':
        return "\\v";
    case '\n':
        return "\\n";
    case '\r':
        return "\\r";
    case '\\':
        return "\\\\";
    case '\"':
        return "\\\"";
    case '\a':
        return "\\a";
    case '\b':
        return "\\b";
    default:
        return QString(1, ch);
    }
}

QString fillValue(const char* value, int valsize, bool bFpuRegistersLittleEndian)
{
    if(bFpuRegistersLittleEndian)
        return QString(QByteArray(value, valsize).toHex()).toUpper();
    else // Big Endian
        return QString(ByteReverse(QByteArray(value, valsize)).toHex()).toUpper();
}

QString composeRegTextXMM(const char* value, int mode)
{
    bool bFpuRegistersLittleEndian = ConfigBool("Gui", "FpuRegistersLittleEndian");
    QString valueText;
    switch(mode)
    {
    default:
    case 0:
    {
        valueText = fillValue(value, 16, bFpuRegistersLittleEndian);
    }
    break;
    case 2:
    {
        const double* dbl_values = reinterpret_cast<const double*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = ToDoubleString(&dbl_values[0]) + ' ' + ToDoubleString(&dbl_values[1]);
        else // Big Endian
            valueText = ToDoubleString(&dbl_values[1]) + ' ' + ToDoubleString(&dbl_values[0]);
    }
    break;
    case 1:
    {
        const float* flt_values = reinterpret_cast<const float*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = ToFloatString(&flt_values[0]) + ' ' + ToFloatString(&flt_values[1]) + ' '
                        + ToFloatString(&flt_values[2]) + ' ' + ToFloatString(&flt_values[3]);
        else // Big Endian
            valueText = ToFloatString(&flt_values[3]) + ' ' + ToFloatString(&flt_values[2]) + ' '
                        + ToFloatString(&flt_values[1]) + ' ' + ToFloatString(&flt_values[0]);
    }
    break;
    case 9:
    {
        if(bFpuRegistersLittleEndian)
            valueText = fillValue(value) + ' ' + fillValue(value + 1 * 2) + ' ' + fillValue(value + 2 * 2) + ' ' + fillValue(value + 3 * 2)
                        + ' ' + fillValue(value + 4 * 2) + ' ' + fillValue(value + 5 * 2) + ' ' + fillValue(value + 6 * 2) + ' ' + fillValue(value + 7 * 2);
        else // Big Endian
            valueText = fillValue(value + 7 * 2) + ' ' + fillValue(value + 6 * 2) + ' ' + fillValue(value + 5 * 2) + ' ' + fillValue(value + 4 * 2)
                        + ' ' + fillValue(value + 3 * 2) + ' ' + fillValue(value + 2 * 2) + ' ' + fillValue(value + 1 * 2) + ' ' + fillValue(value);
    }
    break;
    case 3:
    {
        const short* sword_values = reinterpret_cast<const short*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = QString::number(sword_values[0]) + ' ' + QString::number(sword_values[1]) + ' ' + QString::number(sword_values[2]) + ' ' + QString::number(sword_values[3])
                        + ' ' + QString::number(sword_values[4]) + ' ' + QString::number(sword_values[5]) + ' ' + QString::number(sword_values[6]) + ' ' + QString::number(sword_values[7]);
        else // Big Endian
            valueText = QString::number(sword_values[7]) + ' ' + QString::number(sword_values[6]) + ' ' + QString::number(sword_values[5]) + ' ' + QString::number(sword_values[4])
                        + ' ' + QString::number(sword_values[3]) + ' ' + QString::number(sword_values[2]) + ' ' + QString::number(sword_values[1]) + ' ' + QString::number(sword_values[0]);
    }
    break;
    case 6:
    {
        const unsigned short* uword_values = reinterpret_cast<const unsigned short*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = QString::number(uword_values[0]) + ' ' + QString::number(uword_values[1]) + ' ' + QString::number(uword_values[2]) + ' ' + QString::number(uword_values[3])
                        + ' ' + QString::number(uword_values[4]) + ' ' + QString::number(uword_values[5]) + ' ' + QString::number(uword_values[6]) + ' ' + QString::number(uword_values[7]);
        else // Big Endian
            valueText = QString::number(uword_values[7]) + ' ' + QString::number(uword_values[6]) + ' ' + QString::number(uword_values[5]) + ' ' + QString::number(uword_values[4])
                        + ' ' + QString::number(uword_values[3]) + ' ' + QString::number(uword_values[2]) + ' ' + QString::number(uword_values[1]) + ' ' + QString::number(uword_values[0]);
    }
    break;
    case 10:
    {
        if(bFpuRegistersLittleEndian)
            valueText = fillValue(value, 4) + ' ' +  fillValue(value + 1 * 4, 4) + ' ' +  fillValue(value + 2 * 4, 4) + ' ' +  fillValue(value + 3 * 4, 4);
        else // Big Endian
            valueText = fillValue(value + 3 * 4, 4) + ' ' +  fillValue(value + 2 * 4, 4) + ' ' +  fillValue(value + 1 * 4, 4) + ' ' +  fillValue(value, 4);
    }
    break;
    case 4:
    {
        const int* sdword_values = reinterpret_cast<const int*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = QString::number(sdword_values[0]) + ' ' + QString::number(sdword_values[1]) + ' ' + QString::number(sdword_values[2]) + ' ' + QString::number(sdword_values[3]);
        else // Big Endian
            valueText = QString::number(sdword_values[3]) + ' ' + QString::number(sdword_values[2]) + ' ' + QString::number(sdword_values[1]) + ' ' + QString::number(sdword_values[0]);
    }
    break;
    case 7:
    {
        const unsigned int* udword_values = reinterpret_cast<const unsigned int*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = QString::number(udword_values[0]) + ' ' + QString::number(udword_values[1]) + ' ' + QString::number(udword_values[2]) + ' ' + QString::number(udword_values[3]);
        else // Big Endian
            valueText = QString::number(udword_values[3]) + ' ' + QString::number(udword_values[2]) + ' ' + QString::number(udword_values[1]) + ' ' + QString::number(udword_values[0]);
    }
    break;
    case 11:
    {
        if(bFpuRegistersLittleEndian)
            valueText = fillValue(value, 8) + ' ' + fillValue(value + 8, 8);
        else // Big Endian
            valueText = fillValue(value + 8, 8) + ' ' + fillValue(value, 8);
    }
    break;
    case 5:
    {
        const long long* sqword_values = reinterpret_cast<const long long*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = QString::number(sqword_values[0]) + ' ' + QString::number(sqword_values[1]);
        else // Big Endian
            valueText = QString::number(sqword_values[1]) + ' ' + QString::number(sqword_values[0]);
    }
    break;
    case 8:
    {
        const unsigned long long* uqword_values = reinterpret_cast<const unsigned long long*>(value);
        if(bFpuRegistersLittleEndian)
            valueText = QString::number(uqword_values[0]) + ' ' + QString::number(uqword_values[1]);
        else // Big Endian
            valueText = QString::number(uqword_values[1]) + ' ' + QString::number(uqword_values[0]);
    }
    break;
    }
    return valueText;
}

QString composeRegTextYMM(const char* value, int mode)
{
    bool bFpuRegistersLittleEndian = ConfigBool("Gui", "FpuRegistersLittleEndian");
    if(mode == 0)
        return fillValue(value, 32, bFpuRegistersLittleEndian);
    else if(bFpuRegistersLittleEndian)
        return composeRegTextXMM(value, mode) + ' ' + composeRegTextXMM(value + 16, mode);
    else
        return composeRegTextXMM(value + 16, mode) + ' ' + composeRegTextXMM(value, mode);
}

QString composeRegTextZMM(const char* value, int mode)
{
    bool bFpuRegistersLittleEndian = ConfigBool("Gui", "FpuRegistersLittleEndian");
    if(mode == 0)
        return fillValue(value, 64, bFpuRegistersLittleEndian);
    else if(bFpuRegistersLittleEndian)
        return composeRegTextXMM(value, mode) + ' ' + composeRegTextXMM(value + 16, mode) + ' ' + composeRegTextXMM(value + 32, mode) + ' ' + composeRegTextXMM(value + 48, mode);
    else
        return composeRegTextXMM(value + 48, mode) + ' ' + composeRegTextXMM(value + 32, mode) + ' ' + composeRegTextXMM(value + 16, mode) + ' ' + composeRegTextXMM(value, mode);
}

QString GetDataTypeString(const void* buffer, duint size, ENCODETYPE type)
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
        return QString(ByteReverse(QByteArray((const char*)buffer, 6)).toHex());
    case enc_qword:
        return ToIntegralString<unsigned long long int>(buffer);
    case enc_tbyte:
        return QString(ByteReverse(QByteArray((const char*)buffer, 10)).toHex());
    case enc_oword:
        return QString(ByteReverse(QByteArray((const char*)buffer, 16)).toHex());
    case enc_mmword:
        return QString(QByteArray((const char*)buffer, size).toHex());
    case enc_xmmword:
        return composeRegTextXMM((const char*)buffer, ConfigUint("Gui", "SIMDRegistersDisplayMode"));
    case enc_ymmword:
        return composeRegTextYMM((const char*)buffer, ConfigUint("Gui", "SIMDRegistersDisplayMode"));
    case enc_real4:
        return ToFloatString(buffer);
    case enc_real8:
        return ToDoubleString(buffer);
    case enc_real10:
        return ToLongDoubleString(buffer);
    case enc_ascii:
        return EscapeCh(*(const char*)buffer);
    case enc_unicode:
        return EscapeCh(*(const wchar_t*)buffer);
    default:
        return ToIntegralString<unsigned char>(buffer);
    }
}

QString isoDateTime()
{
    auto now = QDateTime::currentDateTime();
    return QString().sprintf("%04d%02d%02d-%02d%02d%02d",
                             now.date().year(),
                             now.date().month(),
                             now.date().day(),
                             now.time().hour(),
                             now.time().minute(),
                             now.time().second()
                            );
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
        return QLocale(QString(gCurrentLocale)).toString(qdate) + FILETIMEToTime(localdate);
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

QString DbgCmdEscape(QString argument)
{
    // TODO: implement this properly
    argument.replace("\"", "\\\"");
    argument.replace("{", "\\{");

    return argument;
}
