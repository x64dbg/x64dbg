/*
 * include/dd_real.h
 *
 * This work was supported by the Director, Office of Science, Division
 * of Mathematical, Information, and Computational Sciences of the
 * U.S. Department of Energy under contract number DE-AC03-76SF00098.
 *
 * Copyright (c) 2000-2007
 *
 * Double-double precision (>= 106-bit significand) floating point
 * arithmetic package based on David Bailey's Fortran-90 double-double
 * package, with some changes. See
 *
 *   http://www.nersc.gov/~dhbailey/mpdist/mpdist.html
 *
 * for the original Fortran-90 version.
 *
 * Overall structure is similar to that of Keith Brigg's C++ double-double
 * package.  See
 *
 *   http://www-epidem.plansci.cam.ac.uk/~kbriggs/doubledouble.html
 *
 * for more details.  In particular, the fix for x86 computers is borrowed
 * from his code.
 *
 * Yozo Hida
 */

#if !defined( QD_DD_REAL_H__ )
#define QD_DD_REAL_H__

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

// Begin inline code
#ifndef QD_API
#define QD_API /**/
#endif

/* If fused multiply-add is available, define to correct macro for
   using it.  It is invoked as QD_FMA(a, b, c) to compute fl(a * b + c).
   If correctly rounded multiply-add is not available (or if unsure),
   keep it undefined.*/
#ifndef QD_FMA
/* #undef QD_FMA */
#endif

/* If fused multiply-subtract is available, define to correct macro for
   using it.  It is invoked as QD_FMS(a, b, c) to compute fl(a * b - c).
   If correctly rounded multiply-add is not available (or if unsure),
   keep it undefined.*/
#ifndef QD_FMS
/* #undef QD_FMS */
#endif

/* Define this macro to be the copysign(x, y) function. */
#define QD_COPYSIGN(x, y) ::_copysign(x, y)

/* Define this macro to be the isfinite(x) function. */
#define QD_ISFINITE(x) ( ::_finite(x) != 0 )

/* Define this macro to be the isinf(x) function. */
#define QD_ISINF(x) ( ( ::_fpclass(x) & (_FPCLASS_NINF | _FPCLASS_PINF ) ) != 0 )

/* Define this macro to be the isnan(x) function. */
#define QD_ISNAN(x) ( ::_isnan(x) != 0 )

namespace qd
{
    static double const _d_inf = std::numeric_limits<double>::infinity();

    /* Computes fl(a+b) and err(a+b).  Assumes |a| >= |b|. */
    inline double quick_two_sum(double a, double b, double & err)
    {
        double s = a + b;
        err = QD_ISFINITE(s) ? b - (s - a) : 0.0;
        return s;
    }

    /* Computes fl(a-b) and err(a-b).  Assumes |a| >= |b| */
    inline double quick_two_diff(double a, double b, double & err)
    {
        double s = a - b;
        err = QD_ISFINITE(s) ? (a - s) - b : 0.0;
        return s;
    }

    /* Computes fl(a+b) and err(a+b).  */
    inline double two_sum(double a, double b, double & err)
    {
        double s = a + b;
        if(QD_ISFINITE(s))
        {
            double bb = s - a;
            err = (a - (s - bb)) + (b - bb);
        }
        else
            err = 0;
        return s;
    }

    /* Computes fl(a-b) and err(a-b).  */
    inline double two_diff(double a, double b, double & err)
    {
        double s = a - b;
        if(QD_ISFINITE(s))
        {
            double bb = s - a;
            err = (a - (s - bb)) - (b + bb);
        }
        else
            err = 0.0;
        return s;
    }

#if !defined( QD_FMS )
    /* Computes high word and lo word of a */
    inline void split(double a, double & hi, double & lo)
    {
        int const QD_BITS = (std::numeric_limits<double>::digits + 1) / 2;
        static double const QD_SPLITTER     = std::ldexp(1.0, QD_BITS) + 1.0;
        static double const QD_SPLIT_THRESH = std::ldexp((std::numeric_limits<double>::max)(), -QD_BITS - 1);

        double temp;

        if(std::abs(a) > QD_SPLIT_THRESH)
        {
            a = std::ldexp(a, -QD_BITS - 1);
            temp = QD_SPLITTER * a;
            hi = temp - (temp - a);
            lo = a - hi;
            hi = std::ldexp(hi, QD_BITS + 1);
            lo = std::ldexp(lo, QD_BITS + 1);
        }
        else
        {
            temp = QD_SPLITTER * a;
            hi = temp - (temp - a);
            lo = a - hi;
        }
    }
#endif

