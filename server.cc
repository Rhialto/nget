/*
    server.* - nget configuration handling
    Copyright (C) 2000-2001  Matthew Mueller <donut@azstarnet.com>

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


void c_nget_config::setlist(c_data_section *cfg,c_data_section *hinfo,c_data_section *pinfo,c_data_section *ginfo){
	c_server *server;
	data_list::iterator dli,dlj;
	c_data_section *ds;
	c_data_item *di;
	const char *sida;
	ulong tul;
	//cfg
	assert(cfg);
	cfg->getitemi("usegz",&usegz);
	cfg->getitemi("unequal_line_error",&unequal_line_error);
	//halias
	assert(hinfo);
	for (dli=hinfo->data.begin();dli!=hinfo->data.end();++dli){
		di=(*dli).second;
		if (di->type!=1){
			printf("h not a section\n");
			continue;
		}
		//				ds=dynamic_cast<c_data_section*>(di);
		ds=(c_data_section*)(di);//TODO: ok, this is bad, but dynamic_cast doesn't work. ??
		if (!ds){
			printf("h !ds\n");continue;
		}
		if (!ds->getitema("addr")){
			printf("host %s no addr\n",ds->key.c_str());
			continue;
		}
		if (!(sida=ds->getitema("id"))){
			printf("host %s no id\n",ds->key.c_str());
			continue;
		}
		tul=atoul(sida);
		if (tul==0){
			printf("host %s invalid id '%s'\n",ds->key.c_str(),sida);
			continue;
		}
		server=new c_server(tul,ds->key,ds->getitems("addr"),ds->getitems("user"),ds->getitems("pass"),ds->getitema("fullxover"),ds->getitema("linelenience"),ds->geti("maxstreaming",64));
		serv[server->serverid]=server;
	}
	//hpriority
	if (pinfo)
		for (dli=pinfo->data.begin();dli!=pinfo->data.end();++dli){
			di=(*dli).second;
			if (di->type!=1){
				printf("p not a section\n");
				continue;
			}
			//				ds=dynamic_cast<c_data_section*>(di);
			ds=(c_data_section*)(di);//TODO: ok, this is bad, but dynamic_cast doesn't work. ??
			if (!ds){
				printf("p !ds\n");continue;
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
						printf("prio section %s, server %s not found\n",ds->key.c_str(),di->key.c_str());
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
				addgroup(di->key,di->str,"");
			}else{
				//				ds=dynamic_cast<c_data_section*>(di);
				ds=(c_data_section*)(di);//TODO: ok, this is bad, but dynamic_cast doesn't work. ??
				if (!ds){
					printf("g !ds\n");continue;
				}
				addgroup(ds->key,ds->getitems("group"),ds->getitems("prio"),ds->geti("usegz",-2));
			}
		}
}
