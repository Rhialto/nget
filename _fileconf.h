#ifndef ___FILECONF_H__
#define ___FILECONF_H__

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#ifndef S_IRWXG
#define S_IRWXG 0
#endif
#ifndef S_IRWXO
#define S_IRWXO 0
#endif
#ifndef S_IRGRP
#define S_IRGRP 0
#endif
#ifndef S_IROTH
#define S_IROTH 0
#endif

#define PRIVMODE (S_IRUSR | S_IWUSR)
#define PUBMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
//#define PUBXMODE (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define PUBXMODE (S_IRWXU|S_IRWXG|S_IRWXO)

#if HAVE_MKDIR
# if MKDIR_TAKES_ONE_ARG
/* Mingw32 */
#  define mkdir(a,b) mkdir(a)
# endif
#else
# if HAVE__MKDIR
/* plain Win32 */
#  define mkdir(a,b) _mkdir(a)
# else
#  error "Don't know how to create a directory on this system."
# endif
#endif

#endif
