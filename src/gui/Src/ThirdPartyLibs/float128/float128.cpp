/*
 * src/dd_real.cc
 *
 * This work was supported by the Director, Office of Science, Division
 * of Mathematical, Information, and Computational Sciences of the
 * U.S. Department of Energy under contract number DE-AC03-76SF00098.
 *
 * Copyright (c) 2000-2007
 *
 * Contains implementation of non-inlined functions of double-double
 * package.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 *
 * Functions copied substantially from fdlibm:
 *  __kernel_rem_piby2() ==> rem_piby2()
 */
// Defined for this file only
#define _CRT_SECURE_NO_WARNINGS

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <iomanip>
#include <iostream>
#include <string>

#include "float128.h"

namespace
{
    double const _eps = std::ldexp(1.0, -104);

    dd_real const _zero = dd_real(0.0);
    dd_real const _one  = dd_real(1.0);
    dd_real const _ten  = dd_real(10.0);

    dd_real const _tenth = dd_real("0.1");
    dd_real const _third = dd_real("0.333333333333333333333333333333333333");

    dd_real const _2pi  = dd_real("6.283185307179586476925286766559005768");
    dd_real const _pi   = dd_real("3.141592653589793238462643383279502884");
    dd_real const _pi2  = dd_real("1.570796326794896619231321691639751442");
    dd_real const _pi4  = dd_real("0.785398163397448309615660845819875721");
    dd_real const _3pi4 = _pi2 + _pi4;

    dd_real const _e     = dd_real("2.718281828459045235360287471352662498");

    dd_real const _ln2  = dd_real("0.693147180559945309417232121458176568");
    dd_real const _ln10 = dd_real("2.302585092994045684017991454684364208");

    dd_real const _lge  = dd_real("1.442695040888963407359924681001892137");
    dd_real const _lg10 = dd_real("3.321928094887362347870319429489390176");

    dd_real const _log2 = dd_real("0.301029995663981195213738894724493027");
    dd_real const _loge = dd_real("0.434294481903251827651128918916605082");

    dd_real const _sqrt2 = dd_real("1.414213562373095048801688724209698079");

    dd_real const _inv_pi    = dd_real("0.318309886183790671537767526745028724");
    dd_real const _inv_pi2   = dd_real("0.636619772367581343075535053490057448");
    dd_real const _inv_e     = dd_real("0.367879441171442321595523770161460867");
    dd_real const _inv_sqrt2 = dd_real("0.707106781186547524400844362104849039");

    double a_lge[] =
    {
        std::ldexp((double)0x171547652B82FEULL, -52),
        std::ldexp((double)0x02EEFA1FFB41A4ULL, -105),
        std::ldexp((double)0x0E9F4475ABC000ULL, -158)
    };

    dd_real const inv_int[] =
    {
        dd_real(std::numeric_limits<dd_real>::infinity()),         //  1/0
        dd_real("1.0"),                                              //  1/1
        dd_real("0.5"),                                              //  1/2
        dd_real("0.3333333333333333333333333333333333333"),          //  1/3
        dd_real("0.25"),                                             //  1/4
        dd_real("0.2"),                                              //  1/5
        dd_real("0.1666666666666666666666666666666666667"),          //  1/6
        dd_real("0.1428571428571428571428571428571428571"),          //  1/7
        dd_real("0.125"),                                            //  1/8
        dd_real("0.1111111111111111111111111111111111111"),          //  1/9
        dd_real("0.1"),                                              //  1/10
        dd_real("0.0909090909090909090909090909090909091"),          //  1/11
        dd_real("0.0833333333333333333333333333333333333"),          //  1/12
        dd_real("0.0769230769230769230769230769230769231"),          //  1/13
        dd_real("0.0714285714285714285714285714285714286"),          //  1/14
        dd_real("0.0666666666666666666666666666666666667"),          //  1/15
        dd_real("0.0625"),                                       //  1/16
        dd_real("0.0588235294117647058823529411764705882"),          //  1/17
        dd_real("0.0555555555555555555555555555555555556"),          //  1/18
        dd_real("0.0526315789473684210526315789473684211"),          //  1/19
        dd_real("0.05"),                                             //  1/20
        dd_real("0.0476190476190476190476190476190476190"),          //  1/21
        dd_real("0.0454545454545454545454545454545454545"),          //  1/22
        dd_real("0.0434782608695652173913043478260869565"),          //  1/23
        dd_real("0.0416666666666666666666666666666666667"),          //  1/24
        dd_real("0.04"),                                             //  1/25
        dd_real("0.0384615384615384615384615384615384615"),          //  1/26
        dd_real("0.0370370370370370370370370370370370370"),          //  1/27
        dd_real("0.0357142857142857142857142857142857143"),          //  1/28
        dd_real("0.0344827586206896551724137931034482759"),          //  1/29
        dd_real("0.0333333333333333333333333333333333333"),          //  1/30
        dd_real("0.0322580645161290322580645161290322581"),          //  1/31
        dd_real("0.03125"),                                          //  1/32
        dd_real("0.0303030303030303030303030303030303030"),          //  1/33
        dd_real("0.0294117647058823529411764705882352941"),          //  1/34
        dd_real("0.0285714285714285714285714285714285714"),          //  1/35
        dd_real("0.0277777777777777777777777777777777778"),          //  1/36
        dd_real("0.0270270270270270270270270270270270270"),          //  1/37
        dd_real("0.0263157894736842105263157894736842105"),          //  1/38
        dd_real("0.0256410256410256410256410256410256410"),          //  1/39
        dd_real("0.025"),                                            //  1/40
        dd_real("0.0243902439024390243902439024390243902")       //  1/41
    };

    dd_real const inv_fact[] =
    {
        dd_real("1.0"),                                         //  1/0!
        dd_real("1.0"),                                         //  1/1!
        dd_real("0.5"),                                         //  1/2!
        dd_real("1.66666666666666666666666666666666667E-1"),    //  1/3!
        dd_real("4.16666666666666666666666666666666667E-2"),    //  1/4!
        dd_real("8.33333333333333333333333333333333333E-3"),    //  1/5!
        dd_real("1.38888888888888888888888888888888889E-3"),    //  1/6!
        dd_real("1.98412698412698412698412698412698413E-4"),    //  1/7!
        dd_real("2.48015873015873015873015873015873016E-5"),    //  1/8!
        dd_real("2.75573192239858906525573192239858907E-6"),    //  1/9!
        dd_real("2.75573192239858906525573192239858907E-7"),    //  1/10!
        dd_real("2.50521083854417187750521083854417188E-8"),    //  1/11!
        dd_real("2.08767569878680989792100903212014323E-9"),    //  1/12!
        dd_real("1.60590438368216145993923771701549479E-10"),   //  1/13!
        dd_real("1.14707455977297247138516979786821057E-11"),   //  1/14!
        dd_real("7.64716373181981647590113198578807044E-13"),   //  1/15!
        dd_real("4.77947733238738529743820749111754403E-14"),   //  1/16!
        dd_real("2.81145725434552076319894558301032002E-15"),   //  1/17!
        dd_real("1.56192069685862264622163643500573334E-16"),   //  1/18!
        dd_real("8.22063524662432971695598123687228075E-18"),   //  1/19!
        dd_real("4.11031762331216485847799061843614037E-19"),   //  1/20!
        dd_real("1.95729410633912612308475743735054304E-20"),   //  1/21!
        dd_real("8.89679139245057328674889744250246834E-22"),   //  1/22!
        dd_real("3.86817017063068403771691193152281232E-23"),   //  1/23!
        dd_real("1.61173757109611834904871330480117180E-24"),   //  1/24!
        dd_real("6.44695028438447339619485321920468721E-26"),   //  1/25!
        dd_real("2.47959626322479746007494354584795662E-27"),   //  1/26!
        dd_real("9.18368986379554614842571683647391340E-29"),   //  1/27!
        dd_real("3.27988923706983791015204172731211193E-30"),   //  1/28!
        dd_real("1.13099628864477169315587645769383170E-31"),   //  1/29!
        dd_real("3.76998762881590564385292152564610566E-33"),   //  1/30!
        dd_real("1.21612504155351794962997468569229215E-34"),   //  1/31!
        dd_real("3.80039075485474359259367089278841297E-36"),   //  1/32!
        dd_real("1.15163356207719502805868814932982211E-37"),   //  1/33!
    };

    // quad-double + double-double
    //
    void qd_add(double const* a, dd_real const & b, double* s)
    {
        double t[5];
        s[0] = qd::two_sum(a[0], b._hi(), t[0]);            //  s0 - O( 1 ); t0 - O( e )
        s[1] = qd::two_sum(a[1], b._lo(), t[1]);        //  s1 - O( e ); t1 - O( e^2 )

        s[1] = qd::two_sum(s[1], t[0], t[0]);               //  s1 - O( e ); t0 - O( e^2 )

        s[2] = a[2];                                                //  s2 - O( e^2 )
        qd::three_sum(s[2], t[0], t[1]);                                        //  s2 - O( e^2 ); t0 - O( e^3 ); t1 = O( e^4 )

        s[3] = qd::two_sum(a[3], t[0], t[0]);               //  s3 - O( e^3 ); t0 - O( e^4 )
        t[0] += t[1];                                                       //  fl( t0 + t1 ) - accuracy less important

        qd::renorm(s[0], s[1], s[2], s[3], t[0]);
    }

    // quad-double = double-double * double-double
    //
    void qd_mul(dd_real const & a, dd_real const & b, double* p)
    {
        double p4, p5, p6, p7;

        //  powers of e - 0, 1, 1, 1, 2, 2, 2, 3
        p[0] = qd::two_prod(a._hi(), b._hi(), p[1]);
        if(QD_ISFINITE(p[0]))
        {
            p[2] = qd::two_prod(a._hi(), b._lo(), p4);
            p[3] = qd::two_prod(a._lo(), b._hi(), p5);
            p6 = qd::two_prod(a._lo(), b._lo(), p7);

            //  powers of e - 0, 1, 2, 3, 2, 2, 2, 3
            qd::three_sum(p[1], p[2], p[3]);

            //  powers of e - 0, 1, 2, 3, 2, 3, 4, 3
            qd::three_sum(p4, p5, p6);

            //  powers of e - 0, 1, 2, 3, 3, 3, 4, 3
            p[2] = qd::two_sum(p[2], p4, p4);

            //  powers of e - 0, 1, 2, 3, 4, 5, 4, 3
            qd::three_sum(p[3], p4, p5);

            //  powers of e - 0, 1, 2, 3, 4, 5, 4, 4
            p[3] = qd::two_sum(p[3], p7, p7);

            p4 += (p6 + p7);

            qd::renorm(p[0], p[1], p[2], p[3], p4);
        }
        else
        {
            p[1] = p[2] = p[3] = 0.0;
        }
    }

