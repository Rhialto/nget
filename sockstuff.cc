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

#include "log.h"
#include "strreps.h"
#include "file.h"

//not all systems have this defined...
#ifndef INADDR_NONE
#define INADDR_NONE (-1)
#endif

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


/* Take a service name, and a service type, and return a port number.  If the
service name is not found, it tries it as a decimal number.  The number
returned is byte ordered for the network. */

int atoport(const char *service, const char *proto,char * buf, int buflen) {
	int port;
	long int lport;
	struct servent *serv=NULL;
	char *errpos;
	/* First try to read it from /etc/services */
#if HAVE_FUNC_GETSERVBYNAME_R_6
	struct servent servb;
	getservbyname_r(service,proto,&servb,buf,buflen,&serv);
#elif HAVE_FUNC_GETSERVBYNAME_R_5
	struct servent servb;
	serv = getservbyname_r(service,proto,&servb,buf,buflen);
#elif HAVE_FUNC_GETSERVBYNAME_R_4
	assert(buflen >= sizeof(struct servent_data));
	struct servent servb;
	if (getservbyname_r(service,proto,&servb,(struct servent_data *)buf)==0)
		serv = &servb;
#else
	serv = getservbyname(service, proto);
#endif
	if (serv != NULL)
		port = serv->s_port;
	else { /* Not in services, maybe a number? */
		lport = strtol(service,&errpos,0);
		if ( (errpos[0] != 0) || (lport < 1) || (lport > 65535) )
			throw FileEx(Ex_INIT,"atoport: invalid port number or unknown service %s",service);
		port = htons(lport);
	}
	return port;
}

static int do_gethostbyname(const char *netaddress,struct in_addr *addr,char *buf, int buflen){
	struct hostent *host=NULL;
#if HAVE_FUNC_GETHOSTBYNAME_R_6
	int err;
	struct hostent hostb;
	gethostbyname_r(netaddress,&hostb,buf,buflen,&host,&err);
#elif HAVE_FUNC_GETHOSTBYNAME_R_5
	int err;
	struct hostent hostb;
	host = gethostbyname_r(netaddress,&hostb,buf,buflen,&err);
#elif HAVE_FUNC_GETHOSTBYNAME_R_3
	assert(buflen >= sizeof(struct hostent_data));
	struct hostent hostb;
	if (gethostbyname_r(netaddress,&hostb,(struct hostent_data *)buf)==0)
		host = &hostb;
#else
	host = gethostbyname(netaddress);
#endif
	if (host != NULL) {
		//			addr=(struct in_addr *)*host->h_addr_list;
		memcpy(addr,(struct in_addr *)*host->h_addr_list,sizeof(struct in_addr));
		return 1;
	}
	else return 0;
}

void atoaddr(const char *netaddress,struct in_addr *addr,char *buf, int buflen){
	if
#ifdef HAVE_INET_ATON
		(inet_aton(netaddress,addr))
#else
		((addr->s_addr=inet_addr(netaddress))!=INADDR_NONE)
#endif
			return;
	else {
		int r;
		if (!(r=do_gethostbyname(netaddress, addr, buf, buflen)) && sock_errno==ERANGE) {
			buf = NULL;
			do {
				buflen *= 2;
				buf = (char*)realloc(buf, buflen);
			} while (!(r=do_gethostbyname(netaddress, addr, buf, buflen)) && sock_errno==ERANGE);
			free(buf);
		}
		if (r==0)
			throw FileEx(Ex_INIT,"gethostbyname: %s",sock_hstrerror(sock_h_errno));
	}
}

void atosockaddr(const char *netaddress, const char *defport, const char *proto,struct sockaddr_in *address, char * buf, int buflen){
	const char *p;
	string a;
	address->sin_family = AF_INET;
	if ((p=strrchr(netaddress,':')))
			defport=p+1;
	int port=atoport(defport,proto,buf,buflen);
	address->sin_port=port;
	if (p){
		a.assign(netaddress, p-netaddress);
		netaddress=a.c_str();
	}
	atoaddr(netaddress,&address->sin_addr,buf,buflen);
}

