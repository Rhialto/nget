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
#include "grouplist.h"
#include <memory>
#include <errno.h>
#include "nget.h"
#include "file.h"
#include "strtoker.h"

//####ugly, these should go in some header but there isn't an obvious choice.
void setfilenamegz(string &file, int gz=-2);
c_file *dofileopen(string file, string mode, int gz=-2);


c_nntp_grouplist_server_info::ptr c_nntp_grouplist::getserverinfo(ulong serverid){
	c_nntp_grouplist_server_info::ptr servinfo=server_info[serverid];
	if (!servinfo){
		servinfo=new c_nntp_grouplist_server_info(serverid);
		server_info[serverid]=servinfo;
	}
	return servinfo;
}
		

bool c_group_availability::addserverdescription(ulong serverid, const string &desc){
	t_server_group_description_map::iterator di=servergroups.find(desc.c_str());
	t_server_group_description_map::iterator si;
	for (si=servergroups.begin();si!=servergroups.end(); ++si)
		if (si->second->serverids.find(serverid)!=si->second->serverids.end())
			break;

	if (si!=servergroups.end()) {
		if (si==di) return false;
		si->second->serverids.erase(serverid);
		if (si->second->serverids.empty())
			servergroups.erase(si);
	}
	if (di!=servergroups.end()) {
		di->second->serverids.insert(serverid);
	}else {
		c_server_group_description::ptr sgd = new c_server_group_description(desc);
		sgd->serverids.insert(serverid);
		servergroups.insert(t_server_group_description_map::value_type(sgd->description.c_str(), sgd));
	}
	return true;
}


void c_nntp_grouplist::addgroup(ulong serverid, string name){
	c_group_availability::ptr group;
	t_group_availability_map::iterator i=groups.find(name.c_str());
	if (i!=groups.end()){
		group = i->second;
	}else{
		group = new c_group_availability(name);
		groups.insert(t_group_availability_map::value_type(group->groupname.c_str(), group));
	}
	if (group->serverids.find(serverid)==group->serverids.end()) {
		group->serverids.insert(serverid);
		saveit=true;
	}
}

void c_nntp_grouplist::addgroupdesc(ulong serverid, string name, string desc){
	c_group_availability::ptr group;
	t_group_availability_map::iterator i=groups.find(name.c_str());
	if (i!=groups.end()){
		group = i->second;
	}else{
		group = new c_group_availability(name);
		groups.insert(t_group_availability_map::value_type(group->groupname.c_str(), group));
	}
	if (group->addserverdescription(serverid, desc))
		saveit=true;
}

void c_nntp_grouplist::flushserver(ulong serverid) {
	ulong countg=0, countgs=0, countd=0, countds=0;
	int servinfoerased=server_info.erase(serverid);
	t_group_availability_map::iterator gi, gitmp;
	c_group_availability::ptr group;
	if (quiet<2) {printf("Flushing grouplist for server %lu:", serverid);fflush(stdout);}
	for (gi = groups.begin(); gi != groups.end();) {
		gitmp = gi;
		++gi;
		group = gitmp->second;
		for (t_server_group_description_map::iterator gdi=group->servergroups.begin(); gdi!=group->servergroups.end();) {
			t_server_group_description_map::iterator gditmp=gdi;
			++gdi;
			c_server_group_description::ptr gd = gditmp->second;
			countds += gd->serverids.erase(serverid);
			if (gd->serverids.empty()) {
				group->servergroups.erase(gditmp);
				++countd;
			}
		}
		countgs += group->serverids.erase(serverid);
		if (group->serverids.empty() && group->servergroups.empty()) {
			groups.erase(gitmp);
			++countg;
		}
	}
	if (quiet<2){printf(" %lu groups (%lu dead), %lu descriptions (%lu dead)\n",countgs,countg,countds,countd);}
	if (servinfoerased || countgs || countds) saveit=1;
}

static void nntp_grouplist_printinfo(const t_grouplist_getinfo_list &getinfos, const c_group_availability::ptr &g) {
	t_grouplist_getinfo_list::const_iterator gii, giibegin=getinfos.begin(), giiend=getinfos.end();
	c_grouplist_getinfo::ptr info;
	for (gii=giibegin; gii!=giiend; ++gii) {
		info = *gii;
		if ((*info->pred)(g.gimmethepointer())) {
			for (set<ulong>::const_iterator sii=g->serverids.begin(); sii!=g->serverids.end(); ++sii) {
				printf("%s",nconfig.getserver(*sii)->shortname.c_str());
			}
			printf("\t%s",g->groupname.c_str());
			for (t_server_group_description_map::const_iterator sgdi=g->servergroups.begin(); sgdi!=g->servergroups.end(); ++sgdi) {
				printf("\t%s [",sgdi->first);
				for (set<ulong>::const_iterator sii=sgdi->second->serverids.begin(); sii!=sgdi->second->serverids.end(); ++sii) {
					printf("%s",nconfig.getserver(*sii)->shortname.c_str());
				}
				printf("]");
			}
			printf("\n");
		}
	}
}