    // triple-double = double-double * triple-double
    //
    void td_mul(dd_real const & a, double const* b, double* r)
    {
        double p[11];

        //  powers of e - 0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3
        p[0]  = qd::two_prod(a._hi(), b[0], p[1]);          //  0, 1
        if(QD_ISFINITE(p[0]))
        {
            p[2] = qd::two_prod(a._hi(), b[1], p[4]);       //  1, 2
            p[3] = qd::two_prod(a._lo(), b[0], p[5]);       //  1, 2

            p[6] = qd::two_prod(a._hi(), b[2], p[8]);       //  2, 3
            p[7] = qd::two_prod(a._lo(), b[1], p[9]);       //  2, 3

            p[10] = a._lo() * b[2];                         //  3

            //  powers of e - 0, 1, 2, 3, 2, 2, 2, 2, 3, 3, 3
            qd::three_sum(p[1], p[2], p[3]);

            //  powers of e - 0, 1, 2, 3, 2, 3, 4, 2, 3, 3, 3
            qd::three_sum(p[4], p[5], p[6]);

            //  powers of e - 0, 1, 2, 3, 3, 3, 4, 4, 3, 3, 3
            qd::three_sum(p[2], p[4], p[7]);

            p[3] += p[4] + p[5] + p[8] + p[9] + p[10];

            qd::renorm(p[0], p[1], p[2], p[3]);
        }
        else
        {
            p[1] = p[2] = 0.0;
        }
        r[0] = p[0];
        r[1] = p[1];
        r[2] = p[2];
    }

    /* This routine is called whenever a fatal error occurs. */
    void error(std::string const & msg)
    {
        std::cerr << "ERROR " << msg << std::endl;
    }

    void error(std::wstring const & msg)
    {
        std::wcerr << L"ERROR " << msg << std::endl;
    }

    bool iszero(dd_real const & a)
    {
        return a._hi() == 0.0;
    }

    bool isone(dd_real const & a)
    {
        return (a._hi() == 1.0) && (a._lo() == 0.0);
    }

    double round(double d)
    {
        auto signD = QD_COPYSIGN(1.0, d);

        return signD < 0.0 ? std::ceil(d - 0.5) : std::floor(d + 0.5);
    }

    double scalbn(double a, int exp)
    {
        static_assert(std::numeric_limits<double>::radix == 2, "CONFIGURATION: double radix must be 2!");

        return std::ldexp(a, exp);
    }

    /* @(#)k_rem_pio2.c 1.3 95/01/18 */
    /*
     * ====================================================
     * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
     *
     * Developed at SunSoft, a Sun Microsystems, Inc. business.
     * Permission to use, copy, modify, and distribute this
     * software is freely granted, provided that this notice
     * is preserved.
     * ====================================================
     */

    /*
     * __kernel_rem_pio2(x,y,e0,nx,prec,ipio2)
     * double x[],y[]; int e0,nx,prec; int ipio2[];
     *
     * __kernel_rem_pio2 return the last three digits of N with
     *      y = x - N*pi/2
     * so that |y| < pi/2.
     *
     * The method is to compute the integer (mod 8) and fraction parts of
     * (2/pi)*x without doing the full multiplication. In general we
     * skip the part of the product that are known to be a huge integer (
     * more accurately, = 0 mod 8 ). Thus the number of operations are
     * independent of the exponent of the input.
     *
     * (2/pi) is represented by an array of 24-bit integers in ipio2[].
     *
     * Input parameters:
     *  x[] The input value (must be positive) is broken into nx
     *      pieces of 24-bit integers in double precision format.
     *      x[i] will be the i-th 24 bit of x. The scaled exponent
     *      of x[0] is given in input parameter e0 (i.e., x[0]*2^e0
     *      match x's up to 24 bits.
     *
     *      Example of breaking a double positive z into x[0]+x[1]+x[2]:
     *          e0 = ilogb(z)-23
     *          z  = scalbn(z,-e0)
     *      for i = 0,1,2
     *          x[i] = floor(z)
     *          z    = (z-x[i])*2**24
     *
     *
     *  y[] output result in an array of double precision numbers.
     *      The dimension of y[] is:
     *          24-bit  precision   1
     *          53-bit  precision   2
     *          64-bit  precision   2
     *          113-bit precision   3
     *      The actual value is the sum of them. Thus for 113-bit
     *      precison, one may have to do something like:
     *
     *      long double t,w,r_head, r_tail;
     *      t = (long double)y[2] + (long double)y[1];
     *      w = (long double)y[0];
     *      r_head = t+w;
     *      r_tail = w - (r_head - t);
     *
     *  e0  The exponent of x[0]
     *
     *  nx  dimension of x[]
     *
     *      prec    an integer indicating the precision:
     *          0   24  bits (single)
     *          1   53  bits (double)
     *          2   64  bits (extended)
     *          3   113 bits (quad)
     *
     *  ipio2[]
     *      integer array, contains the (24*i)-th to (24*i+23)-th
     *      bit of 2/pi after binary point. The corresponding
     *      floating value is
     *
     *          ipio2[i] * 2^(-24(i+1)).
     *
     * External function:
     *  double scalbn(), floor();
     *
     *
     * Here is the description of some local variables:
     *
     *  jk  jk+1 is the initial number of terms of ipio2[] needed
     *      in the computation. The recommended value is 2,3,4,
     *      6 for single, double, extended,and quad.
     *
     *  jz  local integer variable indicating the number of
     *      terms of ipio2[] used.
     *
     *  jx  nx - 1
     *
     *  jv  index for pointing to the suitable ipio2[] for the
     *      computation. In general, we want
     *          ( 2^e0*x[0] * ipio2[jv-1]*2^(-24jv) )/8
     *      is an integer. Thus
     *          e0-3-24*jv >= 0 or (e0-3)/24 >= jv
     *      Hence jv = max(0,(e0-3)/24).
     *
     *  jp  jp+1 is the number of terms in PIo2[] needed, jp = jk.
     *
     *  q[] double array with integral value, representing the
     *      24-bits chunk of the product of x and 2/pi.
     *
     *  q0  the corresponding exponent of q[0]. Note that the
     *      exponent for q[i] would be q0-24*i.
     *
     *  PIo2[]  double precision array, obtained by cutting pi/2
     *      into 24 bits chunks.
     *
     *  f[] ipio2[] in floating point
     *
     *  iq[]    integer array by breaking up q[] in 24-bits chunk.
     *
     *  fq[]    final product of x*(2/pi) in fq[0],..,fq[jk]
     *
     *  ih  integer. If >0 it indicates q[] is >= 0.5, hence
     *      it also indicates the *sign* of the result.
     *
     */

    int __kernel_rem_pio2(double* x, double* y, int e0, int nx, int prec, int const* ipio2)
    {
        /*
         * Constants:
         * The hexadecimal values are the intended ones for the following
         * constants. The decimal values may be used, provided that the
         * compiler will convert from decimal to binary accurately enough
         * to produce the hexadecimal values shown.
         */

        static int const init_jk[] = { 2, 3, 4, 6 }; /* initial value for jk */
        static double const PIo2[] =
        {
            1.57079625129699707031e+00, /* 0x3FF921FB, 0x40000000 */
            7.54978941586159635335e-08, /* 0x3E74442D, 0x00000000 */
            5.39030252995776476554e-15, /* 0x3CF84698, 0x80000000 */
            3.28200341580791294123e-22, /* 0x3B78CC51, 0x60000000 */
            1.27065575308067607349e-29, /* 0x39F01B83, 0x80000000 */
            1.22933308981111328932e-36, /* 0x387A2520, 0x40000000 */
            2.73370053816464559624e-44, /* 0x36E38222, 0x80000000 */
            2.16741683877804819444e-51, /* 0x3569F31D, 0x00000000 */
        };
        static double const zero   = 0.0,
                            one    = 1.0,
                            two24   =  1.67772160000000000000e+07, /* 0x41700000, 0x00000000 */
                            twon24  =  5.96046447753906250000e-08; /* 0x3E700000, 0x00000000 */

        int jz, jx, jv, jp, jk, carry, n, iq[20], i, j, k, m, q0, ih;
        double z, fw, f[20], fq[20], q[20];

        /* initialize jk*/
        jk = init_jk[prec];
        jp = jk;

        /* determine jx,jv,q0, note that 3>q0 */
        jx =  nx - 1;
        jv = (e0 - 3) / 24;
        if(jv < 0) jv = 0;
        q0 =  e0 - 24 * (jv + 1);

        /* set up f[0] to f[jx+jk] where f[jx+jk] = ipio2[jv+jk] */
        j = jv - jx;
        m = jx + jk;
        for(i = 0; i <= m; i++, j++) f[i] = (j < 0) ? zero : (double) ipio2[j];

        /* compute q[0],q[1],...q[jk] */
        for(i = 0; i <= jk; i++)
        {
            for(j = 0, fw = 0.0; j <= jx; j++) fw += x[j] * f[jx + i - j];
            q[i] = fw;
        }

        jz = jk;
recompute:
        /* distill q[] into iq[] reversingly */
        for(i = 0, j = jz, z = q[jz]; j > 0; i++, j--)
        {
            fw    = (double)((int)(twon24 * z));
            iq[i] = (int)(z - two24 * fw);
            z     =  q[j - 1] + fw;
        }

        /* compute n */
        z  = scalbn(z, q0);     /* actual value of z */
        z -= 8.0 * floor(z * 0.125);    /* trim off integer >= 8 */
        n  = (int) z;
        z -= (double)n;
        ih = 0;
        if(q0 > 0)  /* need iq[jz-1] to determine n */
        {
            i  = (iq[jz - 1] >> (24 - q0));
            n += i;
            iq[jz - 1] -= i << (24 - q0);
            ih = iq[jz - 1] >> (23 - q0);
        }
        else if(q0 == 0) ih = iq[jz - 1] >> 23;
        else if(z >= 0.5) ih = 2;

        if(ih > 0)  /* q > 0.5 */
        {
            n += 1;
            carry = 0;
            for(i = 0; i < jz ; i++) /* compute 1-q */
            {
                j = iq[i];
                if(carry == 0)
                {
                    if(j != 0)
                    {
                        carry = 1;
                        iq[i] = 0x1000000 - j;
                    }
                }
                else  iq[i] = 0xffffff - j;
            }
            if(q0 > 0)      /* rare case: chance is 1 in 12 */
            {
                switch(q0)
                {
                case 1:
                    iq[jz - 1] &= 0x7fffff;
                    break;
                case 2:
                    iq[jz - 1] &= 0x3fffff;
                    break;
                }
            }
            if(ih == 2)
            {
                z = one - z;
                if(carry != 0) z -= scalbn(one, q0);
            }
        }

        /* check if recomputation is needed */
        if(z == zero)
        {
            j = 0;
            for(i = jz - 1; i >= jk; i--) j |= iq[i];
            if(j == 0) /* need recomputation */
            {
                for(k = 1; iq[jk - k] == 0; k++); /* k = no. of terms needed */

                for(i = jz + 1; i <= jz + k; i++) /* add q[jz+1] to q[jz+k] */
                {
                    f[jx + i] = (double) ipio2[jv + i];
                    for(j = 0, fw = 0.0; j <= jx; j++) fw += x[j] * f[jx + i - j];
                    q[i] = fw;
                }
                jz += k;
                goto recompute;
            }
        }

        /* chop off zero terms */
        if(z == 0.0)
        {
            jz -= 1;
            q0 -= 24;
            while(iq[jz] == 0) { jz--; q0 -= 24;}
        }
        else     /* break z into 24-bit if necessary */
        {
            z = scalbn(z, -q0);
            if(z >= two24)
            {
                fw = (double)((int)(twon24 * z));
                iq[jz] = (int)(z - two24 * fw);
                jz += 1;
                q0 += 24;
                iq[jz] = (int) fw;
            }
            else iq[jz] = (int) z ;
        }

        /* convert integer "bit" chunk to floating-point value */
        fw = scalbn(one, q0);
        for(i = jz; i >= 0; i--)
        {
            q[i] = fw * (double)iq[i];
            fw *= twon24;
        }

        /* compute PIo2[0,...,jp]*q[jz,...,0] */
        for(i = jz; i >= 0; i--)
        {
            for(fw = 0.0, k = 0; k <= jp && k <= jz - i; k++) fw += PIo2[k] * q[i + k];
            fq[jz - i] = fw;
        }

        /* compress fq[] into y[] */
        switch(prec)
        {
        case 0:
            fw = 0.0;
            for(i = jz; i >= 0; i--) fw += fq[i];
            y[0] = (ih == 0) ? fw : -fw;
            break;
        case 1:
        case 2:
            fw = 0.0;
            for(i = jz; i >= 0; i--) fw += fq[i];
            y[0] = (ih == 0) ? fw : -fw;
            fw = fq[0] - fw;
            for(i = 1; i <= jz; i++) fw += fq[i];
            y[1] = (ih == 0) ? fw : -fw;
            break;
        case 3: /* painful */
            for(i = jz; i > 0; i--)
            {
                fw      = fq[i - 1] + fq[i];
                fq[i]  += fq[i - 1] - fw;
                fq[i - 1] = fw;
            }
            for(i = jz; i > 1; i--)
            {
                fw      = fq[i - 1] + fq[i];
                fq[i]  += fq[i - 1] - fw;
                fq[i - 1] = fw;
            }
            for(fw = 0.0, i = jz; i >= 2; i--) fw += fq[i];
            if(ih == 0)
            {
                y[0] =  fq[0];
                y[1] =  fq[1];
                y[2] =  fw;
            }
            else
            {
                y[0] = -fq[0];
                y[1] = -fq[1];
                y[2] = -fw;
            }
        }
        return n & 7;
    }