    /* Computes fl(a*b) and err(a*b). */
    inline double two_prod(double a, double b, double & err)
    {
        double p = a * b;
        if(QD_ISFINITE(p))
        {
#if defined( QD_FMS )
            err = QD_FMS(a, b, p);
#else
            double a_hi, a_lo, b_hi, b_lo;
            split(a, a_hi, a_lo);
            split(b, b_hi, b_lo);
            err = ((a_hi * b_hi - p) + a_hi * b_lo + a_lo * b_hi) + a_lo * b_lo;
#endif
        }
        else
            err = 0.0;
        return p;
    }

    /* Computes fl(a*a) and err(a*a).  Faster than the above method. */
    inline double two_sqr(double a, double & err)
    {
        double p = a * a;
        if(QD_ISFINITE(p))
        {
#if defined( QD_FMS )
            err = QD_FMS(a, a, p);
#else
            double hi, lo;
            split(a, hi, lo);
            err = ((hi * hi - p) + 2.0 * hi * lo) + lo * lo;
#endif
        }
        else
            err = 0.0;
        return p;
    }

    inline void renorm(double & c0, double & c1, double & c2, double & c3)
    {
        double s0, s1, s2 = 0.0, s3 = 0.0;

        if(QD_ISINF(c0)) return;

        s0 = quick_two_sum(c2, c3, c3);
        s0 = quick_two_sum(c1, s0, c2);
        c0 = quick_two_sum(c0, s0, c1);

        s0 = c0;
        s1 = c1;
        if(s1 != 0.0)
        {
            s1 = quick_two_sum(s1, c2, s2);
            if(s2 != 0.0)
                s2 = quick_two_sum(s2, c3, s3);
            else
                s1 = quick_two_sum(s1, c3, s2);
        }
        else
        {
            s0 = quick_two_sum(s0, c2, s1);
            if(s1 != 0.0)
                s1 = quick_two_sum(s1, c3, s2);
            else
                s0 = quick_two_sum(s0, c3, s1);
        }

        c0 = s0;
        c1 = s1;
        c2 = s2;
        c3 = s3;
    }

    inline void renorm(double & c0, double & c1, double & c2, double & c3, double & c4)
    {
        double s0, s1, s2 = 0.0, s3 = 0.0;

        if(QD_ISINF(c0)) return;

        s0 = quick_two_sum(c3, c4, c4);
        s0 = quick_two_sum(c2, s0, c3);
        s0 = quick_two_sum(c1, s0, c2);
        c0 = quick_two_sum(c0, s0, c1);

        s0 = c0;
        s1 = c1;

        s0 = quick_two_sum(c0, c1, s1);
        if(s1 != 0.0)
        {
            s1 = quick_two_sum(s1, c2, s2);
            if(s2 != 0.0)
            {
                s2 = quick_two_sum(s2, c3, s3);
                if(s3 != 0.0)
                    s3 += c4;
                else
                    s2 += c4;
            }
            else
            {
                s1 = quick_two_sum(s1, c3, s2);
                if(s2 != 0.0)
                    s2 = quick_two_sum(s2, c4, s3);
                else
                    s1 = quick_two_sum(s1, c4, s2);
            }
        }
        else
        {
            s0 = quick_two_sum(s0, c2, s1);
            if(s1 != 0.0)
            {
                s1 = quick_two_sum(s1, c3, s2);
                if(s2 != 0.0)
                    s2 = quick_two_sum(s2, c4, s3);
                else
                    s1 = quick_two_sum(s1, c4, s2);
            }
            else
            {
                s0 = quick_two_sum(s0, c3, s1);
                if(s1 != 0.0)
                    s1 = quick_two_sum(s1, c4, s2);
                else
                    s0 = quick_two_sum(s0, c4, s1);
            }
        }

        c0 = s0;
        c1 = s1;
        c2 = s2;
        c3 = s3;
    }

    inline void three_sum(double & a, double & b, double & c)
    {
        double t1, t2, t3;

        t1 = two_sum(a, b, t2);
        a  = two_sum(c, t1, t3);
        b  = two_sum(t2, t3, c);
    }

