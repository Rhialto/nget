/*
    server.* - nget configuration handling
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
#include "server.h"
#include "strreps.h"
#include "nget.h"
#include <sys/types.h>
#include <dirent.h>

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

c_server::c_server(ulong id, string alia, string shortnam, string add, string use,string pas,const char *fullxove,const char *ll,int maxstrea, int idletimeou):alias(alia),shortname(shortnam),addr(add),user(use),pass(pas),idletimeout(idletimeou){
	serverid=id;
	if (shortname.empty())
		shortname=alias[0];
	if (fullxove)
		fullxover=atoi(fullxove);
	else
		fullxover=nconfig.fullxover;
	if (maxstrea<0) {
		PERROR("invalid maxstreaming %i for host %s",maxstrea,addr.c_str());
		maxstreaming = 0;
	}else
		maxstreaming = maxstrea;
	if (ll){
		int l,h;
		if (!parse_int_pair(ll,&l,&h)){
			lineleniencelow=l;lineleniencehigh=h;
		}else{
			PERROR("invalid linelenience %s for host %s",ll,addr.c_str());
			lineleniencelow=lineleniencehigh=0;
		}
	}else{
		lineleniencelow=lineleniencehigh=0;
	}

	penalty_count=0;
	last_penalty=0;
	penalty_time=0;
}

void c_nget_config::setlist(c_data_section *cfg,c_data_section *hinfo,c_data_section *pinfo,c_data_section *ginfo){
	c_server::ptr server;
	data_list::iterator dli,dlj;
	c_data_section *ds;
	c_data_item *di;
	const char *sida;
	ulong tul;
	//cfg
	assert(cfg);
	const char *cp=cfg->getitema("curservmult");
	float f;
	if (cp){
		f=atof(cp);
		if (f)
			nconfig.curservmult=f;
		else
			PERROR("invalid curservmult: %s",cp);
	}
	cfg->getitemi("usegz",&usegz);
	fullxover=cfg->geti("fullxover", fullxover);
	fatal_user_errors=cfg->geti("fatal_user_errors", fatal_user_errors);
	cfg->getitemi("unequal_line_error",&unequal_line_error);
	cfg->getitemi("maxstreaming",&maxstreaming);
	cfg->getitemi("maxconnections",&maxconnections);
	cfg->getitemi("idletimeout",&idletimeout);
	cfg->getitemi("penaltystrikes",&penaltystrikes);
	cfg->getitemi("initialpenalty",&initialpenalty);
	cp=cfg->getitema("penaltymultiplier");
	if (cp){
		f=atof(cp);
		if (f>0)
			nconfig.penaltymultiplier=f;
		else
			PERROR("invalid penaltymultiplier: %s",cp);
	}
	//halias
	assert(hinfo);
	for (dli=hinfo->data.begin();dli!=hinfo->data.end();++dli){
		di=(*dli).second;
		if (di->type!=1){
			PERROR("h not a section");
			continue;
		}
		ds=static_cast<c_data_section*>(di);
		if (!ds){
			PERROR("h !ds");continue;
		}
		if (!ds->getitema("addr")){
			PERROR("host %s no addr",ds->key.c_str());
			continue;
		}
		if (!(sida=ds->getitema("id"))){
			PERROR("host %s no id",ds->key.c_str());
			continue;
		}
		tul=atoul(sida);
		if (tul==0){
			PERROR("host %s invalid id '%s'",ds->key.c_str(),sida);
			continue;
		}
		server=new c_server(tul,ds->key,ds->getitems("shortname"),ds->getitems("addr"),ds->getitems("user"),ds->getitems("pass"),ds->getitema("fullxover"),ds->getitema("linelenience"),ds->geti("maxstreaming",maxstreaming),ds->geti("idletimeout",idletimeout));
		serv[server->serverid]=server;
	}
	//hpriority
	if (pinfo)
		for (dli=pinfo->data.begin();dli!=pinfo->data.end();++dli){
			di=(*dli).second;
			if (di->type!=1){
				PERROR("p not a section");
				continue;
			}
			ds=static_cast<c_data_section*>(di);
			if (!ds){
				PERROR("p !ds");continue;
			}
			c_server_priority_grouping *pgrouping=new c_server_priority_grouping(ds->key);
			for (dlj=ds->data.begin();dlj!=ds->data.end();++dlj){
				di=(*dlj).second;
				if (di->key=="_level"){
					pgrouping->deflevel=atof(di->str.c_str());
				}else if (di->key=="_glevel"){
					pgrouping->defglevel=atof(di->str.c_str());
				}else{
					server=getserver(di->key);
					if (!server){
						PERROR("prio section %s, server %s not found",ds->key.c_str(),di->key.c_str());
						continue;
					}
					c_server_priority *sprio=new c_server_priority(server,atof(di->str.c_str()));
					pgrouping->priorities.insert(t_server_priority_grouping::value_type(sprio->server->serverid,sprio));
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
	if (ginfo)
		for (dli=ginfo->data.begin();dli!=ginfo->data.end();++dli){
			di=(*dli).second;
			if (di->type!=1){
				addgroup_or_metagroup(di->key,di->str);
			}else{
				ds=static_cast<c_data_section*>(di);
				if (!ds){
					PERROR("g !ds");continue;
				}
				addgroup(ds->key,ds->getitems("group"),ds->getitems("prio"),ds->geti("usegz",-2));
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
	if (name.empty())return NULL;
	if (prio.empty())prio="default";
	c_server_priority_grouping *priog=getpriogrouping(prio);
	if (!priog){
		printf("group %s(%s), prio %s not found\n",name.c_str(),alias.c_str(),prio.c_str());
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
	DIR *dir=opendir(nghome.c_str());
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
