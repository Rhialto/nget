#ifndef _COMPAT_SOCKETHEADERS_H_
#define _COMPAT_SOCKETHEADERS_H_


#ifdef HAVE_CONFIG_H 
# include "../config.h"
#endif

#include <sys/types.h>

#ifdef WIN32
# define WIN32_LEAN_AND_MEAN
# undef NOMINMAX
# define NOMINMAX 1
# ifdef HAVE_WINSOCK2_H
#  include <winsock2.h>
#  ifdef HAVE_WS2TCPIP_H
#   include <ws2tcpip.h>
#  endif
# else
#  include <winsock.h>
# endif
#else /* !WIN32 */
# include <unistd.h>
# include <sys/socket.h>
# include <netdb.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <errno.h>
# include <string.h>
#endif


#endif
