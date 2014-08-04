#ifndef __BEAENGINE_MACROS_H__
#define __BEAENGINE_MACROS_H__
/*
============================================================================
 Compiler Silencing macros

 Some compilers complain about parameters that are not used.  This macro
 should keep them quiet.
 ============================================================================
 */

# if defined (__GNUC__) && ((__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2)))
#   define BEA_UNUSED_ARG(a) (void) (a)
#elif defined (ghs) || defined (__GNUC__) || defined (__hpux) || defined (__sgi) || defined (__DECCXX) || defined (__rational__) || defined (__USLC__) || defined (BEA__RM544) || defined (__DCC__) || defined (__PGI) || defined (__TANDEM) || defined(__BORLANDC__)
/*
 Some compilers complain about "statement with no effect" with (a).
 This eliminates the warnings, and no code is generated for the null
 conditional statement.  Note, that may only be true if -O is enabled,
 such as with GreenHills (ghs) 1.8.8.
 */

# define BEA_UNUSED_ARG(a) do {/* null */} while (&a == 0)
#elif defined (__DMC__)
#if defined(__cplusplus)
#define BEA_UNUSED_ID(identifier)
template <class T>
inline void BEA_UNUSED_ARG(const T & BEA_UNUSED_ID(t)) { }
#else
#define BEA_UNUSED_ARG(a)
#endif
#else /* ghs || __GNUC__ || ..... */
# define BEA_UNUSED_ARG(a) (a)
#endif /* ghs || __GNUC__ || ..... */

#if defined (_MSC_VER) || defined(__sgi) || defined (ghs) || defined (__DECCXX) || defined(__BORLANDC__) || defined (BEA_RM544) || defined (__USLC__) || defined (__DCC__) || defined (__PGI) || defined (__TANDEM) || (defined (__HP_aCC) && (__HP_aCC >= 60500))
# define BEA_NOTREACHED(a)
#else  /* __sgi || ghs || ..... */
# define BEA_NOTREACHED(a) a
#endif /* __sgi || ghs || ..... */

#endif /* __BEAENGINE_MACROS_H__ */