    //  calculates x mod PI/2, returning the quadrant in which the
    //  original value was found.
    //
    //  note that at this point, we know that x is finite.
    //
    int rem_piby2(dd_real const & x, double* remainder)
    {
        static double const zero = 0.0;
        static int const two_over_pi[] =
        {
            0xA2F983, 0x6E4E44, 0x1529FC, 0x2757D1, 0xF534DD, 0xC0DB62,
            0x95993C, 0x439041, 0xFE5163, 0xABDEBB, 0xC561B7, 0x246E3A,
            0x424DD2, 0xE00649, 0x2EEA09, 0xD1921C, 0xFE1DEB, 0x1CB129,
            0xA73EE8, 0x8235F5, 0x2EBB44, 0x84E99C, 0x7026B4, 0x5F7E41,
            0x3991D6, 0x398353, 0x39F49C, 0x845F8B, 0xBDF928, 0x3B1FF8,
            0x97FFDE, 0x05980F, 0xEF2F11, 0x8B5A0A, 0x6D1F6D, 0x367ECF,
            0x27CB09, 0xB74F46, 0x3F669E, 0x5FEA2D, 0x7527BA, 0xC7EBE5,
            0xF17B3D, 0x0739F7, 0x8A5292, 0xEA6BFB, 0x5FB11F, 0x8D5D08,
            0x560330, 0x46FC7B, 0x6BABF0, 0xCFBC20, 0x9AF436, 0x1DA9E3,
            0x91615E, 0xE61B08, 0x659985, 0x5F14A0, 0x68408D, 0xFFD880,
            0x4D7327, 0x310606, 0x1556CA, 0x73A8C9, 0x60E27B, 0xC08C6B,
        };

        double tx[6];
        int e0, i, nx, n;
        dd_real z = std::abs(x);

        if(z <= _pi4)                        /* |x| ~<= pi/4 , no need for reduction */
        {
            remainder[0] = x._hi();
            remainder[1] = x._lo();
            remainder[2] = 0.0;
            return 0;
        }

        /* set z = scalbn( |x|, -ilogb( x ) + 23 ) */
        e0  = std::ilogb(x) - 23;
        z = std::scalbn(z, -e0);
        for(i = 0; i < 5; i++)
        {
            tx[i] = z.toInt();
            z     = std::scalbn(z - tx[ i ], 24);
        }
        tx[5] = z.toInt();

        nx = 6;
        while(tx[ nx - 1] == zero) nx--;     /* skip zero terms */
        n  =  __kernel_rem_pio2(tx, remainder, e0, nx, 3, two_over_pi);

        if(x < 0.0)
        {
            remainder[0] = -remainder[0];
            remainder[1] = -remainder[1];
            remainder[2] = -remainder[2];
            return -n;
        }
        return n;
    }

    //  calculate sin( a ), cos( a ). assumes |a| <= Pi/4.
    //
    void piby2_sincos(dd_real const & a, dd_real & sin_a, dd_real & cos_a)
    {
        if(iszero(a))
        {
            sin_a = a;                      //  handle case of -0.0 according to IEEE 754
            cos_a = 1.0;
            return;
        }

        //  note that |r| < |a| < 1.0!

        dd_real r = -sqr(a);

        dd_real s = inv_fact[31];
        for(auto i = 31; i > 1; i -= 2)
        {
            s = std::Fma(s, r, inv_fact[i - 2]);
        }
        s *= a;

        dd_real c = inv_fact[30];
        for(auto i = 30; i > 0; i -= 2)
        {
            c = std::Fma(c, r, inv_fact[i - 2]);
        }

        sin_a = s;
        cos_a = c;
    }

    void round_string(char* s, int precision, int* offset)
    {
        /*
         Input string must be all digits or errors will occur.
         */

        int i;
        int D = precision ;

        /* Round, handle carry */
        if(s[D - 1] >= '5')
        {
            s[D - 2]++;

            i = D - 2;
            while(i > 0 && s[i] > '9')
            {
                s[i] -= 10;
                s[--i]++;
            }
        }

        /* If first digit is 10, shift everything. */
        if(s[0] > '9')
        {
            // e++; // don't modify exponent here
            for(i = precision; i >= 2; i--) s[i] = s[i - 1];
            s[0] = '1';
            s[1] = '0';

            (*offset)++ ; // now offset needs to be increased by one
            precision++ ;
        }

        s[precision] = 0; // add terminator for array
    }

    void append_expn(std::string & str, int expn)
    {
        int k;

        str += (expn < 0 ? '-' : '+');
        expn = std::abs(expn);

        if(expn >= 100)
        {
            k = (expn / 100);
            str += static_cast<char>('0' + k);
            expn -= 100 * k;
        }

        k = (expn / 10);
        str += static_cast<char>('0' + k);
        expn -= 10 * k;

        str += static_cast<char>('0' + expn);
    }


    //  a[] is a quad-double in the range -1024.0 <= a <= 1024.0
    //
    dd_real _exp2(double* a, int scale = 0)
    {
        //  extract integer part from a[0]
        //
        double int_a;
        a[0] = std::modf(a[0], &int_a);

        int_a += scale;
        if(int_a > std::numeric_limits<dd_real>::max_exponent)
            return std::numeric_limits<dd_real>::infinity();

        if(int_a < std::numeric_limits<dd_real>::min_exponent)
            return _zero;

        //  renormalize a, copying bits from a[2] if necessary
        //
        qd::three_sum(a[0], a[1], a[2]);

        dd_real frac_a(a[0], a[1]);

        frac_a *= _ln2;                         //  TODO - calculate with high precision!
        dd_real x = inv_fact[28];
        for(auto i = 28; i > 0; i--)
        {
            x = std::Fma(x, frac_a, inv_fact[i - 1]);
        }

        return std::ldexp(x, (int)int_a);
    }

    //  assumes 0.0 < a < inf
    //
    dd_real _log(dd_real const & a)
    {
        int k;
        dd_real f = std::frexp(a, &k);   //  0.5 <= |f| < 1.0
        if(f < _inv_sqrt2)
        {
            f = std::ldexp(f, 1);
            --k;
        }

        //  sqrt( 0.5 ) <= f < sqrt( 2.0 )
        //  -0.1715... <= s < 0.1715...
        //
        double res[3];
        res[0] = qd::two_sum(f._hi(), 1.0, res[1]);
        res[1] = qd::two_sum(f._lo(), res[1], res[2]);
        dd_real f_plus = res[0] == 0.0 ? dd_real(res[1], res[2]) : dd_real(res[0], res[1]);
        res[0] = qd::two_sum(f._hi(), -1.0, res[1]);
        res[1] = qd::two_sum(f._lo(), res[1], res[2]);
        dd_real f_minus = res[0] == 0.0 ? dd_real(res[1], res[2]) : dd_real(res[0], res[1]);

        dd_real s = f_minus / f_plus;

        //  calculate log( f ) = log( 1 + s ) - log( 1 - s )
        //
        //  log( 1+s ) =  s - s^2/2 + s^3/3 - s^4/4 ...
        //  log( 1-s ) = -s + s^2/2 - s^3/3 - s^4/4 ...
        //  log( f ) = 2*s + 2s^3/3 + 2s^5/5 + ...
        //
        dd_real s2 = s * s;

        //  TODO    - economize the power series using Chebyshev polynomials
        //
        dd_real x = inv_int[41];
        for(int i = 41; i > 1; i -= 2)
        {
            x = std::Fma(x, s2, inv_int[i - 2]);
        }
        x *= std::ldexp(s, 1);          //  x *= 2*s

        return std::Fma(k, _ln2, x);
    }