void c_nntp_grouplist::printinfos(const t_grouplist_getinfo_list &getinfos) {
	t_group_availability_map::const_iterator gi;
	for (gi = groups.begin(); gi != groups.end(); ++gi)
		nntp_grouplist_printinfo(getinfos, gi->second);
}


class c_nntp_grouplist_reader {
	protected:
		c_file *f;
	public:
		ulong curline, num, nums, numd, numsd, countdeads, countdeadg;
		c_nntp_grouplist_reader(c_file *cf, t_nntp_grouplist_server_info_map &server_infoi);
		c_group_availability::ptr read_group(void);
		void check_counts(void);
};

c_nntp_grouplist_reader::c_nntp_grouplist_reader(c_file *cf, t_nntp_grouplist_server_info_map &server_info):f(cf) {
	curline=0; num=0; nums=0; numd=0; numsd=0; countdeads=0; countdeadg=0;

	char *t[2];
	int i;

	if (f->bgets()<=0)
		throw CacheEx(Ex_INIT, "unexpected EOF on grouplist file line %lu",curline);
	curline++;
	if (strcmp(f->rbufp(),GLCACHE_VERSION)!=0){
		if (strncmp(f->rbufp(),"NGETGL",6)==0)
			throw CacheEx(Ex_INIT,"grouplist is from a different version of nget");
		else
			throw CacheEx(Ex_INIT,"does not seem to be an nget grouplist file");
	}
	
	while (1){
		if (f->beof())
			throw CacheEx(Ex_INIT, "unexpected EOF on grouplist file line %lu",curline);
		curline++;
		if (f->bpeek()=='.') {
			if (f->bgetsp()[1]!=0) {
				printf("grouplist: stuff after . line %lu mode SERVERINFO\n",curline);
				set_cache_warn_status();
			}
			//mode=FILE_MODE;//start new file mode
			return;
		}
		i = f->btoks('\t',t,2);
		if (i==2) {
			ulong serverid=atoul(t[0]);
			if (nconfig.getserver(serverid)) {
				server_info.insert(t_nntp_grouplist_server_info_map::value_type(serverid, new c_nntp_grouplist_server_info(serverid, t[1])));
			}else{
				printf("warning: serverid %lu not found in server list\n",serverid);
				set_cache_warn_status();
			}
		}else {
			printf("grouplist: invalid line %lu (%i toks)\n",curline,i);
			set_cache_warn_status();
		}
	}
}

c_group_availability::ptr c_nntp_grouplist_reader::read_group(void) {
	char *t[2];
	int i;
	char *buf;
	c_group_availability::ptr gd;
	while (!f->beof()){
		curline++;
		i = f->btoks('\t',t,2);
		if (i==2){
			char * cp;
			buf = t[0];
			gd = new c_group_availability(t[1]);
			while ((cp = goodstrtok(&buf, ','))) {
				ulong serverid=atoul(cp);
				if (nconfig.getserver(serverid)) {
					gd->serverids.insert(serverid);
					nums++;
				}else{
					countdeads++;
				}
			}
			num++;
			while (!f->beof()){
				curline++;
				if (f->bpeek()=='.') {
					if (f->bgetsp()[1]!=0) {
						printf("grouplist: stuff after . line %lu",curline);
						set_cache_warn_status();
					}
					break;
				}
				i = f->btoks('\t',t,2);
				if (i==2){
					char * cp;
					buf = t[0];
					while ((cp = goodstrtok(&buf, ','))) {
						ulong serverid=atoul(cp);
						if (nconfig.getserver(serverid)) {
							gd->addserverdescription(serverid, t[1]);
							numsd++;
						}else{
							countdeads++;
						}
					}
				}else {
					printf("grouplist: invalid line %lu (%i toks)\n",curline,i);
					set_cache_warn_status();
				}
			}
			if (!gd->servergroups.empty())
				numd+=gd->servergroups.size();
			if (!gd->servergroups.empty() || !gd->serverids.empty()){
				return gd;
			} else
				++countdeadg;
		}else {
			printf("grouplist: invalid line %lu (%i toks)\n",curline,i);
			set_cache_warn_status();
		}
	}
	if (gd)
		throw CacheEx(Ex_INIT, "unexpected EOF on grouplist file line %lu",curline);
	return NULL;
}