    inline void three_sum2(double & a, double & b, double & c)
    {
        double t1, t2, t3;
        t1 = two_sum(a, b, t2);
        a  = two_sum(c, t1, t3);
        b = t2 + t3;
    }

    /* s = quick_three_accum(a, b, c) adds c to the dd-pair (a, b).
     * If the result does not fit in two doubles, then the sum is
     * output into s and (a,b) contains the remainder.  Otherwise
     * s is zero and (a,b) contains the sum. */
    inline double quick_three_accum(double & a, double & b, double c)
    {
        double s;
        bool za, zb;

        s = two_sum(b, c, b);
        s = two_sum(a, s, a);
        za = (a != 0.0);
        zb = (b != 0.0);

        if(za && zb)
            return s;

        if(!zb)
        {
            b = a;
            a = s;
        }
        else
        {
            a = s;
        }

        return 0.0;
    }

    /* Computes the nearest integer to d. */
    inline double round(double d)
    {
        if(d == std::floor(d))
            return d;
        return std::floor(d + 0.5);
    }

    /* Computes the truncated integer. */
    inline double trunc(double d)
    {
        return (d >= 0.0) ? std::floor(d) : std::ceil(d);
    }

}
// End inline code

// Some compilers define isnan, isfinite, and isinf as macros, even for
// C++ codes, which cause havoc when overloading these functions.  We undef
// them here.
#ifdef isnan
#undef isnan
#endif

#ifdef isfinite
#undef isfinite
#endif

#ifdef isinf
#undef isinf
#endif

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif


//  class dd_real
//
//  defines a set of extended-precision floating-point values by representing
//  values as an unevaluated sum of two 'double's, subject to the constraint
//  that x[0] > 2^digits * x[1]. This normalizing constraint ensures that a
//  particular value has a unique representation.
//
//  Using IEEE 754 binary64 values as the 'double' type, this gives at least
//  106 bits of precision, with an exponent range of -968..1024. Note that
//  these are not "true" floating-point values - 1 + 1E-300 is representable
//  in this format, but is not representable in e.g. IEEE 754 binary128,
//  which has 113 bits of precision.
//
//  MANY FP PROOFS / LEMMAS ARE NOT VALID FOR THIS TYPE. USE WITH CAUTION!
//
class QD_API dd_real
{
public:
    static int const _ndigits = static_cast<int>(60206L * std::numeric_limits<double>::digits / 100000L);

    static dd_real add(double a, double b)
    {
        double e, s = qd::two_sum(a, b, e);
        return dd_real(s, e);
    }

    static dd_real sub(double a, double b)
    {
        double e, s = qd::two_sum(a, -b, e);
        return dd_real(s, e);
    }

    static dd_real mul(double a, double b)
    {
        double e, p = qd::two_prod(a, b, e);
        return dd_real(p, e);
    }

    static dd_real div(double a, double b);

    static dd_real sqr(double a)
    {
        double p2, p1 = qd::two_sqr(a, p2);
        return dd_real(p1, p2);
    }

    static dd_real sqrt(double a);

    dd_real()
    {
        x[0] = 0.0;
        x[1] = 0.0;
    }

    dd_real(double h)
    {
        x[0] = h;
        x[1] = 0.0;
    }

    dd_real(double hi, double lo)
    {
        x[0] = qd::two_sum(hi, lo, x[1]);
    }

    dd_real(int h)
    {
        x[0] = static_cast<double>(h);
        x[1] = x[0] - h;
    }

    dd_real(unsigned int h)
    {
        x[0] = static_cast<double>(h);
        x[1] = x[0] - h;
    }

    dd_real(long h)
    {
        x[0] = static_cast<double>(h);
        x[1] = x[0] - h;
    }

    dd_real(unsigned long h)
    {
        x[0] = static_cast<double>(h);
        x[1] = x[0] - h;
    }

    dd_real(long long h)
    {
        x[0] = static_cast<double>(h);
        x[1] = x[0] - h;
    }

    dd_real(unsigned long long h)
    {
        x[0] = static_cast<double>(h);
        x[1] = x[0] - h;
    }

    dd_real(std::string const & s);
    dd_real(std::wstring const & ws);

    explicit dd_real(double const* xx)
    {
        x[0] = xx[0];
        x[1] = xx[1];
    }

    double _hi() const                      { return x[0]; }
    double _lo() const                      { return x[1]; }

    double toDouble() const
    {
        return _hi() + _lo();
    }

