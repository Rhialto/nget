/*
    litenntp.* - ngetlite nntp protocol handler
    Copyright (C) 2000-2002  Matthew Mueller <donut@azstarnet.com>

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
#ifndef _LITENNTP_H__
#define _LITENNTP_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include <sys/types.h>
#include <stdarg.h>

#include "buffysock.h"

class c_prot_nntp {
	public:
		char *cbuf;
		CharBuffer sockbuf;
//		int cursock;
		sockbuffy cursock;
		char *curhost;
		char *curgroup;
		char *curuser;
		char *curpass;

		int stdputline(int echo,const char * str,...);
		int putline(int echo, const char * str,...)
			__attribute__ ((format (printf, 3, 4)));
		int doputline(int echo,const char * str,va_list ap);
		int getline(int echo);
		int getreply(int echo);
//		int stdgetreply(int echo);
		int chkreply(int reply);

		void doarticle(ulong anum,ulong bytes,ulong lines,const char *outfile);

		void dogroup(const char *group);
		void doclose(void);
		void doopen(const char *host, const char *user, const char *pass);
		void nntp_auth(void);
		void nntp_doauth(const char *user, const char *pass);

		c_prot_nntp();
		~c_prot_nntp();
};
#endif