    //  assumes -1.0 < a < 2.0
    //
    dd_real _log1p(dd_real const & a)
    {
        static const dd_real a_max = _sqrt2 - 1.0;
        static const dd_real a_min = _inv_sqrt2 - 1.0;

        int ilog = std::ilogb(a) + 1;       //  0.5 <= frac < 1.0

        if(ilog < -std::numeric_limits<dd_real>::digits / 2)        //  |a| <= 2^-54 - error O( 2^-108)
            return a;
        if(ilog < -std::numeric_limits<dd_real>::digits / 3)        //  |a| <= 2^-36 - error O( 2^-108)
            return a * std::Fma(a, -0.5, 1.0);
        if(ilog < -std::numeric_limits<dd_real>::digits / 4)        //  |a| <= 2^-27 - error O( 2^-108)
            return a * std::Fma(a, -std::Fma(a, -_third, 0.5), 1.0);

        dd_real f_minus = a;
        int k = 0;

        if((a > a_max) || (a < a_min))
        {
            double res[3];
            res[0] = qd::two_sum(a._hi(), 1.0, res[1]);
            res[1] = qd::two_sum(a._lo(), res[1], res[2]);
            dd_real f_p1 = res[0] == 0.0 ? dd_real(res[1], res[2]) : dd_real(res[0], res[1]);

            f_p1 = std::frexp(f_p1, &k);    //  0.5 <= |f_p1| < 1.0; k <= 2
            if(f_p1 < _inv_sqrt2)
            {
                --k;
                std::ldexp(f_p1, 1);
            }

            //  at this point, we have 2^k * ( 1.0 + f ) = 1.0 + a
            //                          sqrt( 0.5 ) <= 1.0 + f <= sqrt( 2.0 )
            //
            //                          f = 2^-k * a - ( 1.0 - 2^-k )
            double df[2];
            df[0] = qd::two_sum(1.0, -std::ldexp(1.0, -k), df[1]);
            f_minus = std::ldexp(a, -k) - dd_real(df[0], df[1]);
        }

        dd_real f_plus = f_minus + 2.0;
        dd_real s = f_minus / f_plus;

        //  calculate log( f ) = log( 1 + s ) - log( 1 - s )
        //
        //  log( 1+s ) =  s - s^2/2 + s^3/3 - s^4/4 ...
        //  log( 1-s ) = -s + s^2/2 - s^3/3 - s^4/4 ...
        //  log( f ) = 2*s + 2s^3/3 + 2s^5/5 + ...
        //
        dd_real s2 = s * s;

        //  TODO    - economize the power series using Chebyshev polynomials
        //
        dd_real x = inv_int[41];
        for(int i = 41; i > 1; i -= 2)
        {
            x = std::Fma(x, s2, inv_int[i - 2]);
        }
        x *= std::ldexp(s, 1);          //  x *= 2*s

        return std::Fma(k, _ln2, x);
    }

}

dd_real::dd_real(std::string const & s)
{
    if(dd_real::read(s, *this))
    {
        error("(dd_real::dd_real): INPUT ERROR.");
        *this = std::numeric_limits<dd_real>::quiet_NaN();
    }
}

dd_real::dd_real(std::wstring const & ws)
{
    if(dd_real::read(ws, *this))
    {
        error("(dd_real::dd_real): INPUT ERROR.");
        *this = std::numeric_limits<dd_real>::quiet_NaN();
    }
}

dd_real dd_real::div(double a, double b)
{
    if(QD_ISNAN(a))
        return a;

    if(QD_ISNAN(b))
        return b;

    if(b == 0.0)
    {
        auto signA = QD_COPYSIGN(1.0, a);
        auto signB = QD_COPYSIGN(1.0, b);
        return signA * signB * std::numeric_limits<dd_real>::infinity();
    }

    double q1 = a / b;

    /* Compute  a - q1 * b */
    double p2, p1 = qd::two_prod(q1, b, p2);
    double e, s = qd::two_diff(a, p1, e);
    e -= p2;

    /* get next approximation */
    double q2 = (s + e) / b;

    //  normalize
    s = qd::quick_two_sum(q1, q2, e);
    return dd_real(s, e);
}

dd_real & dd_real::operator/=(dd_real const & b)
{
    if(isnan())
        return *this;

    if(b.isnan())
    {
        *this = b;
        return *this;
    }

    if(iszero(b))
    {
        if(iszero(*this))
        {
            *this = std::numeric_limits<dd_real>::quiet_NaN();
        }
        else
        {
            auto signA = QD_COPYSIGN(1.0, x[0]);
            auto signB = QD_COPYSIGN(1.0, b.x[0]);
            *this = signA * signB * std::numeric_limits<dd_real>::infinity();
        }
        return *this;
    }

    double q1 = x[0] / b.x[0];  /* approximate quotient */
    if(QD_ISFINITE(q1))
    {
        dd_real r = std::Fma(-q1, b, *this);

        double q2 = r.x[0] / b.x[0];
        r = std::Fma(-q2, b, r);

        double q3 = r.x[0] / b.x[0];

        qd::three_sum(q1, q2, q3);
        x[0] = q1;
        x[1] = q2;
    }
    else
    {
        x[0] = q1;
        x[1] = 0.0;
    }

    return *this;
}

dd_real & dd_real::operator/=(double b)
{
    if(isnan())
        return *this;

    if(QD_ISNAN(b))
    {
        *this = b;
        return *this;
    }

    if(b == 0.0)
    {
        auto signA = QD_COPYSIGN(1.0, x[0]);
        auto signB = QD_COPYSIGN(1.0, b);
        *this = iszero(*this) ? std::numeric_limits<dd_real>::quiet_NaN() : signA * signB * std::numeric_limits<dd_real>::infinity();
        return *this;
    }

    double q1 = x[0] / b;  /* approximate quotient */
    if(QD_ISFINITE(q1))
    {
        dd_real r = std::Fma(-q1, b, *this);

        double q2 = r.x[0] / b;
        r = std::Fma(-q2, b, r);

        double q3 = r.x[0] / b;

        qd::three_sum(q1, q2, q3);
        x[0] = q1;
        x[1] = q2;
    }
    else
    {
        x[0] = q1;
        x[1] = 0.0;
    }

    return *this;
}

dd_real dd_real::sqrt(double a)
{
    return std::sqrt(dd_real(a));
}

dd_real & dd_real::operator=(std::string const & s)
{
    if(dd_real::read(s, *this))
    {
        error("(dd_real::operator=): INPUT ERROR.");
        *this = std::numeric_limits<dd_real>::quiet_NaN();
    }

    return *this;
}

dd_real & dd_real::operator=(std::wstring const & s)
{
    if(dd_real::read(s, *this))
    {
        error("(dd_real::operator=): INPUT ERROR.");
        *this = std::numeric_limits<dd_real>::quiet_NaN();
    }

    return *this;
}

void dd_real::to_digits(char* s, int & expn, int precision) const
{
    int D = precision + 1;  /* number of digits to compute */
    dd_real r = std::abs(*this);
    int e;  /* exponent */
    int i, d;

    if(iszero(*this))
    {
        /* this == 0.0 */
        expn = 0;
        for(i = 0; i < precision; i++)
            s[i] = '0';
        return;
    }

    /* First determine the (approximate) exponent. */
    std::frexp(*this, &e);   //  e is appropriate for 0.5 <= x < 1
    std::ldexp(r, 1);           //  adjust e, r
    --e;
    e = (_log2 * (double)e).toInt();

    if(e < 0)
    {
        if(e < -300)
        {
            r = std::ldexp(r, 53);
            r *= pown(_ten, -e);
            r = std::ldexp(r, -53);
        }
        else
            r *= pown(_ten, -e);
    }
    else if(e > 0)
    {
        if(e > 300)
        {
            r = std::ldexp(r, -53);
            r /= pown(_ten, e);
            r = std::ldexp(r, +53);
        }
        else
            r /= pown(_ten, e);
    }

    /* Fix exponent if we are off by one */
    if(r >= _ten)
    {
        r /= _ten;
        ++e;
    }
    else if(r < 1.0)
    {
        r *= _ten;
        --e;
    }

    if((r >= _ten) || (r < _one))
    {
        error("(dd_real::to_digits): can't compute exponent.");
        return;
    }

    /* Extract the digits */
    for(i = 0; i < D; i++)
    {
        d = static_cast<int>(r.x[0]);
        r -= d;
        r *= 10.0;

        s[i] = static_cast<char>(d + '0');
    }

    /* Fix out of range digits. */
    for(i = D - 1; i > 0; i--)
    {
        if(s[i] < '0')
        {
            s[i - 1]--;
            s[i] += 10;
        }
        else if(s[i] > '9')
        {
            s[i - 1]++;
            s[i] -= 10;
        }
    }

    if(s[0] <= '0')
    {
        error("(dd_real::to_digits): non-positive leading digit.");
        return;
    }

    /* Round, handle carry */
    if(s[D - 1] >= '5')
    {
        s[D - 2]++;

        i = D - 2;
        while(i > 0 && s[i] > '9')
        {
            s[i] -= 10;
            s[--i]++;
        }
    }

    /* If first digit is 10, shift everything. */
    if(s[0] > '9')
    {
        ++e;
        for(i = precision; i >= 2; i--)
            s[i] = s[i - 1];
        s[0] = '1';
        s[1] = '0';
    }

    s[precision] = 0;
    expn = e;
}