    int toInt() const
    {
        return static_cast<int>(_hi()) + static_cast<int>(_lo());
    }

    long toLong() const
    {
        return static_cast<long>(_hi()) + static_cast<long>(_lo());
    }

    long long toLongLong() const
    {
        return static_cast<long long>(_hi()) + static_cast<long long>(_lo());
    }

    dd_real const & operator+() const
    {
        return *this;
    }

    dd_real operator-() const
    {
        return dd_real(-_hi(), -_lo());
    }

    dd_real & operator=(double a)
    {
        x[0] = a;
        x[1] = 0.0;
        return *this;
    }

    dd_real & operator=(int a)
    {
        x[0] = static_cast<double>(a);
        x[1] = a - x[0];
        return *this;
    }

    dd_real & operator=(long a)
    {
        x[0] = static_cast<double>(a);
        x[1] = a - x[0];
        return *this;
    }

    dd_real & operator=(long long a)
    {
        x[0] = static_cast<double>(a);
        x[1] = a - x[0];
        return *this;
    }

    dd_real & operator=(std::string const & s);
    dd_real & operator=(std::wstring const & s);

    dd_real & operator+=(dd_real const & b)
    {
        double s2;
        x[0] = qd::two_sum(x[0], b.x[0], s2);
        if(QD_ISFINITE(x[0]))
        {
            double t2, t1 = qd::two_sum(x[1], b.x[1], t2);
            x[1] = qd::two_sum(s2, t1, t1);
            t1 += t2;
            qd::three_sum(x[0], x[1], t1);
        }
        else
        {
            x[1] = 0.0;
        }
        return *this;
    }

    dd_real & operator+=(double b)
    {
        double s2;
        x[0] = qd::two_sum(x[0], b, s2);
        if(QD_ISFINITE(x[0]))
        {
            x[1] = qd::two_sum(x[1], s2, s2);
            qd::three_sum(x[0], x[1], s2);
        }
        else
        {
            x[1] = 0.0;
        }
        return *this;
    }

    dd_real & operator-=(dd_real const & b)
    {
        double s2;
        x[0] = qd::two_sum(x[0], -b.x[0], s2);
        if(QD_ISFINITE(x[0]))
        {
            double t2, t1 = qd::two_sum(x[1], -b.x[1], t2);
            x[1] = qd::two_sum(s2, t1, t1);
            t1 += t2;
            qd::three_sum(x[0], x[1], t1);
        }
        else
        {
            x[1] = 0.0;
        }
        return *this;
    }

    dd_real & operator-=(double b)
    {
        double s2;
        x[0] = qd::two_sum(x[0], -b, s2);
        if(QD_ISFINITE(x[0]))
        {
            x[1] = qd::two_sum(x[1], s2, s2);
            qd::three_sum(x[0], x[1], s2);
        }
        else
        {
            x[1] = 0.0;
        }
        return *this;
    }

    dd_real & operator*=(dd_real const & b)
    {
        double p[7];
        //  e powers in p = 0, 1, 1, 1, 2, 2, 2
        p[0] = qd::two_prod(x[0], b.x[0], p[1]);
        if(QD_ISFINITE(p[0]))
        {
            p[2] = qd::two_prod(x[0], b.x[1], p[4]);
            p[3] = qd::two_prod(x[1], b.x[0], p[5]);
            p[6] = x[1] * b.x[1];

            //  e powers in p = 0, 1, 2, 3, 2, 2, 2
            qd::three_sum(p[1], p[2], p[3]);

            //  e powers in p = 0, 1, 2, 3, 2, 3, 4
            p[2] += p[4] + p[5] + p[6];

            qd::three_sum(p[0], p[1], p[2]);

            x[0] = p[0];
            x[1] = p[1];
        }
        else
        {
            x[0] = p[0];
            x[1] = 0.0;
        }
        return *this;
    }

    dd_real & operator*=(double b)
    {
        double p1;
        x[0] = qd::two_prod(x[0], b, p1);
        if(QD_ISFINITE(x[0]))
        {
            x[1] *= b;
            qd::three_sum(x[0], x[1], p1);
        }
        else
        {
            x[1] = 0.0;
        }
        return *this;
    }

    dd_real & operator/=(dd_real const & b);
    dd_real & operator/=(double b);

