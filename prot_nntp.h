/*
    prot_nntp.* - nntp protocol handler
    Copyright (C) 1999-2002  Matthew Mueller <donut@azstarnet.com>

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
#ifndef _PROT_NNTP_H_
#define _PROT_NNTP_H_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#include <string>
#include "cache.h"
//#include "nrange.h"
#include "file.h"
#include <stdarg.h>
#include "server.h"
#include "nrange.h"
#include "nget.h"
#include "sockpool.h"

#define GETFILES_NODECODE		8
#define GETFILES_KEEPTEMP		16
#define GETFILES_TEMPSHORTNAMES 32
#define GETFILES_NOCONNECT		64
#define GETFILES_TESTMODE		512
#define GETFILES_MARK			1024
#define GETFILES_UNMARK			2048

struct quinfo {
	int_fast64_t bytesleft;
	long filestot,filesdone;
	time_t starttime;
	t_show_multiserver doarticle_show_multi;
};
struct arinfo {
	long Bps;
	time_t dtime;

	const char *server_name;//if doarticle_show_multi
	long anum;
	long linesdone,linestot;
	long bytesdone,bytestot;
	time_t starttime;
	int partnum,partreq;
	void print_retrieving_articles(time_t curtime,quinfo*tot);
};

class c_prot_nntp /*: public c_transfer_protocol */{
	public:
//		int ch;
		char *cbuf;
//		int cbuf_size;
		Connection *connection;
		c_server *force_host;
		c_group_info::ptr group;
//		c_nrange *grange;
		c_mid_info *midinfo;
		//c_nntp_cache *gcache;
		c_nntp_cache::ptr gcache;
		c_nntp_files_u *filec;
		time_t starttime;
		int derr;

		int stdputline(int echo,const char * str,...)
			__attribute__ ((format (printf, 3, 4)));
		int putline(int echo, const char * str,...)
			__attribute__ ((format (printf, 3, 4)));
		int getline(int echo);
		int getreply(int echo);
//		int stdgetreply(int echo);
		int chkreply(int reply);
		void doxover(ulong low, ulong high);
		void doxover(c_nrange *r);
//		void nntp_queueretrieve(const char *match, ulong linelimit, int getflags);
		void nntp_retrieve(const nget_options &options);
		void nntp_group(c_group_info::ptr group, int getheaders, const nget_options &options);
		void nntp_dogroup(int getheaders);
		//void nntp_doarticle(long num,long ltotal,long btotal,char *fn);
		int nntp_doarticle_prioritize(c_nntp_part *part,t_nntp_server_articles_prioritized &sap,bool docurservmult);
		int nntp_dowritelite_article(c_file &fw,c_nntp_part *part,char *fn);
		int nntp_doarticle(c_nntp_part*part,arinfo*ari,quinfo*toti,char *fn, const nget_options &options);
		void nntp_dogetarticle(arinfo*ari,quinfo*toti,list<string> &buf);
		void nntp_auth(void);
		void nntp_doauth(const char *user, const char *pass);
		//void nntp_open(const char *h,const char *u,const char *p);
		void nntp_open(c_server *h);
		void nntp_doopen(void);
		void cleanupcache(void);
		void cleanup(void);
		void initready(void);
		c_prot_nntp();
		~c_prot_nntp();
};

#endif
