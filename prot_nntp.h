/*
    prot_nntp.* - nntp protocol handler
    Copyright (C) 1999-2001  Matthew Mueller <donut@azstarnet.com>

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
};
struct arinfo {
	long Bps;
	time_t dtime;

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
		int authed;
		c_file_tcp sock;
		c_server *host;
		c_server *force_host;
		ulong curserverid;
		c_group_info::ptr group;
		int groupselected;//have we selected the group on the server?
//		c_nrange *grange;
		c_mid_info *midinfo;
		//c_nntp_cache *gcache;
		c_nntp_cache::ptr gcache;
		c_nntp_files_u *filec;
		time_t starttime;
		int derr;

		int stdputline(int echo,const char * str,...);
		int putline(int echo, const char * str,...)
			__attribute__ ((format (printf, 3, 4)));
		int doputline(int echo,const char * str,va_list ap);
		int getline(int echo);
		int getreply(int echo);
//		int stdgetreply(int echo);
		int chkreply(int reply);
		void doxover(ulong low, ulong high);
		void doxover(c_nrange *r);
//		void nntp_queueretrieve(const char *match, ulong linelimit, int getflags);
		void nntp_retrieve(const nget_options &options);
		void nntp_print_retrieving_headers(ulong lll,ulong hhh,ulong ccc,ulong bbb);
//		void nntp_print_retrieving_articles(long nnn, long tot,long done,long btot,long bbb);
//		void nntp_print_retrieving_articles(arinfo *ari, arinfo*tot);
		void nntp_group(c_group_info::ptr group, int getheaders, const nget_options &options);
		void nntp_dogroup(int getheaders);
		//void nntp_doarticle(long num,long ltotal,long btotal,char *fn);
		int nntp_doarticle_prioritize(c_nntp_part *part,t_nntp_server_articles_prioritized &sap,t_nntp_server_articles_prioritized::iterator *curservsapi);
		int nntp_dowritelite_article(c_file &fw,c_nntp_part *part,char *fn);
		int nntp_doarticle(c_nntp_part*part,arinfo*ari,quinfo*toti,char *fn, const nget_options &options);
		void nntp_dogetarticle(arinfo*ari,quinfo*toti,list<string> &buf);
		void nntp_auth(void);
		void nntp_doauth(const char *user, const char *pass);
		//void nntp_open(const char *h,const char *u,const char *p);
		void nntp_open(c_server *h);
		void nntp_doopen(void);
		void nntp_close(int fast=0);
		void cleanupcache(void);
		void cleanup(void);
		void initready(void);
		c_prot_nntp();
		~c_prot_nntp();
};

#endif