    std::string to_string(std::streamsize precision = _ndigits, std::streamsize width = 0,  std::ios_base::fmtflags fmt = static_cast<std::ios_base::fmtflags>(0),  bool showpos = false, bool uppercase = false, char fill = ' ') const;

    bool eq(dd_real const & b) const
    {
        if(isnan() || b.isnan()) return false;
        return (_hi() == b._hi()) && (_lo() == b._lo());
    }

    bool eq(double b) const
    {
        if(isnan() || QD_ISNAN(b)) return false;
        return (_hi() == b) && (_lo() == 0.0);
    }

    bool lt(dd_real const & b) const
    {
        if(isnan() || b.isnan()) return false;
        return (_hi() < b._hi()) || ((_hi() == b._hi()) && (_lo() < b._lo()));
    }

    bool lt(double b) const
    {
        if(isnan() || QD_ISNAN(b)) return false;
        return (_hi() < b) || ((_hi() == b) && (_lo() < 0.0));
    }

    bool le(dd_real const & b) const
    {
        if(isnan() || b.isnan()) return false;
        return (_hi() < b._hi()) || ((_hi() == b._hi()) && (_lo() <= b._lo()));
    }

    bool le(double b) const
    {
        if(isnan() || QD_ISNAN(b)) return false;
        return (_hi() < b) || ((_hi() == b) && (_lo() <= 0.0));
    }

    void write(char* s, int len, int precision = _ndigits, bool showpos = false, bool uppercase = false) const;

private:
    static int read(std::string const & s, dd_real & a);
    static int read(std::wstring const & s, dd_real & a);

    bool isnan() const                      { return QD_ISNAN(x[0]) || QD_ISNAN(x[1]); }
    bool isinf() const
    {
        int y = ::_fpclass(x[0]);
        int mask = (_FPCLASS_NINF | _FPCLASS_PINF);
        y &= mask;

        return QD_ISINF(x[0]);
    }

    void to_digits(char* s, int & expn, int precision) const;

    double x[2];
};

QD_API inline dd_real operator+(dd_real const & a, dd_real const & b)
{
    dd_real retval = a;
    return retval += b;
}

QD_API inline dd_real operator+(dd_real const & a, double b)
{
    dd_real retval = a;
    return retval += b;
}

QD_API inline dd_real operator+(double a, dd_real const & b)
{
    dd_real retval = b;
    return retval += a;
}

QD_API inline dd_real operator-(dd_real const & a, dd_real const & b)
{
    dd_real retval = a;
    return retval -= b;
}

QD_API inline dd_real operator-(dd_real const & a, double b)
{
    dd_real retval = a;
    return retval -= b;
}

QD_API inline dd_real operator-(double a, dd_real const & b)
{
    dd_real retval = a;
    return retval -= b;
}

QD_API inline dd_real operator*(dd_real const & a, dd_real const & b)
{
    dd_real retval = a;
    return retval *= b;
}

QD_API inline dd_real operator*(dd_real const & a, double b)
{
    dd_real retval = a;
    return retval *= b;
}

QD_API inline dd_real operator*(double a, dd_real const & b)
{
    dd_real retval = b;
    return retval *= a;
}

QD_API inline dd_real operator/(dd_real const & a, dd_real const & b)
{
    dd_real retval = a;
    return retval /= b;
}

QD_API inline dd_real operator/(dd_real const & a, double b)
{
    dd_real retval = a;
    return retval /= b;
}

QD_API inline dd_real operator/(double a, dd_real const & b)
{
    dd_real retval = a;
    return retval /= b;
}

QD_API inline bool operator==(dd_real const & a, dd_real const & b)
{
    return a.eq(b);
}

QD_API inline bool operator==(dd_real const & a, double b)
{
    return a.eq(b);
}

QD_API inline bool operator==(double a, dd_real const & b)
{
    return b.eq(a);
}

QD_API inline bool operator!=(dd_real const & a, dd_real const & b)
{
    return !a.eq(b);
}

QD_API inline bool operator!=(dd_real const & a, double b)
{
    return !a.eq(b);
}

QD_API inline bool operator!=(double a, dd_real const & b)
{
    return !b.eq(a);
}

QD_API inline bool operator<(dd_real const & a, dd_real const & b)
{
    return a.lt(b);
}

QD_API inline bool operator<(dd_real const & a, double b)
{
    return a.lt(b);
}

QD_API inline bool operator<(double a, dd_real const & b)
{
    return dd_real(a).lt(b);
}