void c_nntp_grouplist_reader::check_counts(void) {
	if (countdeads||countdeadg){
		printf("c_nntp_grouplist: read (and ignored) %lu bad serverids (%lu dead groups)\n",countdeads,countdeadg);
		set_cache_warn_status();
	}
}


c_nntp_grouplist::c_nntp_grouplist(void){
	c_file *f=NULL;
	saveit=0;
	//file=nid;
	filename=nghome+"newsgroups";
	setfilenamegz(filename);
	try {
		f=dofileopen(filename.c_str(),"rb");
	}catch(FileNOENTEx &e){
		return;
	}
	auto_ptr<c_file> fcloser(f);
	try{
		c_nntp_grouplist_reader reader(f, server_info);
		c_group_availability::ptr ng;
		while ((ng=reader.read_group()))
			groups.insert(t_group_availability_map::value_type(ng->groupname.c_str(), ng));
		PDEBUG(DEBUG_MIN,"read %lu groups, %lu descriptions (%lu sgs, %lu sgds)",(ulong)groups.size(),reader.numd,reader.nums,reader.numsd);
		reader.check_counts();
	} catch (CacheEx &e) {
		set_cache_warn_status();
		printf("%s: %s\n", filename.c_str(), e.getExStr());
	}
	f->close();
}

void c_nntp_grouplist::save(void){
	if (!saveit)
		return;
	if (filename.empty())
		return;
	int nums=0, numsd=0, num=0, numd=0;
	string tmpfn=filename+".tmp";
	c_file *f=f=dofileopen(tmpfn,"wb");
	try {
		auto_ptr<c_file> fcloser(f);
		if (debug){printf("saving grouplist: %lu groups..",(ulong)groups.size());fflush(stdout);}
		t_group_availability_map::iterator gdi;
		t_server_group_description_map::iterator sgdi;
		c_group_availability::ptr gd;
		f->putf(GLCACHE_VERSION"\n");
		for (t_nntp_grouplist_server_info_map::iterator glsi=server_info.begin(); glsi!=server_info.end(); ++glsi) {
			f->putf("%lu\t%s\n", glsi->first, glsi->second->date.c_str());
		}
		f->putf(".\n");
		for (gdi=groups.begin(); gdi!=groups.end(); ++gdi){
			gd=gdi->second;
			char *sep="";
			for (set<ulong>::iterator sii=gd->serverids.begin(); sii!=gd->serverids.end(); ++sii) {
				f->putf("%s%lu", sep, *sii);
				sep=",";
				nums++;
			}
			f->putf("\t%s\n",gd->groupname.c_str());
			for (sgdi=gd->servergroups.begin(); sgdi!=gd->servergroups.end(); ++sgdi) {
				c_server_group_description::ptr sgd = sgdi->second;
				char *sep="";
				for (set<ulong>::iterator sii=sgd->serverids.begin(); sii!=sgd->serverids.end(); ++sii) {
					f->putf("%s%lu", sep, *sii);
					sep=",";
					numsd++;
				}
				f->putf("\t%s\n",sgd->description.c_str());
				numd++;
			}
			f->putf(".\n");
			num++;
		}
		if (debug) printf(" (%i g, %i sg, %i gd, %i sgd) done.\n",num,nums,numd,numsd);
		f->close();
	}catch(FileEx &e){
		if (unlink(tmpfn.c_str()))
			perror("unlink:");
		throw;
	}
	xxrename(tmpfn.c_str(), filename.c_str());
}
c_nntp_grouplist::~c_nntp_grouplist(){
	try {
		save();
	} catch (FileEx &e) {
		printCaughtEx(e);
		fatal_exit();
	}
}

void nntp_grouplist_printinfos(const t_grouplist_getinfo_list &getinfos) {
	c_file *f=NULL;
	string filename=nghome+"newsgroups";
	setfilenamegz(filename);
	try {
		f=dofileopen(filename.c_str(),"rb");
	}catch(FileNOENTEx &e){
		return;
	}
	auto_ptr<c_file> fcloser(f);
	try{
		t_nntp_grouplist_server_info_map server_info;
		c_nntp_grouplist_reader reader(f, server_info);
		c_group_availability::ptr ng;
		ulong numgroups=0;
		while ((ng=reader.read_group())) {
			numgroups++;
			nntp_grouplist_printinfo(getinfos, ng);
		}
		PDEBUG(DEBUG_MIN,"scanned %lu groups, %lu descriptions (%lu sgs, %lu sgds)",numgroups,reader.numd,reader.nums,reader.numsd);
		reader.check_counts();
	} catch (CacheEx &e) {
		set_cache_warn_status();
		printf("%s: %s\n", filename.c_str(), e.getExStr());
	}
	f->close();
}
