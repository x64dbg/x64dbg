
#ifndef ZYDIS_EXPORT_H
#define ZYDIS_EXPORT_H

#ifdef ZYDIS_STATIC_DEFINE
#  define ZYDIS_EXPORT
#  define ZYDIS_NO_EXPORT
#else
#  ifndef ZYDIS_EXPORT
#    ifdef Zydis_EXPORTS
/* We are building this library */
#      define ZYDIS_EXPORT
#    else
/* We are using this library */
#      define ZYDIS_EXPORT
#    endif
#  endif

#  ifndef ZYDIS_NO_EXPORT
#    define ZYDIS_NO_EXPORT
#  endif
#endif

#ifndef ZYDIS_DEPRECATED
#  define ZYDIS_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef ZYDIS_DEPRECATED_EXPORT
#  define ZYDIS_DEPRECATED_EXPORT ZYDIS_EXPORT ZYDIS_DEPRECATED
#endif

#ifndef ZYDIS_DEPRECATED_NO_EXPORT
#  define ZYDIS_DEPRECATED_NO_EXPORT ZYDIS_NO_EXPORT ZYDIS_DEPRECATED
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define ZYDIS_NO_DEPRECATED
#endif

#endif
