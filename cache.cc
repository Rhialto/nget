/*
    cache.* - nntp header cache code
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
#include "cache.h"
#include "strreps.h"
#include "log.h"
#include <set>
#include <memory>
#include <errno.h>
#include <unistd.h>
#include "auto_vector.h"
#include "nget.h"
#include "mylockfile.h"
#include "strtoker.h"
#include "path.h"
#include "par.h"

static inline bool count_partnum(int partnum, int req) {
	if (req>0)
		return (partnum>0 && partnum<=req);
	else
		return (partnum == req);
}

int c_nntp_header::parsepnum(const char *str,const char *soff){
	const char *p;
	assert(str);
	assert(soff>=str);
	if ((p=strpbrk(soff+1,")]"))){
		char m,m2=*p;
		if (m2==')') m='(';
		else m='[';
		tailoff=p-str;
		for(p=soff;p>str;p--)
			if (*p==m){
				p++;
				char *erp;
				partoff=p-str;
				partnum=strtol(p,&erp,10);
				if (*erp!='/' || erp==p) return -1;
				int req=strtol(soff+1,&erp,10);
				if (*erp!=m2 || erp==soff+1) return -1;
				if (partnum>req) return -1;
				if (partnum==0)
					req=0;//handle 0-files seperatly from the binary they accompany
				return req;
			}
	}
	return -1;
}

void c_nntp_header::setfileid(char *refstr, unsigned int refstrlen){
#ifdef CHECKSUM
	fileid=CHECKSUM(0L, Z_NULL, 0);
	fileid=CHECKSUM(fileid,(Byte*)subject.c_str(),subject.size());
	fileid=CHECKSUM(fileid,(Byte*)author.c_str(),author.size());
	if (refstrlen)
		fileid=CHECKSUM(fileid,(Byte*)refstr,refstrlen);
#else
	hash<char *> H;
	fileid=H(subject.c_str())+H(author.c_str());//prolly not as good as crc32, but oh well.
	char *p = refstr;
	while (p - refstr < refstrlen) {
		fileid+=H(p);
		p+=strlen(p) + 1;
	}
#endif
}
void c_nntp_header::set(char * str,const char *a,ulong anum,time_t d,ulong b,ulong l,const char *mid, char *refstr){
	assert(str);
	assert(a);
	author=a;articlenum=anum;date=d;bytes=b;lines=l;
	messageid=mid;
	int refstrlen=refstr?strlen(refstr):0;

	references.clear();
	if (refstr && *refstr) {
		char *ref, *refstr_copy=refstr;
		while ((ref = goodstrtok(&refstr_copy,' '))) {
			references.push_back(ref);
		}
	}

	const char *s=str+strlen(str)-3;//-1 for null, -2 for ), -3 for num
	req=0;
	for (;s>str;s--) {
		if (*s=='/')
			if ((req=parsepnum(str,s))>=0){
				if (req==0) {
					subject=str;
				} else {
					subject="";
					subject.append(str,partoff);
					subject.append("*");
					subject.append(s);
				}
				setfileid(refstr, refstrlen);
				return;
			}
	}
	partoff=-1;tailoff=-1;
//	partnum=0;
	partnum=-1;
	subject=str;
	setfileid(refstr, refstrlen);
}

c_nntp_server_article::c_nntp_server_article(ulong _server,const c_group_info::ptr &_group,ulong _articlenum,ulong _bytes,ulong _lines):serverid(_server),group(_group),articlenum(_articlenum),bytes(_bytes),lines(_lines){}

//c_nntp_part::c_nntp_part(c_nntp_header *h):partnum(h->partnum),articlenum(h->articlenum),date(h->date),bytes(h->bytes),lines(h->lines){}
c_nntp_part::c_nntp_part(c_nntp_header *h):partnum(h->partnum),date(h->date),messageid(h->messageid){
	addserverarticle(h);
}

void c_nntp_part::addserverarticle(c_nntp_header *h){
	c_nntp_server_article *sa;
#ifndef NDEBUG
	if (debug>=DEBUG_MIN){
		t_nntp_server_articles::iterator sai=articles.find(h->serverid);
		if (sai!=articles.end()){
			sa=(*sai).second;
			printf("adding server_article we already have %lu %lu %lu %lu(%lu %lu %lu %lu)\n",h->serverid,h->articlenum,h->bytes,h->lines,sa->serverid,sa->articlenum,sa->bytes,sa->lines);
			//		return;//could be useful, lets add it.
		}
	}
	if (h->date!=date)
		printf("adding server_article with different date, date=%li h->date=%li mid=%s\n",date,h->date,h->messageid.c_str());
#endif
	sa=new c_nntp_server_article(h->serverid,h->group,h->articlenum,h->bytes,h->lines);
	articles.insert(t_nntp_server_articles::value_type(h->serverid,sa));
}

c_nntp_part::~c_nntp_part(){
	t_nntp_server_articles::iterator i;
	for(i = articles.begin();i!=articles.end();++i){
		assert((*i).second);
		delete (*i).second;
	}
}

void c_nntp_file::addpart(c_nntp_part *p){
	assert(p);
	//assert((req==-1 && p->partnum<=0) || (p->partnum<=req));//#### req==-1 hack for old version that set non-multipart messages partnum to 0 instead of -1
//	parts[p->partnum]=p;
#ifndef NDEBUG
	t_nntp_file_parts::iterator nfpi=parts.find(p->partnum);
	assert(nfpi==parts.end());
#endif
	parts.insert(t_nntp_file_parts::value_type(p->partnum,p));
	if (count_partnum(p->partnum, req)) have++;
//	bytes+=p->apxbytes;lines+=p->apxlines;
}

void c_nntp_file::mergefile(c_nntp_file::ptr &f){
	t_nntp_file_parts::iterator fpi=f->parts.begin();
	while (fpi!=f->parts.end()){
		const c_nntp_part *p = fpi->second;
		t_nntp_file_parts::iterator nfpi=parts.find(p->partnum);
		if (nfpi==parts.end()) {
			addpart(new c_nntp_part(p->partnum, p->date, p->messageid));
			nfpi=parts.find(p->partnum);
		}else{
			if (nfpi->second->messageid!=p->messageid){
				PDEBUG(DEBUG_MED,"%s was gonna merge, but already have this part(sub=%s part=%i omid=%s)?\n",p->messageid.c_str(),f->subject.c_str(),p->partnum,nfpi->second->messageid.c_str());
				++fpi;
				continue;
			}
		}
		for (t_nntp_server_articles::const_iterator fsai=p->articles.begin(); fsai!=p->articles.end(); ++fsai){
			
			c_nntp_server_article *nsa = new c_nntp_server_article(*fsai->second);
			nfpi->second->articles.insert(t_nntp_server_articles::value_type(nsa->serverid,nsa));
		}
		t_nntp_file_parts::iterator del_pi = fpi;
		++fpi;
		delete del_pi->second;
		f->parts.erase(del_pi);
	}
}

//fill a mapping of how many parts of the file each server has
void c_nntp_file::get_server_have_map(t_server_have_map &have_map) const{
	t_nntp_file_parts::const_iterator pi(parts.begin());
	for (;pi!=parts.end();++pi){
		t_nntp_server_articles::const_iterator nsai(pi->second->articles.begin());
		ulong serverid;
		int partnum=pi->second->partnum;
		set<ulong> servers_already_found;

		for (;nsai!=pi->second->articles.end();++nsai) {
			serverid=nsai->first;
			//don't increment count twice if a server has multiple server_articles for a single part
			if (servers_already_found.insert(serverid).second){
				t_server_have_map::iterator hmi(have_map.insert(t_server_have_map::value_type(serverid, 0)).first);
				if (count_partnum(partnum, req))
					++hmi->second;
			}
		}
	}
}

c_nntp_file::c_nntp_file(int r,ulong f,t_id fi,const char *s,const char *a,int po,int to):c_nntp_file_base(fi, r, po, a, s),have(0),flags(f),tailoff(to){
//	printf("aoeu1.1\n");
}
c_nntp_file::c_nntp_file(c_nntp_header *h):c_nntp_file_base(*h),have(0),flags(0),tailoff(h->tailoff){
//	printf("aoeu1\n");
}

c_nntp_file::~c_nntp_file(){
	t_nntp_file_parts::iterator i;
	for(i = parts.begin();i!=parts.end();++i){
		assert((*i).second);
		delete (*i).second;
	}
}


c_nntp_getinfo::c_nntp_getinfo(const string &pat, const string &temppat, const vector<string> &dupepaths, nntp_file_pred *pre,int flag):path(pat), temppath(temppat), pred(pre), flags(flag) {
	if (!(flags&GETFILES_NODUPEFILECHECK)) {
		for (vector<string>::const_iterator si=dupepaths.begin(); si!=dupepaths.end(); ++si)
			flist.addfrompath(*si);
		flist.addfrompath(path);
	}
}

static void nntp_cache_getfile(c_nntp_files_u *fc, ParHandler *parhandler, meta_mid_info *midinfo, const t_nntp_getinfo_list &getinfos, const c_nntp_file::ptr &f) {
	pair<t_nntp_files_u::const_iterator,t_nntp_files_u::const_iterator> firange;
	t_nntp_getinfo_list::const_iterator gii, giibegin=getinfos.begin(), giiend=getinfos.end();
	c_nntp_getinfo::ptr info;
	for (gii=giibegin; gii!=giiend; ++gii) {
		info = *gii;
		if ((info->flags&GETFILES_GETINCOMPLETE || f->iscomplete()) && (info->flags&GETFILES_NODUPEIDCHECK || !(midinfo->check(f->bamid()))) && (*info->pred)(f.gimmethepointer())){//matches user spec
			if (!(info->flags&GETFILES_AUTOPAR_DISABLING_FLAGS)) {
				if (parhandler->maybe_add_parfile(f, info->path, info->temppath))
					continue;
			}
			firange=fc->files.equal_range(f->badate());
			for (;firange.first!=firange.second;++firange.first){
				if ((*firange.first).second->file->bamid()==f->bamid())
					return;
			}

			if (!(info->flags&GETFILES_NODUPEFILECHECK) && info->flist.checkhavefile(f->subject.c_str(),f->bamid(),f->bytes())){
				if (info->flags&GETFILES_DUPEFILEMARK)
					midinfo->insert(f);
				continue;
			}
			fc->addfile(f,info->path,info->temppath);
			return;
		}
	}
}

void c_nntp_cache::getfiles(c_nntp_files_u *fc, ParHandler *parhandler, meta_mid_info *midinfo, const t_nntp_getinfo_list &getinfos) { 
	t_nntp_files::const_iterator fi;
	for(fi = files.begin();fi!=files.end();++fi){
		nntp_cache_getfile(fc, parhandler, midinfo, getinfos, (*fi).second);
	}
}

static bool cache_ismultiserver(const t_nntp_server_info &server_info) {
	int num=0;
	for (t_nntp_server_info::const_iterator sii=server_info.begin(); sii!=server_info.end(); ++sii)
		if (sii->second.num > 0)
			num++;
	return num > 1;
}
bool c_nntp_cache::ismultiserver(void) const {
	return cache_ismultiserver(server_info);
}

c_nntp_server_info* c_nntp_cache::getserverinfo(ulong serverid){
	t_nntp_server_info::iterator i = server_info.find(serverid);
	if (i != server_info.end())
		return &i->second;
	return &server_info.insert(t_nntp_server_info::value_type(serverid, serverid)).first->second;
}
int c_nntp_cache::additem(c_nntp_header *h){
	assert(h);
	c_nntp_file::ptr f;
	t_nntp_files::iterator i;
	pair<t_nntp_files::iterator, t_nntp_files::iterator> irange = files.equal_range(h);
//	t_nntp_files::const_iterator i;
//	pair<t_nntp_files::const_iterator, t_nntp_files::const_iterator> irange = files.equal_range(h->mid);

	c_nntp_server_info *servinfo=getserverinfo(h->serverid);
	if (h->articlenum > servinfo->high)
		servinfo->high = h->articlenum;
	if (h->articlenum < servinfo->low)
		servinfo->low = h->articlenum;
	servinfo->num++;

	saveit=1;
//	printf("%lu %s..",h->articlenum,h->subject.c_str());
	for (i=irange.first;i!=irange.second;++i){
		f=(*i).second;
		assert(!f.isnull());
		t_nntp_file_parts::iterator op;
		if ((op=f->parts.find(h->partnum))!=f->parts.end()){
			c_nntp_part *matchpart=(*op).second;
			if (matchpart->messageid==h->messageid){
				matchpart->addserverarticle(h);
				return 0;
			}
			PDEBUG(DEBUG_MED,"%s was gonna add, but already have this part(sub=%s part=%i omid=%s)?\n",h->messageid.c_str(),f->subject.c_str(),h->partnum,matchpart->messageid.c_str());
			continue;
		}
//			printf("adding\n");
		c_nntp_part *p=new c_nntp_part(h);
		f->addpart(p);
		totalnum++;
		return 0;
	}
//	printf("new\n");
	f=new c_nntp_file(h);
	c_nntp_part *p=new c_nntp_part(h);
	f->addpart(p);
	totalnum++;
	//files[f->subject.c_str()]=f;
	files.insert(t_nntp_files::value_type(f.gimmethepointer(),f));
	return 1;
}

void c_nntp_cache::getxrange(c_nntp_server_info *servinfo,ulong newlow,ulong newhigh, c_nrange *range) const {
	range->clear();
	range->insert(newlow<servinfo->low?newlow:servinfo->low,newhigh);
	getxrange(servinfo, range);
}
void c_nntp_cache::getxrange(c_nntp_server_info *servinfo, c_nrange *range) const {
	t_nntp_files::const_iterator i;
	c_nntp_file::ptr nf;
	t_nntp_file_parts::const_iterator pi;
	c_nntp_part *np;
	pair<t_nntp_server_articles::const_iterator,t_nntp_server_articles::const_iterator> sarange;
	c_nntp_server_article *sa;
	for(i = files.begin();i!=files.end();++i){
		nf=(*i).second;
		assert(!nf.isnull());
		assert(!nf->parts.empty());
		for(pi = nf->parts.begin();pi!=nf->parts.end();++pi){
			np=(*pi).second;
			assert(np);
			sarange=np->articles.equal_range(servinfo->serverid);
			while (sarange.first!=sarange.second){
				sa=(*sarange.first).second;
				assert(sa);
				range->remove(sa->articlenum);
				++sarange.first;
			}
		}
	}
}
ulong c_nntp_cache::flushlow(c_nntp_server_info *servinfo, ulong newlow, meta_mid_info *midinfo){
	assert(newlow>0);
	c_nrange flushrange;
	flushrange.insert(0, newlow-1);
	ulong r = flush(servinfo, flushrange, midinfo);
	servinfo->low=newlow;
	return r;
}
ulong c_nntp_cache::flush(c_nntp_server_info *servinfo, c_nrange flushrange, meta_mid_info *midinfo){
	ulong count=0,countp=0,countf=0;
	t_nntp_files::iterator i,in;
	c_nntp_file::ptr nf;
	t_nntp_file_parts::iterator pi,pic;
	c_nntp_part *np;
	pair<t_nntp_server_articles::iterator,t_nntp_server_articles::iterator> sarange;
	t_nntp_server_articles::iterator sai;
	c_nntp_server_article *sa;
	c_mid_info rel_midinfo("");
	//restrict the message to the range of headers we actually have, since showing 0-4294967295 or something isn't too useful ;0
	flushrange.remove(0,servinfo->low-1);
	flushrange.remove(servinfo->high+1,ULONG_MAX);
	if (flushrange.empty())
		return 0;
	if (quiet<2) {printf("Flushing headers %lu-%lu(%lu):", flushrange.low(), flushrange.high(), flushrange.get_total());fflush(stdout);}
	for(in = files.begin();in!=files.end();){
		i=in;
		++in;
		nf=(*i).second;
		assert(!nf.isnull());
		assert(!nf->parts.empty());
		for(pi = nf->parts.begin();pi!=nf->parts.end();){
			pic=pi;
			++pi;
			np=(*pic).second;
			assert(np);
			sarange=np->articles.equal_range(servinfo->serverid);
			while (sarange.first!=sarange.second){
				sai=sarange.first;
				++sarange.first;
				sa=(*sai).second;
				assert(sa);
				if (flushrange.check(sa->articlenum)){
					delete sa;
					np->articles.erase(sai);
					if (np->articles.empty()){
						if (count_partnum(np->partnum,nf->req)) nf->have--;
						midinfo->set_delete(np->messageid);
						delete np;
						np=NULL;
						nf->parts.erase(pic);
						countp++;
					}
					count++;
				}
			}
			if (np && midinfo->check(np->messageid)) rel_midinfo.insert(np->messageid);
		}
		if (nf->parts.empty()){
//			nf->dec_rcount();
//			delete nf;
			files.erase(i);
			countf++;
//#ifdef HAVE_HASH_MAP_H
//			in=files.begin();//not needed, apparantly.
//#endif
		}
	}
	servinfo->num-=count;
	totalnum-=countp;
#ifndef NDEBUG
	for(in = files.begin();in!=files.end();++in){
		nf=(*in).second;
		assert(!nf.isnull());
		assert(!nf->parts.empty());
		for(pi = nf->parts.begin();pi!=nf->parts.end();++pi){
			np=(*pi).second;
			assert(np);
			sai=np->articles.find(servinfo->serverid);
			if (sai!=np->articles.end()){
				sa=(*sai).second;
				assert(!flushrange.check(sa->articlenum));
			}
		}
	}
#endif
	if (quiet<2){printf(" %lu (%lu,%lu)\n",count,countp,countf);}
	if (count)saveit=1;

	midinfo->do_delete_fun(rel_midinfo);

	return count;
}
void setfilenamegz(string &file, int gz=-2){
#ifndef HAVE_LIBZ
	gz=0;
#endif
	if (gz==-2)
		gz=nconfig.usegz;
	if (gz)
		file.append(".gz");
}
c_file *dofileopen(string file, string mode, int gz=-2){
	c_file *f=NULL;
#ifndef HAVE_LIBZ
	gz=0;
#endif
	if (gz==-2)
		gz=nconfig.usegz;
#ifdef HAVE_LIBZ
	if (gz){
		if (gz>0){
			char blah[10];
			sprintf(blah,"%i",gz);
			mode.append(blah);
		}
		f=new c_file_gz(file.c_str(),mode.c_str());
	}
#endif
	if (!gz){
		f=new c_file_fd(file.c_str(),mode.c_str());
	}
	if (mode[0]=='r' || mode.find('+')>=0)
		f->initrbuf();
	return f;
}

enum {
	START_MODE=2,
	SERVERINFO_MODE=4,
	FILE_MODE=0,
	PART_MODE=1,
	SERVER_ARTICLE_MODE=3,
	REFERENCES_MODE=5,
};

class c_nntp_cache_reader {
	protected:
		c_file *f;
		meta_mid_info *midinfo;
		c_group_info::ptr group;
	public:
		int cache_sortver;
		ulong count,counta,curline,countdeada,totalnum;
		c_nntp_cache_reader(c_file *cf, meta_mid_info*mi, t_nntp_server_info &server_infoi, const c_group_info::ptr &grou);
		c_nntp_file::ptr read_file(void);
		const char *filename(void) const {return f->name();}
		void check_counts(void);
};

c_nntp_cache_reader::c_nntp_cache_reader(c_file *cf, meta_mid_info *mi, t_nntp_server_info &server_info, const c_group_info::ptr &grou):f(cf), midinfo(mi), group(grou){
	count=0;counta=0;curline=0;countdeada=0;totalnum=0;cache_sortver=-1;
	
	char *t[5];
	int i;
	if (f->beof())
		throw CacheEx(Ex_INIT, "unexpected EOF on cache file line %lu",curline);
	curline++;
	//(mode==START_MODE)
	i = f->btoks('\t',t,2);
	if (i==2 && (strcmp(t[0],CACHE_VERSION)==0)){
		totalnum=atoul(t[1]);
		char *subvercp=strchr(t[1], ' ');
		if (subvercp)
			cache_sortver = atoi(subvercp);
	}else{
		if (i>0 && strncmp(t[0],"NGET",4)==0)
			throw CacheEx(Ex_INIT,"cache is from a different version of nget");
		else
			throw CacheEx(Ex_INIT,"cache does not seem to be an nget cache file");
	}

	while (1) {
		if (f->beof())
			throw CacheEx(Ex_INIT, "unexpected EOF on cache file line %lu",curline);
		curline++;
		//(mode==SERVERINFO_MODE)
		if (f->bpeek()=='.') {
			if (f->bgetsp()[1]!=0) {
				printf("warning: stuff after . line %lu mode %i\n",curline,SERVERINFO_MODE);
				set_cache_warn_status();
			}
			//mode=FILE_MODE;//start new file mode
			return;
		}
		i = f->btoks('\t',t,4);
		if (i==4){
			ulong serverid=atoul(t[0]);
			if (nconfig.getserver(serverid)) {
				server_info.insert(t_nntp_server_info::value_type(serverid, c_nntp_server_info(serverid, atoul(t[1]), atoul(t[2]), atoul(t[3]))));
			}else{
				printf("warning: serverid %lu not found in server list\n",serverid);
				set_cache_warn_status();
			}
		}else{
			printf("invalid line %lu mode %i (%i toks)\n",curline,SERVERINFO_MODE,i);//mode);
			set_cache_warn_status();
		}
	}

}

c_nntp_file::ptr c_nntp_cache_reader::read_file(void) {
	int mode=FILE_MODE;
	//c_nntp_file	*nf=NULL;
	c_nntp_file::ptr nf=NULL;
	c_nntp_part	*np=NULL;
	c_nntp_server_article *sa;
	char *t[8];
	int i;
	while (!f->beof()){
		curline++;
		if (mode==SERVER_ARTICLE_MODE && np){//new server_article mode
			if (f->bpeek()=='.'){
				if (f->bgetsp()[1]!=0) {
					printf("warning: stuff after . line %lu mode %i\n",curline,mode);
					set_cache_warn_status();
				}
				mode=PART_MODE;//go back to new part mode
				continue;
			}else{
				i = f->btoks('\t',t,4);
				if (i==4){
					ulong serverid=atoul(t[0]);
					if (nconfig.getserver(serverid)) {
						sa=new c_nntp_server_article(serverid,group,atoul(t[1]),atoul(t[2]),atoul(t[3]));
						//np->addserverarticle(sa);
						np->articles.insert(t_nntp_server_articles::value_type(sa->serverid,sa));
						counta++;
					}else
						countdeada++;
				}else{
					printf("invalid line %lu mode %i (%i toks)\n",curline,mode,i);
					set_cache_warn_status();
				}
			}
		}
		else if (mode==PART_MODE && nf){//new part mode
			if (np && np->articles.empty()) {
				midinfo->set_delete(np->messageid);
				nf->parts.erase(np->partnum);
				delete np;
				np=NULL;
				count--;
			}
			if (f->bpeek()=='.'){
				if (f->bgetsp()[1]!=0) {
					printf("warning: stuff after . line %lu mode %i\n",curline,mode);
					set_cache_warn_status();
				}
				if (nf->parts.empty()){
					set_cache_warn_status();
					printf("empty nntp_file finished at line %lu mode %i\n",curline,mode);
					nf=NULL;
					np=NULL;
					mode=FILE_MODE;//go back to new file mode
				}else
					return nf;
			}else{
				i = f->btoks('\t',t,3);
				if (i==3){
					np=new c_nntp_part(atoi(t[0]),atoul(t[1]),t[2]);
					nf->addpart(np);//add at '.' section (above) ... o r not.
					count++;
					mode=SERVER_ARTICLE_MODE;//start adding server_articles
				}else{
					printf("invalid line %lu mode %i (%i toks)\n",curline,mode,i);
					set_cache_warn_status();
				}
			}
		}
		else if (mode==FILE_MODE){//new file mode
			i = f->btoks('\t',t,7);
			if (i==7){
				nf=new c_nntp_file(atoi(t[0]),atoul(t[1]),atoul(t[2]),t[3],t[4],atoi(t[5]),atoi(t[6]));
				mode=REFERENCES_MODE;
			}else{
				printf("invalid line %lu mode %i (%i toks)\n",curline,mode,i);
				set_cache_warn_status();
			}
		}
		else if (mode==REFERENCES_MODE && nf){//adding references on new file
			char *buf=f->bgetsp();
			if (buf[0]=='.' && buf[1]==0){
				mode=PART_MODE;
				np=NULL;
				continue;
			}else{
				if (buf[0]=='.') buf++;//unescape any invalid references that started with .
				nf->references.push_back(buf);
			}
		}else{
			assert(0);//should never get here
		}
	}
	if (nf)
		throw CacheEx(Ex_INIT, "unexpected EOF on cache file line %lu",curline);
	return NULL;
}
void c_nntp_cache_reader::check_counts(void) {
	if (countdeada){
		printf("warning: read (and ignored) %lu articles with bad serverids\n",countdeada);
		set_cache_warn_status();
	}
	if (count!=totalnum){
		printf("warning: read %lu parts from cache, expecting %lu\n",count,totalnum);
		totalnum=count;
		set_cache_warn_status();
	}
}

c_nntp_cache::c_nntp_cache(void):totalnum(0), saveit(0){
	fileread=-1;
}
c_nntp_cache::c_nntp_cache(string path,c_group_info::ptr group_,meta_mid_info *midinfo):totalnum(0),group(group_){
	saveit=0;
	//file=nid;
	c_file *f;
	file=path_join(path,group->group + ",cache");
	setfilenamegz(file,group->usegz);
	fileread=0;
	try {
		f=dofileopen(file.c_str(),"rb",group->usegz);
	}catch(FileNOENTEx &e){
		return;
	}
	auto_ptr<c_file> fcloser(f);
	try{
		c_nntp_cache_reader reader(f, midinfo, server_info, group_);
		c_nntp_file::ptr nf;
		while ((nf=reader.read_file()))
			files.insert(t_nntp_files::value_type(nf.gimmethepointer(),nf));
		fileread=1;
		if (reader.cache_sortver!=CACHE_SORTVER)
			saveit=1; //if the cache is from a version with different sorting, force saving it with new sorting even if nothing is changed otherwise.
		PDEBUG(DEBUG_MIN,"read %lu parts (%lu sa) %lu files",reader.count,reader.counta,(ulong)files.size());
		reader.check_counts();
		totalnum = reader.totalnum;
	} catch (CacheEx &e) {
		set_cache_warn_status();
		printf("%s: %s\n", file.c_str(), e.getExStr());
	}
	f->close();
}
c_nntp_cache::~c_nntp_cache(){
	t_nntp_files::iterator i;
	if (fileread!=-1 && saveit && (fileread || !files.empty())){
		string tmpfn;
		tmpfn=file+".tmp";
		try {
			c_file *f=dofileopen(tmpfn,"wb",group->usegz);
			ulong count=0,counta=0;
			try {
				auto_ptr<c_file> fcloser(f);
				if (quiet<2){printf("saving cache: %lu parts, %lu files..",totalnum,(ulong)files.size());fflush(stdout);}
				c_nntp_file::ptr nf;
				t_references::iterator ri;
				t_nntp_file_parts::iterator pi;
				t_nntp_server_articles::iterator sai;
				c_nntp_server_article *sa;
				c_nntp_part *np;
				f->putf(CACHE_VERSION"\t%lu %i\n",totalnum,CACHE_SORTVER);//START_MODE
				//vv SERVERINFO_MODE
				for (t_nntp_server_info::const_iterator sii=server_info.begin(); sii!=server_info.end(); ++sii) {
					const c_nntp_server_info &si = sii->second;
					f->putf("%lu\t%lu\t%lu\t%lu\n",si.serverid,si.high,si.low,si.num);//mode 4
				}
				f->putf(".\n");
				//end SERVERINFO_MODE
				//vv FILE_MODE
				for(i = files.begin();i!=files.end();++i){
					nf=(*i).second;
					assert(!nf.isnull());
					assert(!nf->parts.empty());
					f->putf("%i\t%lu\t%lu\t%s\t%s\t%i\t%i\n",nf->req,nf->flags,nf->fileid,nf->subject.c_str(),nf->author.c_str(),nf->partoff,nf->tailoff);//FILE_MODE
					for(ri = nf->references.begin();ri!=nf->references.end();++ri){
						if ((*ri)[0]=='.') f->putf("."); //escape possible invalid references that might start with .
						f->putf("%s\n",ri->c_str());//REFERENCES_MODE
					}
					f->putf(".\n");//end REFERENCES_MODE
					for(pi = nf->parts.begin();pi!=nf->parts.end();++pi){
						np=(*pi).second;
						assert(np);
						f->putf("%i\t%lu\t%s\n",np->partnum,np->date,np->messageid.c_str());//PART_MODE
						for (sai = np->articles.begin(); sai != np->articles.end(); ++sai){
							sa=(*sai).second;
							assert(sa);
							f->putf("%lu\t%lu\t%lu\t%lu\n",sa->serverid,sa->articlenum,sa->bytes,sa->lines);//SERVER_ARTICLE_MODE
							counta++;
						}
						f->putf(".\n");//end SERVER_ARTICLE_MODE
						count++;
					}
					f->putf(".\n");//end PART_MODE
					(*i).second=NULL; //free cache as we go along instead of at the end, so we don't swap more with low-mem.
					//nf->storef(f);
					//delete nf;
					//nf->dec_rcount();
				}
				f->close();
			}catch(FileEx &e){
				printCaughtEx(e);
				if (unlink(tmpfn.c_str()))
					perror("unlink:");
				fatal_exit();
			}
			if (quiet<2) printf(" done. (%lu sa)\n",counta);
			if (count!=totalnum){
				printf("warning: wrote %lu parts from cache, expecting %lu\n",count,totalnum);
				set_cache_warn_status();
			}
			xxrename(tmpfn.c_str(), file.c_str());
			return;
		}catch (FileEx &e){
			printCaughtEx(e);
			fatal_exit();
		}
	}

	if (quiet<2){printf("freeing cache: %lu parts, %lu files..\n",totalnum,(ulong)files.size());}//fflush(stdout);}

//	for(i = files.begin();i!=files.end();++i){
		//delete (*i).second;
//		(*i).second->dec_rcount();
//	}
//	if (!quiet) printf(" done.\n");
}
c_nntp_files_u::~c_nntp_files_u(){
//	t_nntp_files_u::iterator i;
//	for(i = files.begin();i!=files.end();++i){
//		(*i).second->dec_rcount();
//	}
}

static inline bool ltfp(const c_nntp_file::ptr &f1, const c_nntp_file::ptr &f2) {
	return *f1 < *f2;
}
	
void nntp_cache_getfiles(c_nntp_files_u *fc, ParHandler *parhandler, bool *ismultiserver, string path, const vector<c_group_info::ptr> &groups, meta_mid_info*midinfo, const t_nntp_getinfo_list &getinfos){
	set<ulong> usedservers;
	auto_vector<c_file> cachefiles;
	vector<t_nntp_server_info> server_infos;
	vector<c_nntp_cache_reader> readers;
	vector<c_nntp_file::ptr> nfiles;
	ulong mergedcount=0, numfiles=0, mergedfiles=0, count=0, counta=0;
	c_nntp_file::ptr nf, mergef;
	for (vector<c_group_info::ptr>::const_iterator gi=groups.begin(); gi!=groups.end(); ++gi) {
		const c_group_info::ptr &group = *gi;
		string file=path_join(path,group->group + ",cache");
		setfilenamegz(file,group->usegz);
		c_file *f=NULL;
		try {
			f=dofileopen(file.c_str(),"rb",group->usegz);
		}catch(FileNOENTEx &e){
			//pass
		}
		
		if (f) {
			cachefiles.push_back(f);
			try{
				t_nntp_server_info server_info;
				c_nntp_cache_reader reader(f, midinfo, server_info, group);
				
				if (reader.cache_sortver!=CACHE_SORTVER)
					throw CacheEx(Ex_INIT, "cache file must be updated with this version of nget before it can be used with metagrouping");

				for (t_nntp_server_info::const_iterator sii=server_info.begin(); sii!=server_info.end(); ++sii)
					if (sii->second.num > 0)
						usedservers.insert(sii->first);
				
				if ((nf=reader.read_file())) {
					//printf("initial file %i\n", nfiles.size());
					nfiles.push_back(nf);//initialize with first nntp_file.
					readers.push_back(reader);
					numfiles++;
				}
				
			} catch (CacheEx &e) {
				set_cache_warn_status();
				printf("%s: %s\n", file.c_str(), e.getExStr());
			}
		}
	}

	*ismultiserver = usedservers.size() > 1;

	vector<c_nntp_file::ptr>::iterator nfi_m;

	while (!nfiles.empty()) {
		nfi_m = min_element(nfiles.begin(), nfiles.end(), ltfp);
		mergef = *nfi_m;
		//printf("pre-loop. nfiles.size=%u, merged=%lu, numfiles=%lu\n", nfiles.size(), mergedfiles, numfiles);
		for (unsigned i = 0; i<nfiles.size();) {
			//printf("loop start. i=%u nfiles.size=%u, merged=%lu, numfiles=%lu\n",i, nfiles.size(), mergedfiles, numfiles);
			if (nfiles[i]==mergef || *nfiles[i]==*mergef) {
				//printf("file %u equals min\n", i);
				if (nfiles[i]!=mergef){
					//printf("file %u merging\n", i);
					mergef->mergefile(nfiles[i]);
				}
				if (nfiles[i]==mergef || nfiles[i]->parts.empty()) {
					try{
						//printf("reading file %u\n", i);
						nf=readers[i].read_file();
					} catch (CacheEx &e) {
						nf=NULL;
						set_cache_warn_status();
						printf("%s: %s\n", readers[i].filename(), e.getExStr());
					}
					//printf("file %u = %p\n", i, nf.gimmethepointer());
					if (nf) {
						assert(!(nf < nfiles[i]));
						numfiles++;
						nfiles[i]=nf;
					} else {
						nfiles.erase(nfiles.begin()+i);
						count+=readers[i].count;
						counta+=readers[i].counta;
						readers[i].check_counts();
						readers.erase(readers.begin()+i);
						continue;
					}
				}
			}
			++i;
		}
		//printf("post-loop. nfiles.size=%u, merged=%lu, numfiles=%lu\n", nfiles.size(), mergedfiles, numfiles);
		nntp_cache_getfile(fc, parhandler, midinfo, getinfos, mergef);
		mergedfiles++;
		mergedcount+=mergef->parts.size();
	}
	
	PDEBUG(DEBUG_MIN,"scanned %lu parts %lu files (total: %lu parts (%lu sa) %lu files)",mergedcount,mergedfiles,count,counta,numfiles);

	for (vector<c_nntp_cache_reader>::iterator cri=readers.begin(); cri!=readers.end(); ++cri) 
		cri->check_counts();

	for (auto_vector<c_file>::iterator cfi=cachefiles.begin(); cfi!=cachefiles.end(); ++cfi)
		(*cfi)->close();
}

void nntp_cache_getfiles(c_nntp_files_u *fc, ParHandler *parhandler, bool *ismultiserver, string path, c_group_info::ptr group, meta_mid_info*midinfo, const t_nntp_getinfo_list &getinfos){

	string file=path_join(path,group->group + ",cache");
	setfilenamegz(file,group->usegz);
	c_file *f;
	try {
		f=dofileopen(file.c_str(),"rb",group->usegz);
	}catch(FileNOENTEx &e){
		return;
	}
	auto_ptr<c_file> fcloser(f);
	try{
		t_nntp_server_info server_info;
		ulong numfiles=0;
		c_nntp_cache_reader reader(f, midinfo, server_info, group);
		*ismultiserver = cache_ismultiserver(server_info);
		c_nntp_file::ptr nf;

		while ((nf=reader.read_file())) {
			nntp_cache_getfile(fc, parhandler, midinfo, getinfos, nf);
			numfiles++;
		}

		PDEBUG(DEBUG_MIN,"scanned %lu parts (%lu sa) %lu files",reader.count,reader.counta,numfiles);
		reader.check_counts();
	} catch (CacheEx &e) {
		set_cache_warn_status();
		printf("%s: %s\n", file.c_str(), e.getExStr());
	}
	f->close();
}



#define MID_INFO_MIN_KEEP (14*24*60*60)
#define MID_INFO_MIN_KEEP_DEL (7*24*60*60)
void c_mid_info::do_delete_fun(const c_mid_info &rel_mid){
	t_message_state_list::iterator i=states.begin();
	c_message_state::ptr s;
	int deld=0;
	time_t curtime=time(NULL);
	for (;i!=states.end();++i){
		s=(*i).second;
		if (rel_mid.check(s->messageid))
			continue;
		if ((s->date_removed==TIME_T_MAX1 && s->date_added+MID_INFO_MIN_KEEP<curtime) || (s->date_added+MID_INFO_MIN_KEEP<curtime && s->date_removed+MID_INFO_MIN_KEEP_DEL<curtime)){
//			delete s;
//			states.erase(i);
//			i=states.begin();//urgh.
			s->date_removed=TIME_T_DEAD;//let em just not get saved.
			changed=1;deld++;
		}
	}
	PDEBUG(DEBUG_MIN,"c_mid_info::do_delete_fun: %i killed",deld);
}
c_mid_info::c_mid_info(string path){
	load(path);
}
void c_mid_info::load(string path,bool merge,bool lock){
	if (!merge){
		clear();
		changed=0;
	}
	if (path.empty())
		return;
	c_file *f=NULL;
	if (!merge)
		setfilenamegz(path);//ugh, hack.
		file=path;
	int line=0;
	//c_lockfile locker(path,WANT_SH_LOCK);
	auto_ptr<c_lockfile> locker;
	if (lock)
		locker.reset(new c_lockfile(path,WANT_SH_LOCK));
//	c_regex_r midre("^(.+) ([0-9]+) ([0-9]+)$");
	char *t[3];
	int i;
	try {
		f=dofileopen(path.c_str(),"rb");
	}catch(FileNOENTEx &e){
		return;
	}
	auto_ptr<c_file> fcloser(f);
	while (!f->beof()){
		line++;
		i = f->btoks(' ',t,3);
		if (i==3)
			insert_full(t[0],atol(t[1]),atol(t[2]));//TODO: shouldn't set changed flag if no new ones are actually merged.
		else {
			printf("c_mid_info::load: invalid line %i (%i toks)\n",line,i);
			set_cache_warn_status();
		}
	}
	f->close();
	PDEBUG(DEBUG_MIN,"c_mid_info::load read %i lines",line);
	if (!merge)
		changed=0;
	return;
}
c_mid_info::~c_mid_info(){
	try {
		save();
	} catch (FileEx &e) {
		printCaughtEx(e);
		fatal_exit();
	}
	clear();
}
void c_mid_info::save(void){
	if (!changed)
		return;
	if (file.empty())
		return;
	c_file *f=NULL;
	c_lockfile locker(file,WANT_EX_LOCK);//lock before we read, so that multiple copies trying to save at once don't lose changes.
	{
		unsigned long count1=states.size();
		load(file,1,0);//merge any changes that might have happened
		if (count1!=states.size()){
			if (debug){printf("saving mid_info: merged something...(%lu)\n",(ulong)states.size()-count1);}
		}
	}
	int nums=0;
	string tmpfn=file+".tmp";
	f=dofileopen(tmpfn,"wb");
	try {
		auto_ptr<c_file> fcloser(f);
		if (debug){printf("saving mid_info: %lu infos..",(ulong)states.size());fflush(stdout);}
		t_message_state_list::iterator sli;
		c_message_state::ptr ms;
		for (sli=states.begin(); sli!=states.end(); ++sli){
			ms=(*sli).second;
			if (ms->date_removed==TIME_T_DEAD)
				continue;
			f->putf("%s %li %li\n",ms->messageid.c_str(),ms->date_added,ms->date_removed);
			nums++;
		}
		if (debug) printf(" (%i) done.\n",nums);
		f->close();
	}catch(FileEx &e){
		if (unlink(tmpfn.c_str()))
			perror("unlink:");
		throw;
	}
	xxrename(tmpfn.c_str(), file.c_str());
	return;
}