/* This is a generic function to make a connection to a given server/port.
service is the port name/number,
type is either SOCK_STREAM or SOCK_DGRAM, and
netaddress is the host name to connect to.
The function returns the socket, ready for action.*/
int make_connection(int type,const char *netaddress,const char *service,char * buf, int buflen){
	PERROR("make_connection(%i,%s,%s,%p,%i)",type, netaddress, service,buf, buflen);
	int sock, connected;
	struct sockaddr_in address;
	char *prot;
	if (type == SOCK_STREAM)
		prot="tcp";
	else if (type == SOCK_DGRAM)
		prot="udp";
	else 
		throw FileEx(Ex_INIT,"make_connection:  Invalid prot type %i",type);

	memset((char *) &address, 0, sizeof(address));
	atosockaddr(netaddress,service,prot,&address,buf,buflen);

	unsigned char *i;
	i=(unsigned char *)&address.sin_addr.s_addr;
	PMSG("Connecting to %i.%i.%i.%i:%i",i[0],i[1],i[2],i[3],ntohs(address.sin_port));

	sock = socket(address.sin_family, type, 0);
	if (sock < 0)
		throw FileEx(Ex_INIT,"socket: %s",sock_strerror(sock_errno));

	if (type == SOCK_STREAM) {
#if defined(HAVE_SELECT) && defined(HAVE_FCNTL)
		fcntl(sock,F_SETFL,O_NONBLOCK);//we can set the sock to nonblocking, and use select to enforce a timeout.
#endif
		connected = connect(sock, (struct sockaddr *) &address,
				sizeof(address));
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
					sock_close(sock);
					throw FileEx(Ex_INIT,"getsockopt: %s",sock_strerror(sock_errno));
				}
				if (erp) {
					sock_close(sock);
					throw FileEx(Ex_INIT,"connect: %s",sock_strerror(erp));
				}
			}else{
				sock_close(sock);
				throw FileEx(Ex_INIT,"make_connection timeout reached (%is)", sock_timeout);
			}
			fcntl(sock,F_SETFL,0);//set to blocking again (I hope)
			connected=0;
		}
#endif
		if (connected < 0) {
			sock_close(sock);
			throw FileEx(Ex_INIT,"connect: %s",sock_strerror(sock_errno));
		}
		return sock;
	}
	/* Otherwise, must be for udp, so bind to address. */
	if (bind(sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
		sock_close(sock);
		throw FileEx(Ex_INIT,"bind: %s",sock_strerror(sock_errno));
	}
	return sock;
}





int getsocketaddress(int s, struct sockaddr_in *addr){
	socklen_t len = sizeof(struct sockaddr_in);

	if (getsockname(s, (struct sockaddr *)addr, &len) < 0) {
		PERROR("getsockname: %s",sock_strerror(sock_errno));
		return -1;
	}
	return 0;
}       /* GetSocketAddress */






/* This function listens on a port, and returns connections. 
* 
*  The parameters are as follows:
*    socket_type: SOCK_STREAM or SOCK_DGRAM (TCP or UDP sockets)
*    port: The port to listen on.  Remember that ports < 1000 are
*	reserved for the root user.  Must be passed in network byte
*	order (see "man htons").
*/

int get_connection1(int socket_type, u_short port) {

	int gc_listening_socket;
	struct sockaddr_in gc_address;
	int gc_reuse_addr = 1;


	/* Setup internet address information.  
	   This is used with the bind() call */
	memset((char *) &gc_address, 0, sizeof(gc_address));
	gc_address.sin_family = AF_INET;
	gc_address.sin_port = port;
	gc_address.sin_addr.s_addr = htonl(INADDR_ANY);

	gc_listening_socket = socket(AF_INET, socket_type, 0);
	if (gc_listening_socket < 0) {
		PERROR("socket: %s",sock_strerror(sock_errno));
		return(-1);
	}

	setsockopt(gc_listening_socket, SOL_SOCKET, SO_REUSEADDR, (SETSOCKOPT_ARG4)&gc_reuse_addr,
			sizeof(gc_reuse_addr));

	if (bind(gc_listening_socket, (struct sockaddr *) &gc_address, 
				sizeof(gc_address)) < 0) {
		PERROR("bind: %s",sock_strerror(sock_errno));
		sock_close(gc_listening_socket);
		return (-2);
	}
	if (socket_type == SOCK_STREAM)
		listen(gc_listening_socket, 1);
	return gc_listening_socket;
}
int get_connection2(int socket_type,int gc_listening_socket)
{
	int connected_socket = -1;
	if (socket_type == SOCK_STREAM) {

		while(connected_socket < 0) {
			connected_socket = accept(gc_listening_socket, NULL, NULL);
			if (connected_socket < 0) {
				/* Either a real error occured, or blocking was interrupted for
				   some reason.  Only abort execution if a real error occured. */
				if (sock_errno != EINTR) {
					PERROR("accept: %s",sock_strerror(sock_errno));
					sock_close(gc_listening_socket);
					return (-3);
				} else {
					PERROR("accept1: %s",sock_strerror(sock_errno));
					continue;    /* don't fork - do the accept again */
				}
			}

			//	sock_close(gc_listening_socket); /* sock_close our copy of this socket */

		}
		sock_close(gc_listening_socket);
		return connected_socket;
	}
	else
		return gc_listening_socket;
}




/* This is just like the write() system call, accept that it will
make sure that all data is transmitted. */
int sock_write_ensured(int sockfd, const char *buf, size_t count) {
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
int sock_read(int sockfd, void *buf, size_t count){
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
		return recv(sockfd, (RECV_ARG2)buf, count, 0);
#ifdef HAVE_SELECT
	}
	if (i==0)
		throw FileEx(Ex_INIT,"sock_read timeout reached (%is)", sock_timeout);
	return i;
#endif
}

bool sock_datawaiting(int sockfd){
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

