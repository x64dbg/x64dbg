#include <stdint.h>
#include "StringUtil.h"
#include "float128.h"

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

    // Swap endianness (buffer is an array of bytes; not a true variable)
    uint8_t bytes[10];

    for(size_t k = 0; k < 10; k++)
        bytes[k] = ((uint8_t*)buffer)[10 - k - 1];

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
        if((exponent - EXP_BIAS) > 4932)
            return "INF";

        // e-4931
        if((exponent - EXP_BIAS) < -4931)
            return "-INF";
    }
    break;
    }

    // Convert both numbers to 128 bit types
    dd_real dd_mantissa((double)mantissa);
    dd_real dd_divisor((uint64_t)1 << 63);

    // Calculate significand (m) and exponent (e)
    //
    // (-1)^s * (m / 2^63) * 2^(e - 16383)
    dd_real significand = dd_mantissa / dd_divisor;
    dd_real exp = pown(2, exponent - EXP_BIAS);

    if(std::isnan(exp))
        return "NAN";

    significand *= exp;

    // Determine flags for rounding between scientific notation and standard notation (billions)
    std::ios_base::fmtflags fmt = 0;
    int digits = 19;
    int leadingDigits = 0;

    // Determine number of leading zeroes with log (log(0) is undefined)
    if(significand == dd_real(0))
        leadingDigits = 0;
    else
        leadingDigits = std::floor(std::log10(significand)).toInt() + 1;

    if(leadingDigits <= 10 && leadingDigits >= -4)
    {
        fmt = std::ios_base::fixed;
        digits = 20 - leadingDigits;
    }

    // Signed-ness
    if(sign)
        significand *= -1;

    // Print result
    return QString::fromStdString(significand.to_string(digits, 0, fmt));
}
