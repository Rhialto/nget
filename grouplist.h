/*
    grouplist.* - nntp newsgroups list cache code
    Copyright (C) 2002  Matthew Mueller <donut@azstarnet.com>

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
#ifndef _GROUPSLIST_H_
#define _GROUPSLIST_H_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "rcount.h"
#include <string>
#include <set>
#include <map>
#include "stlhelp.h"
#include "etree.h"

#define GLCACHE_VERSION "NGETGL1"

class c_grouplist_getinfo : public c_refcounted<c_grouplist_getinfo>{
	public:
		nntp_grouplist_pred *pred;
		int flags;
		c_grouplist_getinfo(nntp_grouplist_pred *pre,int flag):pred(pre), flags(flag) { }
		~c_grouplist_getinfo() { delete pred; }
};
typedef list<c_grouplist_getinfo::ptr> t_grouplist_getinfo_list;


class c_nntp_grouplist_server_info : public c_refcounted<c_nntp_grouplist_server_info> {
	public:
		ulong serverid;
		string date;
		c_nntp_grouplist_server_info(ulong sid):serverid(sid) {}
		c_nntp_grouplist_server_info(ulong sid,string dat):serverid(sid),date(dat) {}
};
typedef map<ulong,c_nntp_grouplist_server_info::ptr> t_nntp_grouplist_server_info_map;


class c_server_group_description : public c_refcounted<c_server_group_description> {
	public:
		string description;
		set<ulong> serverids;
		c_server_group_description(const string &desc):description(desc){}
};
typedef map<const char*,c_server_group_description::ptr, ltstr> t_server_group_description_map;

class c_group_availability : public c_refcounted<c_group_availability>{
	public:
		string groupname;
		set<ulong> serverids;
		t_server_group_description_map servergroups;
		bool addserverdescription(ulong serverid, const string &desc);
		c_group_availability(const string &name):groupname(name){}
};
typedef map<const char*, c_group_availability::ptr, ltstr> t_group_availability_map;


class c_nntp_grouplist : public c_refcounted<c_nntp_grouplist>{
	protected:
		string filename;
		bool saveit;
		t_nntp_grouplist_server_info_map server_info;
		t_group_availability_map groups;
	public:
		
		c_nntp_grouplist_server_info::ptr getserverinfo(ulong serverid);
		void addgroup(ulong serverid, string name);
		void addgroupdesc(ulong serverid, string name, string desc);
		void printinfos(const t_grouplist_getinfo_list &getinfos);

		c_nntp_grouplist(void);
		void save(void);
		~c_nntp_grouplist();
};

void nntp_grouplist_printinfos(const t_grouplist_getinfo_list &getinfos);

#endif
