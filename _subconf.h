/* get rid of stupid undefined errors for (v)asprintf */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* If we're not using GNU C, elide __attribute__ */
#ifndef __GNUC__
#  define  __attribute__(x)  /*NOTHING*/
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

#ifdef __cplusplus
using namespace std;
#endif

#ifdef HAVE_INTTYPES_H
# ifndef __STDC_FORMAT_MACROS
# define __STDC_FORMAT_MACROS
# endif
# ifndef __STDC_CONSTANT_MACROS
# define __STDC_CONSTANT_MACROS
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

#ifndef HAVE_INT32_T
# define int32_t int
#endif
#ifndef HAVE_UINT32_T
# define uint32_t unsigned int
#endif
#ifndef HAVE_INT64_T
# define int64_t int_fast64_t
#endif
#ifndef HAVE_UINT64_T
# define uint64_t uint_fast64_t
# if (SIZEOF_INT_FAST64_T!=8)
#  error "my uint64_t isn't 8 bytes."
# endif
#endif
#ifndef HAVE_INTPTR_T
typedef int intptr_t;
#endif
#ifndef HAVE_UINTPTR_T
typedef unsigned int uintptr_t;
#endif


#define	SWAP16(type)  ((((type) >> 8) & 0x00ff) | \
					(((type) << 8) & 0xff00))

#define	SWAP32(type)  ((((type) >>24) & 0x000000ff) | \
					(((type) >> 8) & 0x0000ff00) | \
					(((type) << 8) & 0x00ff0000) | \
					(((type) <<24) & 0xff000000))

#define	SWAP64(type)  ((((type) >>56) & 0x00000000000000ff) | \
					(((type) >>40) & 0x000000000000ff00) | \
					(((type) >>24) & 0x0000000000ff0000) | \
					(((type) >> 8) & 0x00000000ff000000) | \
					(((type) << 8) & UINT64_C(0x000000ff00000000)) | \
					(((type) <<24) & UINT64_C(0x0000ff0000000000)) | \
					(((type) <<40) & UINT64_C(0x00ff000000000000)) | \
					(((type) <<56) & UINT64_C(0xff00000000000000)))

#ifdef WIN32
#define sleep(x) _sleep(x*1000)
#endif