std::string dd_real::to_string(std::streamsize precision, std::streamsize width, std::ios_base::fmtflags fmt, bool showpos, bool uppercase, char fill) const
{
    std::string s;
    bool fixed = (fmt & std::ios_base::fixed) != 0;
    bool sgn = true;
    int i, e = 0;

    if(isnan())
    {
        s = uppercase ? "NAN" : "nan";
        sgn = false;
    }
    else
    {
        if(std::signbit(*this))
            s += '-';
        else if(showpos)
            s += '+';
        else
            sgn = false;

        if(isinf())
        {
            s += uppercase ? "INF" : "inf";
        }
        else if(*this == 0.0)
        {
            /* Zero case */
            s += '0';
            if(precision > 0)
            {
                s += '.';
                s.append(static_cast<unsigned int>(precision), '0');
            }
        }
        else
        {
            /* Non-zero case */
            int off = (fixed ? (1 + std::floor(std::log10(std::abs(*this)))).toInt() : 1);
            int d = static_cast<int>(precision) + off;

            int d_with_extra = d;
            if(fixed)
                d_with_extra = std::max(60, d); // longer than the max accuracy for DD

            // highly special case - fixed mode, precision is zero, abs(*this) < 1.0
            // without this trap a number like 0.9 printed fixed with 0 precision prints as 0
            // should be rounded to 1.
            if(fixed && (precision == 0) && (std::abs(*this) < 1.0))
            {
                if(std::abs(*this) >= 0.5)
                    s += '1';
                else
                    s += '0';

                return s;
            }

            // handle near zero to working precision (but not exactly zero)
            if(fixed && d <= 0)
            {
                s += '0';
                if(precision > 0)
                {
                    s += '.';
                    s.append(static_cast<unsigned int>(precision), '0');
                }
            }
            else     // default
            {

                char* t; //  = new char[d+1];
                int j;

                if(fixed)
                {
                    t = new char[d_with_extra + 1];
                    to_digits(t, e, d_with_extra);
                }
                else
                {
                    t = new char[d + 1];
                    to_digits(t, e, d);
                }

                if(fixed)
                {
                    // fix the string if it's been computed incorrectly
                    // round here in the decimal string if required
                    round_string(t, d + 1 , &off);

                    if(off > 0)
                    {
                        for(i = 0; i < off; i++) s += t[i];
                        if(precision > 0)
                        {
                            s += '.';
                            for(j = 0; j < precision; j++, i++) s += t[i];
                        }
                    }
                    else
                    {
                        s += "0.";
                        if(off < 0) s.append(-off, '0');
                        for(i = 0; i < d; i++) s += t[i];
                    }
                }
                else
                {
                    s += t[0];
                    if(precision > 0) s += '.';

                    for(i = 1; i <= precision; i++)
                        s += t[i];

                }
                delete [] t;
            }
        }

        // trap for improper offset with large values
        // without this trap, output of values of the for 10^j - 1 fail for j > 28
        // and are output with the point in the wrong place, leading to a dramatically off value
        if(fixed && (precision > 0))
        {
            // make sure that the value isn't dramatically larger
            double from_string = atof(s.c_str());

            // if this ratio is large, then we've got problems
            if(fabs(from_string / this->x[0]) > 3.0)
            {

                // loop on the string, find the point, move it up one
                // don't act on the first character
                for(std::string::size_type i = 1; i < s.length(); i++)
                {
                    if(s[i] == '.')
                    {
                        s[i] = s[i - 1] ;
                        s[i - 1] = '.' ;
                        break;
                    }
                }

                from_string = atof(s.c_str());
                // if this ratio is large, then the string has not been fixed
                if(fabs(from_string / this->x[0]) > 3.0)
                {
                    error("Re-rounding unsuccessful in large number fixed point trap.") ;
                }
            }
        }


        if(!fixed && !isinf())
        {
            /* Fill in exponent part */
            s += uppercase ? 'E' : 'e';
            append_expn(s, e);
        }
    }

    /* Fill in the blanks */
    int len = s.length();
    if(len < width)
    {
        int delta = static_cast<int>(width) - len;
        if(fmt & std::ios_base::internal)
        {
            if(sgn)
                s.insert(static_cast<std::string::size_type>(1), delta, fill);
            else
                s.insert(static_cast<std::string::size_type>(0), delta, fill);
        }
        else if(fmt & std::ios_base::left)
        {
            s.append(delta, fill);
        }
        else
        {
            s.insert(static_cast<std::string::size_type>(0), delta, fill);
        }
    }

    return s;
}

/* Reads in a double-double number from the string s. */
int dd_real::read(std::string const & s, dd_real & a)
{
    char const* p = s.c_str();
    char ch;
    int sign = 0;
    int point = -1;
    int nd = 0;
    int e = 0;
    bool done = false;
    dd_real r = 0.0;
    int nread;

    /* Skip any leading spaces */
    while(std::isspace(*p))
        ++p;

    while(!done && (ch = *p) != '\0')
    {
        if(std::isdigit(ch))
        {
            int d = ch - '0';
            r *= 10.0;
            r += static_cast<double>(d);
            nd++;
        }
        else
        {
            switch(ch)
            {
            case '.':
                if(point >= 0)
                    return -1;
                point = nd;
                break;

            case '-':
            case '+':
                if(sign != 0 || nd > 0)
                    return -1;
                sign = (ch == '-') ? -1 : 1;
                break;

            case 'E':
            case 'e':
                nread = std::sscanf(p + 1, "%d", &e);
                done = true;
                if(nread != 1)
                    return -1;
                break;

            default:
                return -1;
            }
        }

        ++p;
    }

    if(point >= 0)
    {
        e -= (nd - point);
    }

    if(e > 0)
    {
        r *= pown(_ten, e);
    }
    else if(e < 0)
        r /= pown(_ten, -e);

    a = (sign == -1) ? -r : r;
    return 0;
}

/* Reads in a double-double number from the string s. */
int dd_real::read(std::wstring const & s, dd_real & a)
{
    wchar_t const* p = s.c_str();
    wchar_t ch;
    int sign = 0;
    int point = -1;
    int nd = 0;
    int e = 0;
    bool done = false;
    dd_real r = 0.0;
    int nread;

    /* Skip any leading spaces */
    while(std::iswspace(*p))
        ++p;

    while(!done && (ch = *p) != L'\0')
    {
        if(std::iswdigit(ch))
        {
            int d = ch - L'0';
            r *= 10.0;
            r += static_cast<double>(d);
            nd++;
        }
        else
        {
            switch(ch)
            {
            case L'.':
                if(point >= 0)
                    return -1;
                point = nd;
                break;

            case L'-':
            case L'+':
                if(sign != 0 || nd > 0)
                    return -1;
                sign = (ch == L'-') ? -1 : 1;
                break;

            case L'E':
            case L'e':
                nread = std::swscanf(p + 1, L"%d", &e);
                done = true;
                if(nread != 1)
                    return -1;
                break;

            default:
                return -1;
            }
        }

        ++p;
    }

    if(point >= 0)
    {
        e -= (nd - point);
    }

    if(e > 0)
    {
        r *= pown(_ten, e);
    }
    else if(e < 0)
        r /= pown(_ten, e);

    a = (sign == -1) ? -r : r;
    return 0;
}

/* Writes the double-double number into the character array s of length len.
   The integer d specifies how many significant digits to write.
   The string s must be able to hold at least (d+8) characters.
   showpos indicates whether to use the + sign, and uppercase indicates
   whether the E or e is to be used for the exponent. */
void dd_real::write(char* s, int len, int precision, bool showpos, bool uppercase) const
{
    std::string str = to_string(precision, 0, std::ios_base::scientific, showpos, uppercase);
    strncpy(s, str.c_str(), len - 1);
    s[ len - 1 ] = 0;
}

/* polyeval(c, n, x)
    Evaluates the given n-th degree polynomial at x.
    The polynomial is given by the array of (n+1) coefficients. */
dd_real polyeval(const dd_real* c, int n, const dd_real & x)
{
    /* Just use Horner's method of polynomial evaluation. */
    dd_real r = c[n];

    for(int i = n - 1; i >= 0; i--)
    {
        r *= x;
        r += c[i];
    }

    return r;
}

dd_real ddrand()
{
    static double const m_const = 1.0 / (RAND_MAX + 1);
    dd_real r = 0.0;

    for(auto m = m_const; m > std::numeric_limits<dd_real>::epsilon(); m *= m_const)
    {
        double d = std::rand() * m;
        r += d;
    }

    return r;
}

QD_API dd_real reciprocal(dd_real const & a)
{
    if(iszero(a))
        return std::copysign(std::numeric_limits<dd_real>::infinity(), a);

    if(std::isinf(a))
        return std::copysign(_zero, a);

    double q1 = 1.0 / a._hi();  /* approximate quotient */
    if(QD_ISFINITE(q1))
    {
        dd_real r = std::Fma(-q1, a, 1.0);

        double q2 = r._hi() / a._hi();
        r = std::Fma(-q2, a, r);

        double q3 = r._hi() / a._hi();
        qd::three_sum(q1, q2, q3);
        return dd_real(q1, q2);
    }
    else
    {
        return dd_real(q1, 0.0);
    }
}

/* polyroot(c, n, x0)
    Given an n-th degree polynomial, finds a root close to
    the given guess x0.  Note that this uses simple Newton
    iteration scheme, and does not work for multiple roots.  */
QD_API dd_real polyroot(const dd_real* c, int n,
                        const dd_real & x0, int max_iter, double thresh)
{
    dd_real x = x0;
    dd_real f;
    dd_real* d = new dd_real[n];
    bool conv = false;
    int i;
    double max_c = std::abs(c[0].toDouble());
    double v;

    if(thresh == 0.0) thresh = _eps;

    /* Compute the coefficients of the derivatives. */
    for(i = 1; i <= n; i++)
    {
        v = std::abs(c[i].toDouble());
        if(v > max_c) max_c = v;
        d[i - 1] = c[i] * static_cast<double>(i);
    }
    thresh *= max_c;

    /* Newton iteration. */
    for(i = 0; i < max_iter; i++)
    {
        f = polyeval(c, n, x);

        if(std::abs(f) < thresh)
        {
            conv = true;
            break;
        }
        x -= (f / polyeval(d, n - 1, x));
    }
    delete [] d;

    if(!conv)
    {
        error("(dd_real::polyroot): Failed to converge.");
        return std::numeric_limits<dd_real>::quiet_NaN();
    }

    return x;
}

dd_real pown(dd_real const & a, int n)
{
    if(std::isnan(a))
        return a;

    int N = std::abs(n);
    dd_real s;

    switch(N)
    {
    case 0:
        if(iszero(a))
        {
            error("(dd_real::pown): Invalid argument.");
            errno = EDOM;
            return std::numeric_limits<dd_real>::quiet_NaN();
        }
        return 1.0;

    case 1:
        s = a;
        break;

    case 2:
        s = sqr(a);

    default:                            /* Use binary exponentiation */
    {
        dd_real r = a;

        s = 1.0;
        while(N > 0)
        {
            if(N % 2 == 1)
            {
                s *= r;
            }
            N /= 2;
            if(N > 0)
                r = sqr(r);
        }
    }
    break;
    }

    /* Compute the reciprocal if n is negative. */
    return n < 0 ? reciprocal(s) : s;
}

