/* get rid of stupid undefined errors for (v)asprintf */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef HAVE_LIBZ
#undef CHECKSUM
#endif

#include <assert.h>

#define GETOPT_ARGV_T (char * const *)

#ifdef POPT_CONST_ARGV
#define POPT_NAME_T
#define POPT_ARGV_T
#define POPT_ARGV_p_T
#else
#define POPT_NAME_T (char *)
#define POPT_ARGV_T (char **)
#define POPT_ARGV_p_T (char ***)
#endif
/*#ifdef CHECKSUM
#define CACHE_USE_CHECKSUM
#else
#undef SHORT_TEMPNAMES
#endif*/

using namespace std;

#ifdef HAVE_INTTYPES_H
# ifndef __STDC_FORMAT_MACROS
# define __STDC_FORMAT_MACROS
# endif
# include <inttypes.h>
#elif HAVE_STDINT_H
# include <stdint.h>
#endif
#ifndef HAVE_INT_FAST64_T
# undef SIZEOF_INT_FAST64_T
# ifdef HAVE_LONG_LONG
#  define SIZEOF_INT_FAST64_T SIZEOF_LONG_LONG
#  define int_fast64_t long long
#  define uint_fast64_t unsigned long long
# else
//well, they may not be 64 bits but at least it should still work.
#  define SIZEOF_INT_FAST64_T SIZEOF_LONG
#  define int_fast64_t long
#  define uint_fast64_t unsigned long
# endif
#endif
#ifndef PRIuFAST64
# ifdef HAVE_LONG_LONG
#  define PRIuFAST64 "llu"
# else
#  define PRIuFAST64 "lu"
# endif
#endif

#ifdef WIN32
#define sleep _sleep
#endif
