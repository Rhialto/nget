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
#include <climits>
#include "server.h"
#include "strreps.h"
#include "nget.h"
#include "status.h"
#include <sys/types.h>
#include <dirent.h>
#include <algorithm>

bool c_nget_config::penalize(c_server::ptr server) const {
	if (penaltystrikes<=0)
		return false;//penalization disabled
	++server->penalty_count;
	if (server->penalty_count == penaltystrikes) {
		server->penalty_time = initialpenalty;
	}
	else if (server->penalty_count > penaltystrikes) {
		server->penalty_time = (time_t)(server->penalty_time * penaltymultiplier);
	}
	server->last_penalty = time(NULL);
	PDEBUG(DEBUG_MED, "penalized %s: count %i, last %li, time %li", server->alias.c_str(), server->penalty_count, server->last_penalty, server->penalty_time);
	return server->penalty_count >= penaltystrikes;
}

c_server::ptr c_nget_config::getserver(const string &name) const {
	serv_match_by_name name_matcher;
	name_matcher.mname=name.c_str();
	t_server_list::const_iterator sli=find_if(serv.begin(),serv.end(),name_matcher);
	if (sli!=serv.end())
		return (*sli).second;
	return NULL;
}

int parse_int_pair(const char *s, int *l, int *h){
	const char *p;
	char *erp;
	int i;
	if (!s || *s=='\0')return -1;
	p=strchr(s,',');
	if (p){
		int i2;
		p++;
		if (*p=='\0')return -1;
		i=strtol(s,&erp,0);
		if (*erp!=',')
			return -1;
		i2=strtol(p,&erp,0);
		if (*erp!='\0')
			return -1;
		if (i<=i2){
			*l=i;*h=i2;
		}else{
			*l=i2;*h=i;
		}
	}else{
		i=strtol(s,&erp,0);
		if (*erp!='\0')
			return -1;
		if (i<0)i=-i;
		*l=-i;*h=i;
	}
	return 0;
}

c_server::c_server(ulong id, const CfgSection *ds) : serverid(id), alias(ds->key) {
	addr = ds->gets("addr");
	user = ds->gets("user");
	pass = ds->gets("pass");
	bindaddr = ds->gets("bindaddr", nconfig.bindaddr);
	shortname = ds->gets("shortname");
	if (shortname.empty()) shortname = alias[0];
	ds->get("idletimeout",idletimeout,1,INT_MAX,nconfig.idletimeout);
	ds->get("fullxover",fullxover,0,2,nconfig.fullxover);
	ds->get("maxstreaming",maxstreaming,0,INT_MAX,nconfig.maxstreaming);
	if (ds->getitem("linelenience")){
		const char *ll = ds->geta("linelenience");
		int l,h;
		if (!parse_int_pair(ll,&l,&h)){
			lineleniencelow=l;lineleniencehigh=h;
		}else{
			PERROR("%s: invalid linelenience %s",ds->name().c_str(),ll);
			set_user_error_status();
			lineleniencelow=lineleniencehigh=0;
		}
	}else{
		lineleniencelow=lineleniencehigh=0;
	}

	penalty_count=0;
	last_penalty=0;
	penalty_time=0;
}

