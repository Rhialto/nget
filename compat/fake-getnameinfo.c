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
 * This file includes getnameinfo().
 * These funtions are defined in rfc2133.
 *
 * But these functions are not implemented correctly. The minimum subset
 * is implemented for ssh use only. For exapmle, this routine assumes
 * that ai_family is AF_INET. Don't use it for another purpose.
 */

#include "fake-getnameinfo.h"

//RCSID("$Id: fake-getnameinfo.c,v 1.2 2001/02/09 01:55:36 djm Exp $");

#ifndef HAVE_GETNAMEINFO
int getnameinfo(const struct sockaddr *sa, size_t salen, char *host, 
                size_t hostlen, char *serv, size_t servlen, int flags)
{
	struct sockaddr_in *sin = (struct sockaddr_in *)sa;
	struct hostent *hp;
	char tmpserv[16];

	if (serv) {
		snprintf(tmpserv, sizeof(tmpserv), "%d", ntohs(sin->sin_port));
		if (strlen(tmpserv) >= servlen)
			return EAI_MEMORY;
		else
			strcpy(serv, tmpserv);
	}

	if (host) {
		if (flags & NI_NUMERICHOST) {
			if (strlen(inet_ntoa(sin->sin_addr)) >= hostlen)
				return EAI_MEMORY;

			strcpy(host, inet_ntoa(sin->sin_addr));
			return 0;
		} else {
			hp = gethostbyaddr((char *)&sin->sin_addr, 
				sizeof(struct in_addr), AF_INET);
			if (hp == NULL)
				return EAI_NODATA;
			
			if (strlen(hp->h_name) >= hostlen)
				return EAI_MEMORY;

			strcpy(host, hp->h_name);
			return 0;
		}
	}
	return 0;
}
#endif /* !HAVE_GETNAMEINFO */
