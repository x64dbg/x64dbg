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

    // Convert to pointer of array of bytes
    uint8_t* bytes = (uint8_t*)buffer;

    // Extract exponent and mantissa
    uint16_t exponent = *(uint16_t*)&bytes[8];
    uint64_t mantissa = *(uint64_t*)&bytes[0];

    // Extract sign
    bool sign = (exponent & SIGNBIT) != 0;
    exponent &= ~SIGNBIT;

    switch(exponent)
    {
    // Default: exponent is ignored
    default:
        break;

    // If exponent zero
    case 0:
    {
        if((mantissa & HIGHBIT) == 0)
        {
            if((mantissa & QUIETBIT) == 0)
                return (sign) ? "-0.0" : "0.0";
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
    }

    // Convert both numbers to 128 bit types
    dd_real dd_mantissa((double)mantissa);
    dd_real dd_divisor((uint64_t)1 << 63);

    // Calculate significand (m) and exponent (e)
    //
    // (-1)^s * (m / 2^63) * 2^(e - 16383)
    dd_real significand = dd_mantissa / dd_divisor;
    dd_real exp = pown(2, exponent - EXP_BIAS);

    // Calculate final result with sign
    significand *= exp;

    if(sign)
        significand *= -1;

    // Print result
    return QString::fromStdString(significand.to_string(16));
}
