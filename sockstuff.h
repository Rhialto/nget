/*
    sockstuff.* - socket handling code
    Copyright (C) 1999-2000,2002  Matthew Mueller <donut@azstarnet.com>

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
#include <sys/types.h>
#ifdef HAVE_WINSOCK_H
#include <winsock.h>
void sockstuff_init(void);
inline int sock_close(int s) {return closesocket(s);}
#define sock_errno WSAGetLastError()
#define sock_h_errno WSAGetLastError()
const char* sock_strerror(int e);
#define sock_hstrerror sock_strerror
#else /* !HAVE_WINSOCK_H */
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "strreps.h"
#define sockstuff_init()
inline int sock_close(int s) {return close(s);}
#define sock_errno errno
#define sock_h_errno h_errno
inline const char* sock_strerror(int e) {return strerror(e);}
inline const char* sock_hstrerror(int e) {return hstrerror(e);}
#endif /* !HAVE_WINSOCK_H */
#include <stdarg.h>

inline int sock_write(int sockfd, const char *buf, size_t count) {
	//return write(sockfd, buf, count);
	return send(sockfd, buf, count, 0);
}

extern int sock_timeout;

void atosockaddr(const char *netaddress, const char *defport, const char *proto,struct sockaddr_in *address, char * buf, int buflen);

int atoport(const char *service,const char *proto,char * buf, int buflen);
void atoaddr(const char *netaddress,struct in_addr *addr,char *buf, int buflen);

int make_connection(int type,const char *netaddress,const char *service,char * buf, int buflen);

int getsocketaddress(int s, struct sockaddr_in *addr);
int get_connection1(int socket_type, u_short port);
int get_connection2(int socket_type,int gc_listening_socket);
//int get_connection2(int socket_type);
int sock_write_ensured(int sockfd, const char *buf, size_t count);
int sock_read(int sockfd, void *buf, size_t count);
bool sock_datawaiting(int sockfd);

#endif