dd_real powr(dd_real const & a, dd_real const & b)
{
    if(std::isnan(a))
        return a;
    if(std::isnan(b))
        return b;

    return std::exp(b * std::log(a));
}

dd_real rootn(dd_real const & a, int n)
{
    if(std::isnan(a))
        return a;

    if(n <= 0)
    {
        error("(dd_real::nroot): N must be positive.");
        errno = EDOM;
        return std::numeric_limits<dd_real>::quiet_NaN();
    }

    if((n % 2 == 0) && std::signbit(a))
    {
        error("(dd_real::nroot): Negative argument.");
        errno = EDOM;
        return std::numeric_limits<dd_real>::quiet_NaN();
    }

    if(n == 1)
        return a;

    if(n == 2)
        return std::sqrt(a);

    if(iszero(a))
        return a;                           //  handle case of -0.0 according to IEEE 754

    /* Note  a^{-1/n} = exp(-log(a)/n) */
    dd_real r = std::abs(a);
    dd_real x = std::exp(-std::log(r._hi()) / n);

    /* Perform Newton's iteration. */
    x += x * (1.0 - r * pown(x, n)) / static_cast<double>(n);
    if(a._hi() < 0.0)
        x = -x;
    return reciprocal(x);
}

void sincos(dd_real const & a, dd_real & sin_a, dd_real & cos_a)
{
    if(std::isnan(a))
    {
        sin_a = cos_a = a;
        return;
    }

    if(iszero(a))
    {
        sin_a = a;                      //  handle case of -0.0 according to IEEE 754
        cos_a = 1.0;
        return;
    }

    if(std::isinf(a))
    {
        error("(dd_real::sincos) Infinite argument");
        errno = EDOM;
        sin_a = cos_a = std::numeric_limits<dd_real>::quiet_NaN();
        return;
    }

    //  reduce each component modulo Pi/2
    //
    double remainder[3];
    int quadrant = rem_piby2(a, remainder);

    dd_real s, c;
    piby2_sincos(dd_real(remainder[0], remainder[1]), s, c);
    switch(quadrant & 0x03)
    {
    case 0:
        sin_a = s;
        cos_a = c;
        break;
    case 1:
        sin_a = c;
        cos_a = -s;
        break;
    case 2:
        sin_a = -s;
        cos_a = -c;
        break;
    case 3:
    default:
        sin_a = -c;
        cos_a = s;
        break;
    }
}

void sincosh(dd_real const & a, dd_real & sinh_a, dd_real & cosh_a)
{
    static dd_real const a_max = 36.75;

    if(std::isnan(a))
    {
        sinh_a = cosh_a = a;
        return;
    }

    if(iszero(a))
    {
        sinh_a = 0.0;
        cosh_a = 1.0;
        return;
    }

    if(std::isinf(a))
    {
        sinh_a = a;
        cosh_a = std::numeric_limits<dd_real>::infinity();
        return;
    }

    if(std::abs(a) > a_max)
    {
        sinh_a = cosh_a = std::ldexp(std::exp(std::abs(a)), -1);
        return;
    }

    dd_real ea = std::exp(a);
    dd_real inv_ea = std::exp(-a);

    cosh_a = std::ldexp(ea + inv_ea, -1);

    if(std::abs(a) > 1.0)
    {
        sinh_a = std::ldexp(ea - inv_ea, -1);
    }
    else
    {
        /* since a is small, using the above formula gives
            a lot of cancellation.  So use Taylor series.   */
        dd_real r = sqr(a);

        dd_real s = inv_fact[31];
        for(auto i = 31; i > 1; i -= 2)
        {
            s = std::Fma(s, r, inv_fact[i - 2]);
        }
        s *= a;

        sinh_a = s;
    }
}

dd_real sqr(dd_real const & a)
{
    if(std::isnan(a))
        return a;

    double p2, p1 = qd::two_sqr(a._hi(), p2);
    p2 += 2.0 * a._hi() * a._lo();
    p2 += a._lo() * a._lo();

    double s2, s1 = qd::quick_two_sum(p1, p2, s2);
    return dd_real(s1, s2);
}

dd_real const & dd_pi()                  { return _pi; }
dd_real const & dd_inv_pi()              { return _inv_pi; }

dd_real const & dd_e()                   { return _e; }

dd_real const & dd_ln2()                 { return _ln2; }
dd_real const & dd_ln10()                { return _ln10; }

dd_real const & dd_lge()                 { return _lge; }
dd_real const & dd_lg10()                { return _lg10; }

dd_real const & dd_log2()                { return _log2; }
dd_real const & dd_loge()                { return _loge; }


namespace std
{

    //
    //  classification functions
    //
    int fpclassify(dd_real const & a)
    {
        switch(::_fpclass(a._hi()))
        {
        case _FPCLASS_SNAN:
        case _FPCLASS_QNAN:
            return FP_NAN;

        case _FPCLASS_NINF:
        case _FPCLASS_PINF:
            return FP_INFINITE;

        case _FPCLASS_ND:
        case _FPCLASS_PD:
            return FP_SUBNORMAL;

        case _FPCLASS_NZ:
        case _FPCLASS_PZ:
            return FP_ZERO;

        case _FPCLASS_NN:
        case _FPCLASS_PN:
        default:
            return FP_NORMAL;
        }
    }

    QD_API bool isfinite(dd_real const & a)
    {
        return (::_fpclass(a._hi()) & (_FPCLASS_SNAN | _FPCLASS_QNAN | _FPCLASS_NINF | _FPCLASS_PINF)) == 0;
    }

    QD_API bool isinf(dd_real const & a)
    {
        return (::_fpclass(a._hi()) & (_FPCLASS_NINF | _FPCLASS_PINF)) != 0;
    }

    QD_API bool isnan(dd_real const & a)
    {
        return (::_fpclass(a._hi()) & (_FPCLASS_SNAN | _FPCLASS_QNAN)) != 0;
    }

    QD_API bool isnormal(dd_real const & a)
    {
        return (::_fpclass(a._hi()) & (_FPCLASS_NN | _FPCLASS_PN)) != 0;
    }

    bool signbit(dd_real const & a)
    {
        auto signA = QD_COPYSIGN(1.0, a._hi());
        return signA < 0.0;
    }

    //
    //  floating-point manipulation functions
    //
    dd_real copysign(dd_real const & a, dd_real const & b)
    {
        auto signA = QD_COPYSIGN(1.0, a._hi());
        auto signB = QD_COPYSIGN(1.0, b._hi());

        return signA != signB ? -a : a;
    }

    dd_real frexp(dd_real const & a, int* pexp)
    {
        double hi = std::frexp(a._hi(), pexp);
        double lo = std::ldexp(a._lo(), -*pexp);
        return dd_real(hi, lo);
    }

    int ilogb(dd_real const & a)
    {
        if(iszero(a))
            return FP_ILOGB0;

        if(isnan(a))
            return FP_ILOGBNAN;

        if(isinf(a))
            return INT_MAX;

        int exp;
        std::frexp(a._hi(), &exp);
        return exp - 1;
    }

    dd_real ldexp(dd_real const & a, int exp)
    {
        static_assert(numeric_limits<dd_real>::radix == 2, "CONFIGURATION: dd_real radix must be 2!");
        static_assert(numeric_limits<double>::radix == 2, "CONFIGURATION: double radix must be 2!");

        return dd_real(std::ldexp(a._hi(), exp), std::ldexp(a._lo(), exp));
    }

    dd_real logb(dd_real const & a)
    {
        static_assert(numeric_limits<dd_real>::radix == 2, "CONFIGURATION: dd_real radix must be 2!");

        return log2(a);
    }

    dd_real modf(dd_real const & a, dd_real* b)
    {
        if(isnan(a))
        {
            *b = std::numeric_limits<dd_real>::quiet_NaN();
            errno = EDOM;
            return std::numeric_limits<dd_real>::quiet_NaN();
        }

        if(isinf(a))
        {
            *b = a;
            return _zero;
        }

        double int_hi, frac_hi = modf(a._hi(), &int_hi);

        if(frac_hi == 0.0)
        {
            //  hi part is integer - check lo part
            //
            double int_lo, frac_lo = modf(a._lo(), &int_lo);
            int_hi = qd::quick_two_sum(int_hi, int_lo, int_lo);
            *b = dd_real(int_hi, int_lo);
            return frac_lo;
        }
        else
        {
            //  hi part is non-integral - frac_hi + lo are fraction
            //
            double frac_lo;
            frac_hi = qd::quick_two_sum(frac_hi, a._lo(), frac_lo);
            *b = int_hi;
            return dd_real(frac_hi, frac_lo);
        }
    }

    dd_real scalbn(dd_real const & a, int exp)
    {
        static_assert(numeric_limits<dd_real>::radix == 2, "CONFIGURATION: dd_real radix must be 2!");
        static_assert(numeric_limits<double>::radix == 2, "CONFIGURATION: double radix must be 2!");

        return dd_real(std::ldexp(a._hi(), exp), std::ldexp(a._lo(), exp));
    }

    dd_real scalbln(dd_real const & a, long exp)
    {
        static_assert(numeric_limits<dd_real>::radix == 2, "CONFIGURATION: dd_real radix must be 2!");
        static_assert(numeric_limits<double>::radix == 2, "CONFIGURATION: double radix must be 2!");

        return dd_real(std::ldexp(a._hi(), exp), std::ldexp(a._lo(), exp));
    }

    //
    //  rounding and remainder functions
    //
    dd_real ceil(dd_real const & a)
    {
        if(isnan(a))
            return a;

        double hi = std::ceil(a._hi());
        double lo = 0.0;

        if(hi == a._hi())
        {
            /* High word is integer already.  Round the low word. */
            lo = std::ceil(a._lo());
            hi = qd::quick_two_sum(hi, lo, lo);
        }

        return dd_real(hi, lo);
    }

    dd_real floor(dd_real const & a)
    {
        if(isnan(a))
            return a;

        double hi = std::floor(a._hi());
        double lo = 0.0;

        if(hi == a._hi())
        {
            // High word is integer already.  Round the low word.
            //
            lo = std::floor(a._lo());
            hi = qd::quick_two_sum(hi, lo, lo);
        }

        return dd_real(hi, lo);
    }

