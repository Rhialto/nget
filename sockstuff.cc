/*
    sockstuff.* - socket handling code
    Copyright (C) 1999-2003  Matthew Mueller <donut AT dakotacom.net>

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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "sockstuff.h"

#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

extern "C" {
#include "compat/fake-getaddrinfo.h"
#include "compat/fake-getnameinfo.h"
}
#include "log.h"
#include "strreps.h"
#include "file.h"
#include "myregex.h"

//comment the next line out if by chance you actually don't have select.
#define HAVE_SELECT

int sock_timeout=3*60;

#ifdef HAVE_WINSOCK_H
static void sockstuff_cleanup(void){
	WSACleanup();
}
void sockstuff_init(void){
	WORD wVersionRequested;
	WSADATA wsaData;
	int r;

	wVersionRequested = MAKEWORD(2, 0);
	if ((r=WSAStartup( wVersionRequested, &wsaData)))
	{
		throw FileEx(Ex_INIT,"WSAStartup error: %i",r);
	}
	if ( LOBYTE( wsaData.wVersion ) != 2 ||
			HIBYTE( wsaData.wVersion ) != 0 ) {
		/* We couldn't find a usable WinSock DLL. */
		WSACleanup( );
		throw FileEx(Ex_INIT,"bad winsock version %i.%i", LOBYTE( wsaData.wVersion ) , HIBYTE( wsaData.wVersion ));
	}

	atexit(sockstuff_cleanup);
}
//why the !*#&$ doesn't winsock have a strerror function???
const char* sock_strerror(int e) {
	switch (e) {
		case WSAEINTR:	return "WSAEINTR";
		case WSAEBADF:	return "WSAEBADF";
		case WSAEACCES:	return "WSAEACCES";
		case WSAEFAULT:	return "WSAEFAULT";
		case WSAEINVAL:	return "WSAEINVAL";
		case WSAEMFILE:	return "WSAEMFILE";
		case WSAEWOULDBLOCK:	return "WSAEWOULDBLOCK";
		case WSAEINPROGRESS:	return "WSAEINPROGRESS";
		case WSAEALREADY:	return "WSAEALREADY";
		case WSAENOTSOCK:	return "WSAENOTSOCK";
		case WSAEDESTADDRREQ:	return "WSAEDESTADDRREQ";
		case WSAEMSGSIZE:	return "WSAEMSGSIZE";
		case WSAEPROTOTYPE:	return "WSAEPROTOTYPE";
		case WSAENOPROTOOPT:	return "WSAENOPROTOOPT";
		case WSAEPROTONOSUPPORT:	return "WSAEPROTONOSUPPORT";
		case WSAESOCKTNOSUPPORT:	return "WSAESOCKTNOSUPPORT";
		case WSAEOPNOTSUPP:	return "WSAEOPNOTSUPP";
		case WSAEPFNOSUPPORT:	return "WSAEPFNOSUPPORT";
		case WSAEAFNOSUPPORT:	return "WSAEAFNOSUPPORT";
		case WSAEADDRINUSE:	return "WSAEADDRINUSE";
		case WSAEADDRNOTAVAIL:	return "WSAEADDRNOTAVAIL";
		case WSAENETDOWN:	return "WSAENETDOWN";
		case WSAENETUNREACH:	return "WSAENETUNREACH";
		case WSAENETRESET:	return "WSAENETRESET";
		case WSAECONNABORTED:	return "WSAECONNABORTED";
		case WSAECONNRESET:	return "WSAECONNRESET";
		case WSAENOBUFS:	return "WSAENOBUFS";
		case WSAEISCONN:	return "WSAEISCONN";
		case WSAENOTCONN:	return "WSAENOTCONN";
		case WSAESHUTDOWN:	return "WSAESHUTDOWN";
		case WSAETOOMANYREFS:	return "WSAETOOMANYREFS";
		case WSAETIMEDOUT:	return "WSAETIMEDOUT";
		case WSAECONNREFUSED:	return "WSAECONNREFUSED";
		case WSAELOOP:	return "WSAELOOP";
		case WSAENAMETOOLONG:	return "WSAENAMETOOLONG";
		case WSAEHOSTDOWN:	return "WSAEHOSTDOWN";
		case WSAEHOSTUNREACH:	return "WSAEHOSTUNREACH";
		case WSAENOTEMPTY:	return "WSAENOTEMPTY";
		case WSAEPROCLIM:	return "WSAEPROCLIM";
		case WSAEUSERS:	return "WSAEUSERS";
		case WSAEDQUOT:	return "WSAEDQUOT";
		case WSAESTALE:	return "WSAESTALE";
		case WSAEREMOTE:	return "WSAEREMOTE";
		case WSAEDISCON:	return "WSAEDISCON";
		case WSASYSNOTREADY:	return "WSASYSNOTREADY";
		case WSAVERNOTSUPPORTED:	return "WSAVERNOTSUPPORTED";
		case WSANOTINITIALISED:	return "WSANOTINITIALISED";
		case WSAHOST_NOT_FOUND:	return "WSAHOST_NOT_FOUND";
		case WSATRY_AGAIN:	return "WSATRY_AGAIN";
		case WSANO_RECOVERY:	return "WSANO_RECOVERY";
		case WSANO_DATA:	return "WSANO_DATA";
		default:	return "unknown winsock error";
	}
}
#endif


