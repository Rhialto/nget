/*
    sockstuff.* - socket handling code
    Copyright (C) 1999-2004  Matthew Mueller <donut AT dakotacom.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef _SOCKSTUFF_H_
#define _SOCKSTUFF_H_
#ifdef HAVE_CONFIG_H 
#include "config.h"
#endif
#include "compat/socketheaders.h"
#ifdef HAVE_WINSOCK_H
typedef SOCKET sock_t;
inline bool sock_isvalid(sock_t s) {return s!=INVALID_SOCKET;}
#define SOCK_INVALID INVALID_SOCKET
void sockstuff_init(void);
inline int sock_close(sock_t s) {return closesocket(s);}
#define sock_errno WSAGetLastError()
#define sock_h_errno WSAGetLastError()
const char* sock_strerror(int e);
#define sock_hstrerror sock_strerror
#else /* !HAVE_WINSOCK_H */
#include "strreps.h"
typedef int sock_t;
inline bool sock_isvalid(sock_t s) {return s>=0;}
#define SOCK_INVALID -1
#define sockstuff_init()
inline int sock_close(sock_t s) {return close(s);}
#define sock_errno errno
#define sock_h_errno h_errno
inline const char* sock_strerror(int e) {return strerror(e);}
inline const char* sock_hstrerror(int e) {return hstrerror(e);}
#endif /* !HAVE_WINSOCK_H */
#include <stdarg.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

inline int sock_write(sock_t sockfd, const char *buf, size_t count) {
	//return write(sockfd, buf, count);
	return send(sockfd, buf, count, MSG_NOSIGNAL);
}

extern int sock_timeout;

sock_t make_connection(const char *netaddress,const char *service);

int sock_write_ensured(sock_t sockfd, const char *buf, size_t count);
int sock_read(sock_t sockfd, void *buf, size_t count);
bool sock_datawaiting(sock_t sockfd);

#endif
