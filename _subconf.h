/* get rid of stupid undefined errors for (v)asprintf */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef HAVE_SLIST_H
#define slist list
#define SLIST_H <list.h>
#else
#define SLIST_H <slist.h>
#endif

#ifndef HAVE_LIBZ
#undef CHECKSUM
#endif

#include <assert.h>

#ifdef POPT_CONST_ARGV
#define POPT_ARGV_T (const char **)
#define POPT_ARGV_p_T (const char ***)
#else
#define POPT_ARGV_T
#define POPT_ARGV_p_T
#endif
/*#ifdef CHECKSUM
#define CACHE_USE_CHECKSUM
#else
#undef SHORT_TEMPNAMES
#endif*/

using namespace std;