static c_regex_r rfc2732host("\\[(.*)\\](:(.*))?"),
	rawipv6host(".*:.*:.*"),
	normalhost("([^:]*)(:(.*))?");
void parse_host(string &saddr, string &sservice, const string &host, const string &defservice="") {
	sservice=defservice;
	c_regex_subs subs;
	if (!rfc2732host.match(host,&subs)) {
		//actually, rfc2732 indicates that inside the []'s is an ipv6 address only, not ipv4 or a hostname.  But that would be harder to implement and not as flexible anyway ;)
		saddr=subs.subs(1);
		if (subs.sublen(2)>0)
			sservice=subs.subs(3);
	}
	else if (!rawipv6host.match(host,&subs)) {
		saddr=host;
	}
	else if (!normalhost.match(host,&subs)) {
		saddr=subs.subs(1);
		if (subs.sublen(2)>0)
			sservice=subs.subs(3);
	} else
		throw FileEx(Ex_INIT,"parse_host error"); //shouldn't happen.
}

/* This is a generic function to make a stream connection to a given server/port.
service is the default port name/number (can be overridden in netaddress),
netaddress is the host name to connect to.
The function returns the socket, ready for action.*/
sock_t make_connection(const char *netaddress,const char *service){
	PERROR("make_connection(%s,%s)", netaddress, service);
	sock_t sock;
	int connected;
	const char *lasterrorfunc="",*lasterror=""; //save the previous error message so we can include something nice in the exception if all connections fail.  Maintains compatibility in the case of hosts that resolve to a single address, and in the case of resolving to multiple, at least it's something. heh.
	
	string saddr,sservice;
	parse_host(saddr, sservice, netaddress, service);

	struct addrinfo hints, *res, *res0;
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	int error;
	
	/* resolve address/port into sockaddr */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(saddr.c_str(), sservice.c_str(), &hints, &res0);
	if (error)
		throw FileEx(Ex_INIT, "getaddrinfo: %s", gai_strerror(error));

	/* try all the sockaddrs until connection goes successful */
	for (res = res0; res; res = res->ai_next) {
		error = getnameinfo(res->ai_addr, res->ai_addrlen, hbuf,
				sizeof(hbuf), sbuf, sizeof(sbuf),
				NI_NUMERICHOST | NI_NUMERICSERV);
		if (error) {
			PMSG_nnl("Connecting to <getnameinfo error: %s> ...", gai_strerror(error));
		} else
			PMSG_nnl("Connecting to [%s]:%s ... ",hbuf,sbuf);

		sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (!sock_isvalid(sock)) {
			lasterrorfunc="socket";lasterror=sock_strerror(sock_errno);
			PMSG("%s: %s",lasterrorfunc, lasterror);
			continue;
		}

#if defined(HAVE_SELECT) && defined(HAVE_FCNTL)
		fcntl(sock,F_SETFL,O_NONBLOCK);//we can set the sock to nonblocking, and use select to enforce a timeout.
#endif

		connected = connect(sock, res->ai_addr, res->ai_addrlen);

#if defined(HAVE_SELECT) && defined(HAVE_FCNTL)
		if (connected<0 && sock_errno==EINPROGRESS){
			fd_set w;
			struct timeval tv;
			int i;
			tv.tv_sec=sock_timeout;
			tv.tv_usec=0;
			FD_ZERO(&w);
			FD_SET(sock,&w);
			if ((i=select(sock+1,NULL,&w,NULL,&tv))>0){
				int erp;
				socklen_t erl=sizeof(erp);
				if (getsockopt(sock,SOL_SOCKET,SO_ERROR,&erp,&erl)){
					lasterrorfunc="getsockopt";lasterror=sock_strerror(sock_errno);
					PMSG("%s: %s",lasterrorfunc, lasterror);
					sock_close(sock);
					continue;
				}
				if (erp) {
					lasterrorfunc="connect";lasterror=sock_strerror(erp);
					PMSG("%s: %s",lasterrorfunc, lasterror);
					sock_close(sock);
					continue;
				}
			}else{
				lasterrorfunc="make_connection";lasterror="timeout reached";
				PMSG("%s: %s",lasterrorfunc, lasterror);
				sock_close(sock);
				continue;
			}
			fcntl(sock,F_SETFL,0);//set to blocking again (I hope)
			connected=0;
		}
#endif
		if (connected < 0) {
			lasterrorfunc="connect";lasterror=sock_strerror(sock_errno);
			PMSG("%s: %s",lasterrorfunc, lasterror);
			sock_close(sock);
			continue;
		}
		freeaddrinfo(res0);
		PMSG("connected.");
		return sock;
	}

	freeaddrinfo(res0);
	throw FileEx(Ex_INIT, "%s: %s",lasterrorfunc,lasterror);
}