    dd_real fmod(dd_real const & a, dd_real const & b)
    {
        if(isnan(a))
            return a;
        if(isnan(b))
            return b;

        if(isinf(a) || iszero(b))
        {
            errno = EDOM;
            return numeric_limits<dd_real>::quiet_NaN();
        }

        if(iszero(a) || isinf(b))
            return a;

        auto x = abs(a);
        auto y = abs(b);

        while(x >= y)
        {
            //  calculate exponents of x, y
            //
            auto ix = ilogb(x);
            auto iy = ilogb(y);

            //  align exponents of x, y and subtract
            //
            auto ys = scalbn(y, ix - iy);
            if(x < ys)               //  if mantissa( y ) > mantissa( x ),
                scalbn(ys, -1);      //      divide by 2

            double z[3];
            z[0] = x._hi() - ys._hi();  //  x._hi() -= y._hi() is always exact (Sterbentz's lemma)
            //  x._lo() -= y._lo() is not always exact, but best we can do...
            z[1] = qd::two_sum(x._lo(), -ys._lo(), z[2]);
            qd::three_sum(z[0], z[1], z[2]);

            x = dd_real(z[0], z[1]);
        }

        return copysign(x, a);
    }

    dd_real round(dd_real const & a)
    {
        if(isnan(a))
            return a;

        auto a_ceil = ceil(a);
        auto a_floor = floor(a);

        if(a_ceil == a_floor)
        {
            //  a is integer
            //
            return a_ceil;
        }

        auto delta_plus = a_ceil - a;
        auto delta_minus = a - a_floor;

        if(delta_plus > delta_minus)
        {
            //  a is closer to floor
            //
            return a_floor;
        }

        if(delta_plus < delta_minus)
        {
            //  a is closer to floor
            //
            return a_ceil;
        }

        //  a is equally distant from floor, ceiling. choose even
        //
        return fmod(a_ceil, 2.0) == 0.0 ? a_ceil : a_floor;
    }

    dd_real trunc(dd_real const & a)
    {
        return signbit(a) ? ceil(a) : floor(a);
    }

    //
    //  minimum, maximum, difference functions
    //
    dd_real fdim(dd_real const & a, dd_real const & b)
    {
        return fmax(0.0, a - b);
    }

    dd_real fmax(dd_real const & a, dd_real const & b)
    {
        if(isnan(a))
            return b;
        if(isnan(b))
            return a;

        return a < b ? b : a;
    }

    dd_real fmin(dd_real const & a, dd_real const & b)
    {
        if(isnan(a))
            return b;
        if(isnan(b))
            return a;

        return a < b ? a : b;
    }

    //
    //  Other functions
    //
    dd_real abs(dd_real const & a)
    {
        return copysign(a, 1.0);
    }

    dd_real fabs(dd_real const & a)
    {
        return copysign(a, 1.0);
    }

    dd_real Fma(dd_real const & a, dd_real const & b, dd_real const & c)
    {
        double p[4];
        qd_mul(a, b, p);
        qd_add(p, c, p);
        p[0] = qd::two_sum(p[0], p[1] + p[2] + p[3], p[1]);
        return dd_real(p[0], p[1]);
    }

    //
    //  trigonometric functions
    //
    dd_real acos(dd_real const & a)
    {
        if(isnan(a))
            return a;

        dd_real abs_a = abs(a);

        if(abs_a > 1.0)
        {
            error("(dd_real::acos): Argument out of domain.");
            errno = EDOM;
            return std::numeric_limits<dd_real>::quiet_NaN();
        }

        if(isone(abs_a))
            return signbit(a) ? _pi : _zero;

        return atan2(sqrt(Fma(a, -a, 1.0)), a);
    }

    dd_real asin(dd_real const & a)
    {
        if(isnan(a))
            return a;

        dd_real abs_a = abs(a);

        if(abs_a > 1.0)
        {
            error("(dd_real::asin): Argument out of domain.");
            errno = EDOM;
            return std::numeric_limits<dd_real>::quiet_NaN();
        }

        if(isone(abs_a))
            return copysign(_pi2, a);

        return atan2(a, sqrt(Fma(a, -a, 1.0)));
    }

    dd_real atan(dd_real const & a)
    {
        if(isnan(a) || iszero(a))
            return a;

        if(isinf(a))
            return copysign(_pi2, a);

        return atan2(a, 1.0);
    }

    dd_real atan2(dd_real const & y, dd_real const & x)
    {
        if(isnan(y))
            return y;

        if(isnan(x))
            return x;

        bool zeroX = iszero(x);
        bool zeroY = iszero(y);
        bool infX = isinf(x);
        bool infY = isinf(y);

        if(zeroX && zeroY)
        {
            if(signbit(x))
            {
                return copysign(_pi, y);                         //  atan2(+/-0, +/-0) returns +/-PI
            }
            else
            {
                return y;                                       //  atan2(+/-0, +0) returns +/-0
            }
        }

        if(zeroX && !zeroY)
        {
            return copysign(_pi2, y);                            //  atan2(y, +/-0) returns +/-PI/2 for y < 0.
            //  atan2(y, +/-0) returns PI/2 for y > 0.
        }

        if(!zeroX && zeroY)
        {
            if(signbit(x))
            {
                return copysign(_pi, y);                         //  atan2(+/-0, x) returns +/-PI for x < 0.
            }
            else
            {
                return y;                                       //  atan2(+/-0, x) returns +/-0 for x > 0.
            }
        }

        if(infX && infY)
        {
            if(signbit(x))
            {
                return copysign(_3pi4, y);                   //  atan2(+/-INF, -INF) returns +/-3Pi/4.
            }
            else
            {
                return copysign(_pi4, y);                        //  atan2(+/-INF, +INF) returns +/-Pi/4.
            }
        }

        if(infX && !infY)
        {
            if(signbit(x))
            {
                return copysign(_pi, y);                         //  atan2(+/-y, -INF) returns +/-Pi for finite y.
            }
            else
            {
                return copysign(_zero, y);                   //  atan2(+/-y, +INF) returns +/-0 for finite y.
            }
        }

        if(!infX && infY)
        {
            return copysign(_pi2, y);                            //  atan2(+/-INF, x) returns +/-Pi/2 for finite x.
        }

        if(x == y)
        {
            return signbit(y) ? -_3pi4 : _pi4;
        }

        if(x == -y)
        {
            return signbit(y) ? -_pi4 : _3pi4;
        }

        /* Compute double precision approximation to atan. */
        dd_real z;
        dd_real sin_z, cos_z;

        z = std::atan2(y.toDouble(), x.toDouble());
        if(std::abs(x) > std::abs(y))
        {
            dd_real a = y / x;

            sincos(z, sin_z, cos_z);
            z += Fma(a, cos_z, -sin_z) * cos_z;
            sincos(z, sin_z, cos_z);
            z += Fma(a, cos_z, -sin_z) * cos_z;
        }
        else
        {
            dd_real inv_a = x / y;

            sincos(z, sin_z, cos_z);
            z -= Fma(inv_a, sin_z, -cos_z) * sin_z;
            sincos(z, sin_z, cos_z);
            z -= Fma(inv_a, sin_z, -cos_z) * sin_z;
        }

        return z;
    }

    dd_real cos(dd_real const & a)
    {
        if(isnan(a))
            return a;

        if(iszero(a))
            return 1.0;

        if(isinf(a))
        {
            error("(dd_real::cos) Infinite argument");
            errno = EDOM;
            return numeric_limits<dd_real>::quiet_NaN();
        }

        //  reduce each component modulo Pi/2
        //
        double remainder[3];
        int quadrant = rem_piby2(a, remainder);

        dd_real s, c;
        piby2_sincos(dd_real(remainder[0], remainder[1]), s, c);
        switch(quadrant & 0x03)
        {
        case 0:
            return c;
        case 1:
            return -s;
        case 2:
            return -c;
        case 3:
        default:
            return s;
        }
    }

    dd_real sin(dd_real const & a)
    {
        if(isnan(a) || iszero(a))
            return a;                           //  handle case of -0.0 according to IEEE 754

        if(isinf(a))
        {
            error("(dd_real::sin) Infinite argument");
            errno = EDOM;
            return numeric_limits<dd_real>::quiet_NaN();
        }

        //  reduce each component modulo Pi/2
        //
        double remainder[3];
        int quadrant = rem_piby2(a, remainder);

        dd_real s, c;
        piby2_sincos(dd_real(remainder[0], remainder[1]), s, c);
        switch(quadrant & 0x03)
        {
        case 0:
            return s;
        case 1:
            return c;
        case 2:
            return -s;
        case 3:
        default:
            return -c;
        }
    }

    dd_real tan(dd_real const & a)
    {
        if(isnan(a) || iszero(a))
            return a;                           //  handle case of -0.0 according to IEEE 754

        if(isinf(a))
        {
            error("(dd_real::tan) Infinite argument");
            errno = EDOM;
            return numeric_limits<dd_real>::quiet_NaN();
        }

        dd_real s, c;
        sincos(a, s, c);
        return s / c;
    }

    //
    //  hyperbolic functions
    //
    dd_real acosh(dd_real const & a)
    {
        if(isnan(a) || isinf(a))
            return a;

        if(isone(a))
            return _zero;

        if(a < 1.0)
        {
            error("(dd_real::acosh): Argument out of domain.");
            errno = EDOM;
            return std::numeric_limits<dd_real>::quiet_NaN();
        }

        return log(a) + log1p(sqrt(1.0 - reciprocal(sqr(a))));
    }

    dd_real asinh(dd_real const & a)
    {
        if(isnan(a) || iszero(a) || isinf(a))
            return a;

        if(iszero(a))
            return a;

        return copysign(log(abs(a)) + ldexp(log1p(reciprocal(sqr(a))), -1) + log1p(reciprocal(sqrt(1 + 1 / sqr(a)))), a);
    }

    dd_real atanh(dd_real const & a)
    {
        if(isnan(a))
            return a;

        if(iszero(a))
            return a;

        if(isone(abs(a)))
            return copysign(numeric_limits<dd_real>::infinity(), a);

        if(abs(a) > 1.0)
        {
            error("(dd_real::atanh): Argument out of domain.");
            errno = EDOM;
            return std::numeric_limits<dd_real>::quiet_NaN();
        }

        return ldexp(_log1p(a) - _log1p(-a), -1);
    }

    dd_real cosh(dd_real const & a)
    {
        static dd_real const a_max = 36.75;

        if(isnan(a))
            return a;

        if(iszero(a))
            return 1.0;

        if(isinf(a))
            return numeric_limits<dd_real>::infinity();

        if(abs(a) > a_max)
        {
            //  exp(|a|) + exp(-|a|) == exp(|a|)

            //  the calculation of a * lge must be performed at high precision
            //  we do this by multiplying 'a' by a triple-double representation
            //  lg(e).
            //
            double a_lg[3];
            td_mul(std::abs(a), a_lge, a_lg);
            return _exp2(a_lg, -1);
        }
        else
        {
            return ldexp(exp(a) + exp(-a), -1);
        }
    }