void c_nget_config::setlist(const CfgSection *cfg,const CfgSection *hinfo,const CfgSection *pinfo,const CfgSection *ginfo){
	c_server::ptr server;
	CfgSection_map::const_iterator dli;
	CfgItem_map::const_iterator dii; 
	const CfgSection *ds;
	const CfgItem *di;
	ulong tul;
	//cfg
	assert(cfg);
	cfg->get("curservmult",curservmult);
	cfg->get("usegz",usegz,-1,9);
	cfg->get("fullxover", fullxover, 0, 2);
	cfg->get("fatal_user_errors", fatal_user_errors, false, true);
	cfg->get("autopar_optimistic", autopar_optimistic, false, true);
	cfg->get("unequal_line_error",unequal_line_error,0,1);
	cfg->get("maxstreaming",maxstreaming,0,INT_MAX);
	cfg->get("maxconnections",maxconnections,-1,INT_MAX);
	cfg->get("idletimeout",idletimeout,1,INT_MAX);
	cfg->get("penaltystrikes",penaltystrikes,-1,INT_MAX);
	cfg->get("initialpenalty",initialpenalty,1,INT_MAX);
	cfg->get("penaltymultiplier",penaltymultiplier,1.0f,FLT_MAX);
	bindaddr = cfg->gets("bindaddr");
	cfg->get("maxheaders",maxheaders,-1,INT_MAX);
	//halias
	assert(hinfo);
	for (dli=hinfo->sections_begin();dli!=hinfo->sections_end();++dli){
		ds=(*dli).second;
		assert(ds);
		if (!ds->getitem("addr")){
			PERROR("host %s no addr",ds->key.c_str());
			set_user_error_status();
			continue;
		}
		if (!ds->getitem("id")){
			PERROR("host %s no id",ds->key.c_str());
			set_user_error_status();
			continue;
		}
		if (!ds->get("id",tul,1UL,ULONG_MAX))
			continue;
		server = new c_server(tul, ds);
		serv.insert(t_server_list::value_type(server->serverid,server));
	}
	//hpriority
	if (pinfo)
		for (dli=pinfo->sections_begin();dli!=pinfo->sections_end();++dli){
			ds=(*dli).second;
			assert(ds);
			c_server_priority_grouping *pgrouping=new c_server_priority_grouping(ds->key);
			for (dii=ds->items_begin();dii!=ds->items_end();++dii){
				di=(*dii).second;
				if (di->key=="_level"){
					di->get(pgrouping->deflevel);
				}else if (di->key=="_glevel"){
					di->get(pgrouping->defglevel);
				}else{
					server=getserver(di->key);
					if (!server){
						PERROR("prio section %s, server %s not found",ds->key.c_str(),di->key.c_str());
						set_user_error_status();
						continue;
					}
					c_server_priority *sprio=new c_server_priority(server,atof(di->gets().c_str()));
					pgrouping->priorities.insert(t_server_priority_grouping::value_type(sprio->server,sprio));
				}
			}
			if (pgrouping->alias=="trustsizes")
				trustsizes=pgrouping;
			else
				prioritygroupings.insert(t_server_priority_grouping_list::value_type(pgrouping->alias.c_str(),pgrouping));
		}
	if (getpriogrouping("default")==NULL){
		c_server_priority_grouping *pgrouping=new c_server_priority_grouping("default");
		prioritygroupings.insert(t_server_priority_grouping_list::value_type(pgrouping->alias.c_str(),pgrouping));
	}
	if (trustsizes==NULL){
		trustsizes=new c_server_priority_grouping("trustsizes");
	}
	//galias
	if (ginfo) {
		for (dii=ginfo->items_begin();dii!=ginfo->items_end();++dii){
			di=(*dii).second;
			addgroup_or_metagroup(di->key,di->gets());
		}
		for (dli=ginfo->sections_begin();dli!=ginfo->sections_end();++dli){
			ds=(*dli).second;
			assert(ds);
			int susegz;
			ds->get("usegz",susegz,-1,9,-2);
			addgroup(ds->key,ds->gets("group"),ds->gets("prio"),susegz);
		}
	}
}

void c_nget_config::addgroup_or_metagroup(const string &alias, const string &name){
	if (name.find(',')!=string::npos)
		addmetagroup(alias,name);
	else
		addgroup(alias,name,"");
}

void c_nget_config::addmetagroup(const string &alias, const string &name){
	metagroups.insert(t_metagroup_list::value_type(alias,name));
}

c_group_info::ptr c_nget_config::addgroup(const string &alias, const string &name, string prio, int usegz){
	if (prio.empty())prio="default";
	c_server_priority_grouping *priog=getpriogrouping(prio);
	if (!priog){
		printf("group %s(%s), prio %s not found\n",name.c_str(),alias.c_str(),prio.c_str());
		set_user_error_status();
		return NULL;
	}
	assert(priog);
	c_group_info::ptr group(new c_group_info(alias,name,priog,usegz));
	if (group){
		if (!alias.empty())
			groups.insert(t_group_info_list::value_type(group->alias.c_str(),group));
		groups.insert(t_group_info_list::value_type(group->group.c_str(),group));
	}
	return group;
}

void c_nget_config::dogetallcachedgroups(vector<c_group_info::ptr> &groups) {
	DIR *dir=opendir(ngcachehome.c_str());
	struct dirent *de;
	if (!dir)
		throw PathExFatal(Ex_INIT,"opendir: %s(%i)",strerror(errno),errno);
	while ((de=readdir(dir))) {
		char *endp;
		if ((endp=strstr(de->d_name,",cache"))){
			string groupname(de->d_name, endp - de->d_name);
			groups.push_back(getgroup(groupname.c_str()));
		}
	}
	closedir(dir);
}

void c_nget_config::dogetgroups(vector<c_group_info::ptr> &groups, const char *names) {
	char *foo = new char[strlen(names)+1];
	char *cur = foo, *name = NULL;
	strcpy(foo, names);
	while ((name = goodstrtok(&cur, ','))) {
		if (strcmp(name,"*")==0)
			dogetallcachedgroups(groups);
		else {
			t_metagroup_list::const_iterator mgi=metagroups.find(name);
			if (mgi!=metagroups.end())
				dogetgroups(groups, mgi->second.c_str());
			else
				groups.push_back(getgroup(name));
		}
	}
	delete [] foo;
}

void c_nget_config::getgroups(vector<c_group_info::ptr> &groups, const char *names) {
	groups.clear();
	vector<c_group_info::ptr> tmpgroups;
	dogetgroups(tmpgroups, names);
	for (vector<c_group_info::ptr>::const_iterator gi=tmpgroups.begin(); gi!=tmpgroups.end(); ++gi)
		if (find(groups.begin(), groups.end(), *gi)==groups.end())
			groups.push_back(*gi); // return only unique groups
}