/* This is just like the write() system call, accept that it will
make sure that all data is transmitted. */
int sock_write_ensured(sock_t sockfd, const char *buf, size_t count) {
	size_t bytes_sent = 0;
	int this_write;

	while (bytes_sent < count) {
		//    do
		this_write = sock_write(sockfd, buf, count - bytes_sent);
		//    while ( (this_write < 0) && (sock_errno == EINTR) );
		if (this_write <= 0)
			return this_write;
		bytes_sent += this_write;
		buf += this_write;
	}
	return count;
}


//like read(), but uses select() to enforce a timeout.  read() blocks forever if the connection is lost unexpectadly (modem disconnect, etc)
int sock_read(sock_t sockfd, void *buf, size_t count){
#ifdef HAVE_SELECT
	fd_set r;
	struct timeval tv;
	int i;
	tv.tv_sec=sock_timeout;
	tv.tv_usec=0;
	FD_ZERO(&r);
	FD_SET(sockfd,&r);
	if ((i=select(sockfd+1,&r,NULL,NULL,&tv))>0){
#endif
		//return read(sockfd,buf,count);
		return recv(sockfd, (RECV_ARG2)buf, count, MSG_NOSIGNAL);
#ifdef HAVE_SELECT
	}
	if (i==0)
		throw FileEx(Ex_INIT,"sock_read timeout reached (%is)", sock_timeout);
	return i;
#endif
}

bool sock_datawaiting(sock_t sockfd){
#ifdef HAVE_SELECT
	fd_set r;
	struct timeval tv;
	int i;
	tv.tv_sec=0;
	tv.tv_usec=0;
	FD_ZERO(&r);
	FD_SET(sockfd,&r);
	if ((i=select(sockfd+1,&r,NULL,NULL,&tv))>0){
		return true;
	}
#endif
	return false;
}