QD_API inline bool operator<=(dd_real const & a, dd_real const & b)
{
    return a.le(b);
}

QD_API inline bool operator<=(dd_real const & a, double b)
{
    return a.le(b);
}

QD_API inline bool operator<=(double a, dd_real const & b)
{
    return dd_real(a).le(b);
}

QD_API inline bool operator>(dd_real const & a, dd_real const & b)
{
    return b.lt(a);
}

QD_API inline bool operator>(dd_real const & a, double b)
{
    return dd_real(b).lt(a);
}

QD_API inline bool operator>(double a, dd_real const & b)
{
    return b.lt(a);
}

QD_API inline bool operator>=(dd_real const & a, dd_real const & b)
{
    return b.le(a);
}

QD_API inline bool operator>=(dd_real const & a, double b)
{
    return dd_real(b).le(a);
}

QD_API inline bool operator>=(double a, dd_real const & b)
{
    return b.le(a);
}

template<typename _Elem, typename _Tr>
QD_API inline std::basic_ostream<_Elem, _Tr> & operator<<(std::basic_ostream<_Elem, _Tr> & os, dd_real const & dd)
{
    bool showpos = (os.flags() & std::ios_base::showpos) != 0;
    bool uppercase = (os.flags() & std::ios_base::uppercase) != 0;

    return os << dd.to_string(os.precision(), os.width(), os.flags(), showpos, uppercase, os.fill());
}

template<typename _Elem, typename _Tr>
QD_API inline std::basic_istream<_Elem, _Tr> & operator>>(std::basic_istream<_Elem, _Tr> & is, dd_real & dd)
{
    std::basic_string<_Elem> str;

    is >> str;
    dd = dd_real(str.c_str());
    return s;
}

QD_API dd_real ddrand();

QD_API dd_real reciprocal(dd_real const & a);
QD_API dd_real sqr(dd_real const & a);
QD_API dd_real pown(dd_real const & a, int n);
QD_API dd_real powr(dd_real const & a, dd_real const & b);
QD_API dd_real rootn(dd_real const & a, int n);

QD_API dd_real polyeval(const dd_real* c, int n, const dd_real & x);
QD_API dd_real polyroot(const dd_real* c, int n, const dd_real & x0, int max_iter = 32, double thresh = 0.0);

QD_API void sincos(dd_real const & a, dd_real & sin_a, dd_real & cos_a);
QD_API void sincosh(dd_real const & a, dd_real & sinh_a, dd_real & cosh_a);

QD_API dd_real const & dd_pi();
QD_API dd_real const & dd_inv_pi();

QD_API dd_real const & dd_e();

QD_API dd_real const & dd_ln2();
QD_API dd_real const & dd_ln10();

QD_API dd_real const & dd_lge();
QD_API dd_real const & dd_lg10();

QD_API dd_real const & dd_log2();
QD_API dd_real const & dd_loge();


namespace std
{

    template <>
    class numeric_limits<dd_real>
    {
    public:
        typedef dd_real _Ty;

        static _Ty(min)() throw()              { return radix * (numeric_limits<double>::min)() / numeric_limits<double>::epsilon(); }
        static _Ty(max)() throw()              { return (numeric_limits<double>::max)(); }
        static _Ty lowest() throw()             { return (-(max)()); }
        static _Ty epsilon() throw()            { return numeric_limits<double>::epsilon() * numeric_limits<double>::epsilon() / radix; }
        static _Ty round_error() throw()        { return 1.0 / radix; }
        static _Ty denorm_min() throw()         { return 0.0; }
        static _Ty infinity() throw()           { return numeric_limits<double>::infinity(); }
        static _Ty quiet_NaN() throw()          { return numeric_limits<double>::quiet_NaN(); }
        static _Ty signaling_NaN() throw()      { return numeric_limits<double>::signaling_NaN(); }

