#include <stdint.h>
#include "main.h"
#include "StringUtil.h"
#include "float128.h"
#include "MiscUtil.h"

dd_real pow2_fast(int exponent, int* exp10)
{
    // Significantly more precise than using std::XXX math functions
    const static dd_real dd_loge10 = dd_real("2.30258509299404568401799145468436420760110148862");
    const static dd_real dd_loge2  = dd_real("0.69314718055994530941723212145817656807550013436");
    const static dd_real dd_sqrt10 = dd_real("3.16227766016837933199889354443271853371955513932");

    // http://stackoverflow.com/questions/635183/fast-exponentiation-when-only-first-k-digits-are-required
    dd_real integerPart;
    dd_real identityCalc = std::abs(exponent) * (dd_loge2 / dd_loge10);
    dd_real identityFrac = std::modf(identityCalc, &integerPart);

    dd_real fraction = std::exp((identityFrac - 0.5) * dd_loge10) * dd_sqrt10;

    // Fraction is returned; calculate the power of 10 here
    if(exp10)
        *exp10 = std::floor(identityCalc).toInt();

    // Check for a reciprocal
    if(exponent < 0)
    {
        // Use 10 to shift the exponent into 'exp10' only
        //
        // (1 / 2**3824) == 7.26e-1152 --> (10 / 2**3824) == 7.26e-1151
        fraction = dd_real(10) / fraction;

        if(exp10)
            *exp10 = (-1 - *exp10);
    }

    return fraction;
}

QString ToLongDoubleString(void* buffer)
{
    // Assumes that "buffer" is 10 bytes at the minimum
    //
    // 80-bit floating point precision
    // https://en.wikipedia.org/wiki/Extended_precision#IEEE_754_extended_precision_formats
    const uint16_t SIGNBIT    = (1 << 15);
    const uint16_t EXP_BIAS   = (1 << 14) - 1; // 2^(n-1) - 1 = 16383
    const uint64_t HIGHBIT    = (uint64_t)1 << 63;
    const uint64_t QUIETBIT   = (uint64_t)1 << 62;

    // Don't swap endianness (data is represented in swapped endianness already)
    uint8_t bytes[10];

    for(size_t k = 0; k < 10; k++)
        bytes[k] = ((uint8_t*)buffer)[k];

    // Extract exponent and mantissa
    uint16_t exponent = *(uint16_t*)&bytes[8];
    uint64_t mantissa = *(uint64_t*)&bytes[0];

    // Extract sign
    bool sign = (exponent & SIGNBIT) != 0;
    exponent &= ~SIGNBIT;

    switch(exponent)
    {
    // If exponent zero
    case 0:
    {
        if((mantissa & HIGHBIT) == 0)
        {
            if((mantissa & QUIETBIT) == 0)
                return (sign) ? "-0.000000000000000000" : "0.000000000000000000";
        }

        // Everything else psuedo denormal
        // (−1)^s * m * 2^−16382
    }
    break;

    // If exponent all ones
    case 0x7FFF:
    {
        // if (bit 63 is zero)
        if((mantissa & HIGHBIT) == 0)
        {
            // if (bits 61-0 are zero) infinity;
            if((mantissa & (QUIETBIT - 1)) == 0)
                return (sign) ? "-INF" : "INF";

            // else psuedo_nan;
            return "NAN";
        }

        // Bit 63 is 1 at this point
        //
        // if (bit 62 is not set)
        if((mantissa & QUIETBIT) == 0)
        {
            // if (bits 61-0 are zero) infinity;
            if((mantissa & (QUIETBIT - 1)) == 0)
                return (sign) ? "-INF" : "INF";

            // else signalling_nan;
            return "NAN";
        }

        // else quiet_nan;
        return "NAN";
    }
    break;

    // Default: exponent has maximum ranges
    default:
    {
        // e+4932
        //if((exponent - EXP_BIAS) > 4932)
        //    return "INF";

        // e-4931
        //if((exponent - EXP_BIAS) < -4931)
        //    return "-INF";
    }
    break;
    }

    // Convert both numbers to 128 bit types
    dd_real dd_mantissa(mantissa);
    dd_real dd_divisor((uint64_t)1 << 63);

    // Calculate significand (m) and exponent (e)
    //
    // (-1)^s * (m / 2^63) * 2^(e - 16383)
    dd_real significand = dd_mantissa / dd_divisor;

    int exp10 = 0;
    dd_real exp = pow2_fast(exponent - EXP_BIAS, &exp10);

    significand *= exp;

    // The above multiplication can introduce an extra tens place, so remove it
    // (10.204) -> (1.0204) exp10++;
    if(std::floor(std::log10(significand)).toInt() > 0)
    {
        significand /= dd_real(10);
        exp10 += 1;
    }

    // Signed-ness
    if(sign)
        significand *= -1;

    // Print result (20 maximum digits)
    if(exp10 <= 10 && exp10 >= -4)
    {
        // -10000000000.0000000
        // 10000000000.00000000
        significand *= pown(10, exp10);

        return QString::fromStdString(significand.to_string(18 - exp10 - (int)sign, 0, std::ios_base::fixed));
    }
    else
    {
        // -1.00000000000000000e+11
        // 1.000000000000000000e+11
        QString expStr = QString().sprintf("e%+d", exp10);
        QString sigStr = QString::fromStdString(significand.to_string(23 - expStr.length() - (int)sign, 0, std::ios_base::fixed));

        return sigStr + expStr;
    }
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
        "Arp",
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
        return ToDateString(qdate) + FILETIMEToTime(localdate);
    else // today
        return FILETIMEToTime(localdate);
}
