/*
    prot_nntp.* - nntp protocol handler
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
#ifndef _PROT_NNTP_H_
#define _PROT_NNTP_H_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//#include <string>
#include "cache.h"
#include "grouplist.h"
//#include "nrange.h"
#include "file.h"
#include <stdarg.h>
#include "server.h"
#include "nrange.h"
#include "nget.h"
#include "sockpool.h"


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

class c_prot_nntp {
	public:
//		int ch;
		char *cbuf;
//		int cbuf_size;
		Connection *connection;
		c_server::ptr force_host;
		c_group_info::ptr group;
		meta_mid_info *midinfo;
		//c_nntp_cache *gcache;
		c_nntp_cache::ptr gcache;
		bool gcache_ismultiserver;
		
		c_nntp_grouplist::ptr glist;
		
		time_t starttime;

		int stdputline(int echo,const char * str,...)
			__attribute__ ((format (printf, 3, 4)));
		int putline(int echo, const char * str,...)
			__attribute__ ((format (printf, 3, 4)));
		int getline(int echo);
		int getreply(int echo);
//		int stdgetreply(int echo);
		int chkreply(int reply) const;
		int chkreply_setok(int reply);
		void doxover(const c_group_info::ptr &group, ulong low, ulong high);
		void doxover(const c_group_info::ptr &group, c_nrange *r);
		void doxpat(c_nrange &r, c_xpat::ptr xpat, ulong num, ulong low, ulong high);
		void dolistgroup(c_nrange &existing, ulong lowest, ulong highest, ulong total);
		void nntp_retrieve(const vector<c_group_info::ptr> &groups, const t_nntp_getinfo_list &getinfos, const nget_options &options);
		void nntp_retrieve(const vector<c_group_info::ptr> &rgroups, const t_nntp_getinfo_list &getinfos, const t_xpat_list &patinfos, const nget_options &options);
		void nntp_doretrieve(c_nntp_files_u &filec, ParHandler &parhandler, const nget_options &options);
		void nntp_simple_prioritize(c_server_priority_grouping *priogroup, list<c_server::ptr> &doservers);
		void nntp_group(const c_group_info::ptr &group, bool getheaders, const nget_options &options);
		void nntp_xgroup(const c_group_info::ptr &group, const t_xpat_list &patinfos, const nget_options &options);
		void nntp_dogroup(const c_group_info::ptr &group, ulong &num, ulong &low, ulong &high);
		void nntp_dogroup(const c_group_info::ptr &group, bool getheaders);
		void nntp_grouplist(int update, const nget_options &options);
		void nntp_xgrouplist(const t_xpat_list &patinfos, const nget_options &options);
		void nntp_dogetgrouplist(void);
		void nntp_dogrouplist(void);
		void nntp_dogrouplist(const char *wildmat);
		void nntp_dogroupdescriptions(const char *wildmat=NULL);
		void nntp_grouplist_search(const t_grouplist_getinfo_list &getinfos, const t_xpat_list &patinfos, const nget_options &options);
		void nntp_grouplist_search(const t_grouplist_getinfo_list &getinfos, const nget_options &options);
		int nntp_doarticle_prioritize(c_nntp_part *part,t_nntp_server_articles_prioritized &sap,bool docurservmult);
		int nntp_dowritelite_article(c_file &fw,c_nntp_part *part,char *fn);
		int nntp_doarticle(c_nntp_part*part,arinfo*ari,quinfo*toti,char *fn, const nget_options &options);
		void nntp_dogetarticle(arinfo*ari,quinfo*toti,list<string> &buf);
		void nntp_auth(void);
		void nntp_doauth(const char *user, const char *pass);
		void nntp_open(c_server::ptr h);
		void nntp_doopen(void);
		void cleanupcache(void);
		void cleanup(void);
		void initready(void);
		c_prot_nntp();
		~c_prot_nntp();
};

#endif
