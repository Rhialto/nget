/*	This file is originally from OpenSSH v3.6.  The following is the
 	revelant information from the licence file:
 
	Remaining components of the software are provided under a standard
	2-term BSD licence with the following names as copyright holders:
  
	Markus Friedl
	Theo de Raadt
	Niels Provos
	Dug Song
	Aaron Campbell
	Damien Miller
	Kevin Steves
	Daniel Kouril
	Per Allansson

     * Redistribution and use in source and binary forms, with or without
     * modification, are permitted provided that the following conditions
     * are met:
     * 1. Redistributions of source code must retain the above copyright
     *    notice, this list of conditions and the following disclaimer.
     * 2. Redistributions in binary form must reproduce the above copyright
     *    notice, this list of conditions and the following disclaimer in the
     *    documentation and/or other materials provided with the distribution.
     *
     * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
     * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
     * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
     * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
     * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
     * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
     * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
     * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
     * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
     * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * fake library for ssh
 *
 * This file includes getaddrinfo(), freeaddrinfo() and gai_strerror().
 * These funtions are defined in rfc2133.
 *
 * But these functions are not implemented correctly. The minimum subset
 * is implemented for ssh use only. For exapmle, this routine assumes
 * that ai_family is AF_INET. Don't use it for another purpose.
 */

#include "fake-getaddrinfo.h"

//RCSID("$Id: fake-getaddrinfo.c,v 1.5 2003/03/24 02:35:59 djm Exp $");

#ifndef HAVE_GAI_STRERROR
char *gai_strerror(int ecode)
{
	switch (ecode) {
		case EAI_NODATA:
			return "no address associated with hostname.";
		case EAI_MEMORY:
			return "memory allocation failure.";
		default:
			return "unknown error.";
	}
}    
#endif /* !HAVE_GAI_STRERROR */

#ifndef HAVE_FREEADDRINFO
void freeaddrinfo(struct addrinfo *ai)
{
	struct addrinfo *next;

	do {
		next = ai->ai_next;
		free(ai);
	} while (NULL != (ai = next));
}
#endif /* !HAVE_FREEADDRINFO */

#ifndef HAVE_GETADDRINFO
static struct addrinfo *malloc_ai(int port, u_long addr)
{
	struct addrinfo *ai;

	ai = malloc(sizeof(struct addrinfo) + sizeof(struct sockaddr_in));
	if (ai == NULL)
		return(NULL);
	
	memset(ai, 0, sizeof(struct addrinfo) + sizeof(struct sockaddr_in));
	
	ai->ai_addr = (struct sockaddr *)(ai + 1);
	/* XXX -- ssh doesn't use sa_len */
	ai->ai_addrlen = sizeof(struct sockaddr_in);
	ai->ai_addr->sa_family = ai->ai_family = AF_INET;

	((struct sockaddr_in *)(ai)->ai_addr)->sin_port = port;
	((struct sockaddr_in *)(ai)->ai_addr)->sin_addr.s_addr = addr;
	
	return(ai);
}

int getaddrinfo(const char *hostname, const char *servname, 
                const struct addrinfo *hints, struct addrinfo **res)
{
	struct addrinfo *cur, *prev = NULL;
	struct hostent *hp;
	struct servent *sp;
	struct in_addr in;
	int i;
	long int port;
	u_long addr;

	port = 0;
	if (servname != NULL) {
		char *cp;

		port = strtol(servname, &cp, 10);
		if (port > 0 && port <= 65535 && *cp == '\0')
			port = htons(port);
		else if ((sp = getservbyname(servname, NULL)) != NULL)
			port = sp->s_port;
		else
			port = 0;
	}

	if (hints && hints->ai_flags & AI_PASSIVE) {
		addr = htonl(0x00000000);
		if (hostname && inet_aton(hostname, &in) != 0)
			addr = in.s_addr;
		if (NULL != (*res = malloc_ai(port, addr)))
			return 0;
		else
			return EAI_MEMORY;
	}
		
	if (!hostname) {
		if (NULL != (*res = malloc_ai(port, htonl(0x7f000001))))
			return 0;
		else
			return EAI_MEMORY;
	}
	
	if (inet_aton(hostname, &in)) {
		if (NULL != (*res = malloc_ai(port, in.s_addr)))
			return 0;
		else
			return EAI_MEMORY;
	}
	
	hp = gethostbyname(hostname);
	if (hp && hp->h_name && hp->h_name[0] && hp->h_addr_list[0]) {
		for (i = 0; hp->h_addr_list[i]; i++) {
			cur = malloc_ai(port, ((struct in_addr *)hp->h_addr_list[i])->s_addr);
			if (cur == NULL) {
				if (*res)
					freeaddrinfo(*res);
				return EAI_MEMORY;
			}
			
			if (prev)
				prev->ai_next = cur;
			else
				*res = cur;

			prev = cur;
		}
		return 0;
	}
	
	return EAI_NODATA;
}
#endif /* !HAVE_GETADDRINFO */