        static int const digits = 2 * numeric_limits<double>::digits;
        static int const digits10 = (int)(30103L * digits / 100000L);
        static int const max_digits10 = 2 + digits10;
        static int const max_exponent = numeric_limits<double>::max_exponent;
        static int const max_exponent10 = numeric_limits<double>::max_exponent10;
        static int const min_exponent = numeric_limits<double>::min_exponent + numeric_limits<double>::digits;
        static int const min_exponent10 = (int)(30103L * min_exponent / 100000L);
        static float_denorm_style const has_denorm = denorm_absent;
        static bool const has_denorm_loss = false;
        static bool const has_infinity = true;
        static bool const has_quiet_NaN = true;
        static bool const has_signaling_NaN = true;
        static bool const is_bounded = true;
        static bool const is_exact = false;
        static bool const is_iec559 = false;
        static bool const is_integer = false;
        static bool const is_modulo = false;
        static bool const is_signed = true;
        static bool const is_specialized = true;
        static bool const tinyness_before = true;
        static bool const traps = true;
        static float_round_style const round_style = round_to_nearest;
        static int const radix = numeric_limits<double>::radix;
    };

#if !defined( FP_INFINITE )
#define FP_ZERO                                 0
#define FP_SUBNORMAL                            1
#define FP_NORMAL                               2
#define FP_INFINITE                             3
#define FP_NAN                                  4
#endif

#if !defined( FP_ILOGB0 )
#define FP_ILOGB0                               ( INT_MIN )
#define FP_ILOGBNAN                             ( INT_MIN + 1 )
#endif

    //
    //
    //  classification functions
    //
    QD_API int fpclassify(dd_real const & a);
    QD_API bool isfinite(dd_real const & a);
    QD_API bool isinf(dd_real const & a);
    QD_API bool isnan(dd_real const & a);
    QD_API bool isnormal(dd_real const & a);
    QD_API bool signbit(dd_real const & a);

    //
    //  floating-point manipulation functions
    //
    QD_API dd_real copysign(dd_real const & a, dd_real const & b);
    QD_API dd_real frexp(dd_real const & a, int* pexp);
    QD_API int ilogb(dd_real const & a);
    QD_API dd_real ldexp(dd_real const & a, int exp);
    QD_API dd_real logb(dd_real const & a);
    QD_API dd_real modf(dd_real const & a, dd_real* b);
    QD_API dd_real scalbn(dd_real const & a, int exp);
    QD_API dd_real scalbln(dd_real const & a, long exp);

    //
    //  rounding and remainder functions
    //
    QD_API dd_real ceil(dd_real const & a);
    QD_API dd_real floor(dd_real const & a);
    QD_API dd_real fmod(dd_real const & a, dd_real const & b);
    QD_API dd_real round(dd_real const & a);
    QD_API dd_real trunc(dd_real const & a);

    //
    //  minimum, maximum, difference functions
    //
    QD_API dd_real fdim(dd_real const & a, dd_real const & b);
    QD_API dd_real fmax(dd_real const & a, dd_real const & b);
    QD_API dd_real fmin(dd_real const & a, dd_real const & b);

    //
    //  Other functions
    //
    QD_API dd_real abs(dd_real const & a);
    QD_API dd_real fabs(dd_real const & a);
    QD_API dd_real Fma(dd_real const & a, dd_real const & b, dd_real const & c);

    //
    //  trigonometric functions
    //
    QD_API dd_real acos(dd_real const & a);
    QD_API dd_real asin(dd_real const & a);
    QD_API dd_real atan(dd_real const & a);
    QD_API dd_real atan2(dd_real const & y, dd_real const & x);
    QD_API dd_real cos(dd_real const & a);
    QD_API dd_real sin(dd_real const & a);
    QD_API dd_real tan(dd_real const & a);

    //
    //  hyperbolic functions
    //
    QD_API dd_real acosh(dd_real const & a);
    QD_API dd_real asinh(dd_real const & a);
    QD_API dd_real atanh(dd_real const & a);
    QD_API dd_real cosh(dd_real const & a);
    QD_API dd_real sinh(dd_real const & a);
    QD_API dd_real tanh(dd_real const & a);

    //
    //  exponential and logarithmic functions
    //
    QD_API dd_real exp(dd_real const & a);
    QD_API dd_real exp2(dd_real const & a);
    QD_API dd_real expm1(dd_real const & a);
    QD_API dd_real log(dd_real const & a);
    QD_API dd_real log1p(dd_real const & a);
    QD_API dd_real log2(dd_real const & a);
    QD_API dd_real log10(dd_real const & a);

    //
    //  power functions
    //
    QD_API dd_real cbrt(dd_real const & a);
    QD_API dd_real hypot(dd_real const & x, dd_real const & y);
    QD_API dd_real pow(dd_real const & a, dd_real const & b);
    QD_API dd_real sqrt(dd_real const & a);

}

#endif /* _QD_DD_REAL_H */