    dd_real sinh(dd_real const & a)
    {
        static dd_real const a_max = 36.75;

        if(isnan(a) || iszero(a) || isinf(a))
            return a;

        if(abs(a) > 1.0)
        {
            if(abs(a) > a_max)
            {
                //  exp(|a|) - exp(-|a|) == exp(|a|)

                //  the calculation of a * lge must be performed at high precision
                //  we do this by multiplying 'a' by a triple-double representation
                //  lg(e).
                //
                double a_lg[3];
                td_mul(std::abs(a), a_lge, a_lg);
                return copysign(_exp2(a_lg, -1), a);
            }
            else
            {
                return ldexp(exp(a) - exp(-a), -1);
            }
        }

        /* since a is small, using the above formula gives
           a lot of cancellation.  So use Taylor series.   */

        //  note that |r| < |a| < 1.0!

        dd_real r = sqr(a);

        dd_real s = inv_fact[31];
        for(auto i = 31; i > 1; i -= 2)
        {
            s = Fma(s, r, inv_fact[i - 2]);
        }
        s *= a;

        return s;
    }

    dd_real tanh(dd_real const & a)
    {
        static dd_real const a_max = 36.75;

        if(isnan(a))
            return a;

        if(iszero(a))
            return a;

        if(isinf(a))
            return signbit(a) ? -1.0 : 1.0;

        if(abs(a) > a_max)
        {
            //  exp( a ) +/- exp( -a ) == exp( a )
            //
            return copysign(1.0, a);
        }
        else
        {
            dd_real ea = std::exp(a);
            dd_real inv_ea = std::exp(-a);

            dd_real cosh_a = ea + inv_ea;
            dd_real sinh_a;

            if(abs(a) > 1.0)
            {
                sinh_a = ea - inv_ea;
            }
            else
            {
                /* since a is small, using the above formula gives
                a lot of cancellation.  So use Taylor series.   */

                //  note that |r| < |a| < 1.0!

                dd_real r = sqr(a);

                dd_real s = inv_fact[31];
                for(auto i = 31; i > 1; i -= 2)
                {
                    s = Fma(s, r, inv_fact[i - 2]);
                }
                s *= a;

                sinh_a = ldexp(s, 1);
            }

            return sinh_a / cosh_a;
        }
    }

    //
    //  exponential and logarithmic functions
    //
    dd_real exp(dd_real const & a)
    {
        if(isnan(a))
            return a;

        if(iszero(a))
            return 1.0;

        if(isinf(a))
            return signbit(a) ? _zero : a;

        //  the calculation of a * lge must be performed at high precision
        //  we do this by multiplying 'a' by a triple-double representation
        //  lg(e).
        //
        double a_lg[3];
        td_mul(a, a_lge, a_lg);

        return _exp2(a_lg);
    }

    dd_real exp2(dd_real const & a)
    {
        if(isnan(a))
            return a;

        if(iszero(a))
            return 1.0;

        if(isinf(a))
            return signbit(a) ? _zero : a;

        double sum[3];
        sum[0] = a._hi();
        sum[1] = a._lo();
        sum[2] = 0.0;

        return _exp2(sum);
    }

    dd_real expm1(dd_real const & a)
    {
        if(isnan(a))
            return a;

        if(iszero(a))
            return a;

        if(isinf(a))
            return signbit(a) ? -1.0 : a;

        //  exp( a ) > 2.0; no destructive cancelation is possible
        if(std::abs(a) > _ln2)                    //  exp( a ) < 0.5; no destructive cancellation is possible
        {
            return exp(a) - 1.0;
        }

        dd_real x = inv_fact[28];               //  use Taylor series for exp( x ) - 1.0
        for(auto i = 28; i > 1; i--)
        {
            x = std::Fma(x, a, inv_fact[i - 1]);
        }
        x *= a;

        return x;
    }

    dd_real log(dd_real const & a)
    {
        if(isnan(a))
            return a;

        if(iszero(a))
            return -numeric_limits<dd_real>::infinity();

        if(isone(a))
            return 0.0;

        if(signbit(a))
        {
            error("(dd_real::log): Non-positive argument.");
            errno = EDOM;
            return std::numeric_limits<dd_real>::quiet_NaN();
        }

        if(isinf(a))
            return a;

        return _log(a);
    }

    dd_real log1p(dd_real const & a)
    {
        if(isnan(a))
            return a;

        if(iszero(a))
            return 0.0;

        if(a == -1.0)
            return -numeric_limits<dd_real>::infinity();

        if(a < -1.0)
        {
            error("(dd_real::log): Non-positive argument.");
            errno = EDOM;
            return std::numeric_limits<dd_real>::quiet_NaN();
        }

        if(isinf(a))
            return a;


        if((a >= 2.0) || (a <= -0.5))                  //  a >= 2.0 - no loss of significant bits - use log()
            return _log(1.0 + a);

        //  at this point, -1.0 < a < 2.0
        //
        return _log1p(a);
    }

    dd_real log2(dd_real const & a)
    {
        if(isnan(a))
            return a;

        if(iszero(a))
            return -numeric_limits<dd_real>::infinity();

        if(isone(a))
            return 0.0;

        if(signbit(a))
        {
            error("(dd_real::log2): Non-positive argument.");
            errno = EDOM;
            return std::numeric_limits<dd_real>::quiet_NaN();
        }

        if(isinf(a))
            return a;

        return _lge * _log(a);
    }

    dd_real log10(dd_real const & a)
    {
        if(isnan(a))
            return a;

        if(iszero(a))
            return -numeric_limits<dd_real>::infinity();

        if(isone(a))
            return 0.0;

        if(signbit(a))
        {
            error("(dd_real::log10): Non-positive argument.");
            errno = EDOM;
            return std::numeric_limits<dd_real>::quiet_NaN();
        }

        if(isinf(a))
            return a;

        return _loge * _log(a);
    }

    //
    //  power functions
    //
    dd_real cbrt(dd_real const & a)
    {
        if(!isfinite(a) || iszero(a))
            return a;                       //  NaN returns NaN; +/-Inf returns +/-Inf, +/-0.0 returns +/-0.0

        bool signA = signbit(a);
        int e;                              //  0.5 <= r < 1.0
        dd_real r = frexp(abs(a), &e);
        while(e % 3 != 0)
        {
            ++e;
            r = ldexp(r, -1);
        }

        // at this point, 0.125 <= r < 1.0
        dd_real x = pow(r._hi(), -_third._hi());

        //  refine estimate using Newton's iteration
        x += x * (1.0 - r * sqr(x) * x) * _third;
        x += x * (1.0 - r * sqr(x) * x) * _third;
        x = reciprocal(x);

        if(signA)
            x = -x;

        return ldexp(x, e / 3);
    }

    dd_real hypot(dd_real const & x, dd_real const & y)
    {
        //  return +inf for ( +/-inf, NaN ) - see C99 F.9.4.3
        if(isnan(x))
            return isinf(y) ? std::numeric_limits<dd_real>::infinity() : x;
        if(isnan(y))
            return isinf(y) ? std::numeric_limits<dd_real>::infinity() : y;

        if(isinf(x) || isinf(y))
            return numeric_limits<dd_real>::infinity();

        int logX = ilogb(x);
        int logY = ilogb(y);

        if(logX - logY > numeric_limits<dd_real>::digits)
            return std::abs(x);

        if(logY - logX > numeric_limits<dd_real>::digits)
            return std::abs(y);

        //  x, y are within 'digits' orders of magnitude of each other.
        //  shift them to the middle of the FP range, perform the
        //  hypot function, and shift the result back.
        //
        if(logX > logY)
        {
            auto _x = ldexp(x, (logY - logX) / 2);
            auto _y = ldexp(y, (logY - logX) / 2);
            return ldexp(sqrt(sqr(_x) + sqr(_y)), (logX - logY) / 2);
        }
        else
        {
            auto _x = ldexp(x, (logX - logY) / 2);
            auto _y = ldexp(y, (logX - logY) / 2);
            return ldexp(sqrt(sqr(_x) + sqr(_y)), (logY - logX) / 2);
        }
    }

    dd_real pow(dd_real const & a, dd_real const & b)
    {
        if(isnan(a))
            return a;
        if(isnan(b))
            return b;

        //  TODO    add special conditions

        dd_real int_b, frac_b = modf(b, &int_b);

        if(abs(int_b) <= dd_real((numeric_limits<long long>::max)()))
        {
            dd_real s;
            long long N = std::abs(int_b.toLongLong());

            switch(N)
            {
            case 0:
                s = 1.0;
                break;

            case 1:
                s = a;
                break;

            case 2:
                s = sqr(a);
                break;

            default:                        /* Use binary exponentiation */
            {
                dd_real r = a;

                s = 1.0;
                while(N > 0)
                {
                    if(N % 2 == 1)
                    {
                        s *= r;
                    }
                    N /= 2;
                    if(N > 0)
                        r = sqr(r);
                }
            }
            break;
            }

            if(signbit(int_b))
                s = reciprocal(s);
            if(frac_b != 0.0)
                s *= exp(frac_b * log(a));
            return s;
        }
        else
        {
            return exp(log(a) * b);
        }
    }

    dd_real sqrt(dd_real const & a)
    {
        if(isnan(a) || iszero(a))
            return a;                           //  handle case of -0.0 according to IEEE 754

        if(signbit(a))
        {
            error("(dd_real::sqrt): Negative argument.");
            errno = EDOM;
            return std::numeric_limits<dd_real>::quiet_NaN();
        }

        if(isinf(a))
            return a;

        int e;                              //  0.5 <= r < 1.0
        dd_real r = frexp(a, &e);
        while(e % 2 != 0)
        {
            ++e;
            r = ldexp(r, -1);
        }

        // at this point, 0.25 <= r < 1.0
        //  TODO    improve the approximation to sqrt( r )
        //
        double y0 = 1.0 / sqrt(r._hi());
        dd_real x0 = r * y0;
        dd_real h0 = 0.5 * y0;
        dd_real r0 = Fma(-x0, h0, 0.5);
        dd_real x1 = Fma(x0, r0, x0);
        dd_real h1 = Fma(h0, r0, h0);
        dd_real r1 = Fma(-x1, h1, 0.5);
        dd_real x2 = Fma(x1, r1, x1);
        dd_real h2 = Fma(h1, r1, h1);

        return ldexp(x2, e / 2);                 //  more than enough accuracy
    }

}
