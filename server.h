/*
    server.* - nget configuration handling
    Copyright (C) 2000-2003  Matthew Mueller <donut AT dakotacom.net>

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
#ifndef _SERVER_H_
#define _SERVER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <map>
#include <vector>
#include <time.h>
#include "stlhelp.h"
#include "cfgfile.h"
#include "rcount.h"
#include "strtoker.h"

int parse_int_pair(const char *s, int *l, int *h);


class c_server : public c_refcounted<c_server>{
	public:
		ulong serverid;
		string alias;
		string shortname;
		string addr;
		string user,pass;
		string bindaddr;
		int fullxover;
		int maxstreaming;
		int lineleniencelow,lineleniencehigh;
		int idletimeout;

		time_t last_penalty;
		time_t penalty_time;
		int penalty_count;

		string get_bindaddr(const string &force_bindaddr) const {
			return force_bindaddr.empty() ? bindaddr : force_bindaddr;
		}
		c_server(ulong id, const CfgSection *ds);
};
typedef multimap<ulong,c_server::ptr> t_server_list;
typedef pair<t_server_list::const_iterator,t_server_list::const_iterator> t_server_list_range;


class c_server_priority {
	public:
		c_server::ptr server;
		float baseprio;
		c_server_priority(c_server::ptr serv,float basep):server(serv),baseprio(basep){}
};
typedef map<c_server::ptr,c_server_priority*> t_server_priority_grouping;

class c_server_priority_grouping {
	public:
		string alias;
		float defglevel;
		float deflevel;
//		vector<c_server *> servers;
		t_server_priority_grouping priorities;
		float getserverpriority(const c_server::ptr &server) const {
			t_server_priority_grouping::const_iterator spgi=priorities.find(server);
			if (spgi!=priorities.end())
				return (*spgi).second->baseprio;
			else
				return deflevel;
		}
		c_server_priority_grouping(string alia):alias(alia){defglevel=1.0;deflevel=1.0;}
		~c_server_priority_grouping(){
			t_server_priority_grouping::iterator i;
			for (i=priorities.begin(); i!=priorities.end(); ++i)
				delete (*i).second;
		}
};
typedef map<const char *,c_server_priority_grouping*, ltstr> t_server_priority_grouping_list;

class c_group_info {//: public c_refcounted<c_group_info>{
	public:
		typedef c_group_info* ptr; //since we include c_group_info::ptrs in every server_article now, the refcounting overhead is actually noticable.  And since we never free them until program exit anyway, letting them "leak" doesn't matter.
		string alias;
		string group;
//		string priogrouping;
		c_server_priority_grouping *priogrouping;
		int usegz;
		c_group_info(string alia, string name, c_server_priority_grouping *prio, int gz):alias(alia),group(name),priogrouping(prio),usegz(gz){}
};
typedef map<const char *,c_group_info::ptr, ltstr> t_group_info_list;

typedef map<string, string> t_metagroup_list;

class c_nget_config {
	private:
			struct serv_match_by_name {
				const char *mname;
				bool operator() (pair<ulong,c_server::ptr> server) const{
					return (server.second->alias==mname || server.second->addr==mname);
				}
			};
		void dogetgroups(vector<c_group_info::ptr> &groups, const char *names);
	public:
		t_server_list serv;
		t_server_priority_grouping_list prioritygroupings;
		c_server_priority_grouping *trustsizes;
		t_group_info_list groups;
		t_metagroup_list metagroups;
		float curservmult;
		int usegz;
		int unequal_line_error;
		int fullxover;
		int maxstreaming;
		int idletimeout;
		int maxconnections;
		int penaltystrikes, initialpenalty;
		float penaltymultiplier;
		bool fatal_user_errors;
		bool autopar_optimistic;
		string bindaddr;

		void check_penalized(const c_server::ptr &s) const {
			if (penaltystrikes > 0 && s && s->penalty_count >= penaltystrikes) {
				long timeleft = s->last_penalty + s->penalty_time - time(NULL);
				if (timeleft > 0)
					throw TransportExFatal(Ex_INIT,"server %s penalized %li sec", s->alias.c_str(), timeleft);
			}
		}
		void unpenalize(c_server::ptr server) const {
			server->penalty_count = 0;
		}
		bool penalize(c_server::ptr server) const;
		int hasserver(ulong serverid) const {
			return serv.count(serverid);
		}
		t_server_list_range getservers(ulong serverid) const {
			return serv.equal_range(serverid);
		}
		c_server::ptr getserver(const string &name) const;
		c_server_priority_grouping* getpriogrouping(string groupname) const {
			t_server_priority_grouping_list::const_iterator spgli = prioritygroupings.find(groupname.c_str());
			if (spgli != prioritygroupings.end())
				return (*spgli).second;
			return NULL;
		}
		void addgroup_or_metagroup(const string &alias, const string &name);
		c_group_info::ptr addgroup(const string &alias, const string &name, string prio, int usegz=-2);
		void addmetagroup(const string &alias, const string &name);
		c_group_info::ptr getgroup(const char * groupname){
			t_group_info_list::iterator gili=groups.find(groupname);
			if (gili!=groups.end())
				return (*gili).second;
			return addgroup("",groupname,"");//do we even need to addgroup?  since groups are refcounted, could just return a new one.. nothing really needs it to be in the list..
		}
		void getgroups(vector<c_group_info::ptr> &groups, const char *names);
		void dogetallcachedgroups(vector<c_group_info::ptr> &groups);
		void setlist(const CfgSection *cfg,const CfgSection *hinfo,const CfgSection *pinfo,const CfgSection *ginfo);
		c_nget_config(){
			curservmult=2.0;
			usegz=-1;
			unequal_line_error=0;
			fullxover=0;
			maxstreaming=64;
			idletimeout=5*60;
			maxconnections=-1;
			penaltystrikes=3;
			initialpenalty=180;
			penaltymultiplier=2.0;
			fatal_user_errors=false;
			autopar_optimistic=false;
		}
		~c_nget_config(){
			delete trustsizes;
		}
};
class c_group_info_list {
	public:
};

extern c_nget_config nconfig;

#endif
