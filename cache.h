/*
    cache.* - nntp header cache code
    Copyright (C) 1999-2004  Matthew Mueller <donut AT dakotacom.net>

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
#ifndef _CACHE_H_
#define _CACHE_H_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <ctype.h>
#include <string>
#include <map>
#include "_hash_map.h"
#include <list>
#include <vector>
#include "file.h"
#include "log.h"

#include "stlhelp.h"
#include "etree.h"

#include "rcount.h"

#include "server.h"

#include "nrange.h"
#include "dupe_file.h"

#define CACHE_VERSION "NGET5"
#define CACHE_SORTVER (1)

typedef vector<string> t_references;

typedef unsigned long t_id;

class c_nntp_file_base {
	public:
		int req;
		int partoff;
		string author;
		string subject;
		t_references references;
		c_nntp_file_base(int r, int po, const char *a, const char *s): req(r), partoff(po), author(a), subject(s) {}
//		c_nntp_file_base(t_id fi, int r, int po, const string &a, const string &s, const t_references& refs): fileid(fi), req(r), partoff(po), author(a), subject(s), references(refs) {}
//		c_nntp_file_base(const c_nntp_file_base &fb): fileid(fb.fileid), req(fb.req), partoff(fb.partoff), author(fb.author), subject(fb.subject), references(fb.references) {}
		c_nntp_file_base() {}
		bool operator< (const c_nntp_file_base &fb) const {
			if (req<fb.req) return true;
			if (req>fb.req) return false;
			if (partoff<fb.partoff) return true;
			if (partoff>fb.partoff) return false;
			if (author<fb.author) return true;
			if (author>fb.author) return false;
			if (subject<fb.subject) return true;
			if (subject>fb.subject) return false;
			if (references<fb.references) return true;
			//if (references>fb.references) return false;
			return false;
		}
		bool operator== (const c_nntp_file_base &fb) const {
			return (req==fb.req && partoff==fb.partoff && author==fb.author && subject==fb.subject && references==fb.references);
		}
};

class c_nntp_header : public c_nntp_file_base {
	private:
		int parsepnum(const char *str,const char *soff);
	public:
		ulong serverid;
		c_group_info::ptr group;
		int partnum;
		int tailoff;
		ulong articlenum;
		time_t date;
		ulong bytes,lines;
		string messageid;
		void set(char *s,const char *a,ulong anum,time_t d,ulong b,ulong l,const char *mid,char *refstr);//note: modifies refstr
//		c_nntp_header(char *s,const char *a,ulong anum,time_t d,ulong b,ulong l);
};


class c_nntp_server_article {
	public:
		ulong serverid;
		c_group_info::ptr group;
		ulong articlenum;
		ulong bytes,lines;
		c_nntp_server_article(ulong serverid,const c_group_info::ptr &group,ulong articlenum,ulong bytes,ulong lines);
};
typedef multimap<ulong,c_nntp_server_article*> t_nntp_server_articles;
typedef pair<c_nntp_server_article*,c_server::ptr> t_real_server_article;
typedef multimap<float,t_real_server_article,greater<float> > t_nntp_server_articles_prioritized;
class c_nntp_part {
	public:
		int partnum;
		t_nntp_server_articles articles;
		time_t date;
//		ulong apxbytes,apxlines;//approximate/hrmy.
		string messageid;
		c_nntp_server_article *get_best_sa(void) const {
			t_nntp_server_articles::const_iterator nsai(articles.begin());
			c_nntp_server_article *sa;
			c_nntp_server_article *highest_sa=NULL;
			float highprio=-10000.0,f;
			for (;nsai!=articles.end();++nsai) {
				sa=(*nsai).second;
				for (t_server_list_range servers = nconfig.getservers(sa->serverid); servers.first!=servers.second; ++servers.first)
					if ((f=nconfig.trustsizes->getserverpriority(servers.first->second)) > highprio){
						highest_sa=sa;
						highprio=f;
					}
			}
			return highest_sa;
		}
		ulong bytes(void) const {return get_best_sa()->bytes;}
		ulong lines(void) const {return get_best_sa()->lines;}
		c_nntp_part(int pn, time_t d,const string &mid):partnum(pn),date(d),messageid(mid){};
		c_nntp_part(c_nntp_header *h);
		~c_nntp_part();
		void addserverarticle(c_nntp_header *h);
		c_nntp_server_article *getserverarticle(ulong serverid);
};


typedef map<int,c_nntp_part*> t_nntp_file_parts;

//#define FILEFLAG_READ 1

typedef map<ulong,int> t_server_have_map;

class c_nntp_file : public c_nntp_file_base, public c_refcounted<c_nntp_file>{
	public:
		t_nntp_file_parts parts;
		int have;
//		ulong bytes,lines;
		ulong flags;
		int tailoff;
		time_t update;
		void addpart(c_nntp_part *p);
		void addnewpart(c_nntp_part *p);
		void mergefile(c_nntp_file::ptr &f);
		bool is_a_reply(void) const {return (!references.empty()) || (subject.size()>=4 && tolower(subject[0])=='r' && tolower(subject[1])=='e' && subject[2]==':' && subject[3]==' ');}
		bool maybe_a_textreply(void) const {return (have<=1 && is_a_reply() && lines()<1000);}
		bool maybe_a_zerofile(void) const {return (req==0) && (partoff>=0) && (have==1);}
		bool maybe_a_textpost(void) const {return maybe_a_zerofile() || maybe_a_textreply();}
		bool iscomplete(void) const {return (have>=req) || maybe_a_textreply();}
		void get_server_have_map(t_server_have_map &have_map) const;
//		ulong banum(void){assert(!parts.empty());return (*parts.begin()).second->articlenum;}
		string bamid(void) const {assert(!parts.empty());return (*parts.begin()).second->messageid;}
		time_t badate(void) const {assert(!parts.empty());return (*parts.begin()).second->date;}
#define HAPPYSIZEFUNC2(T)		ulong T(void) const {\
			ulong b=0;\
			t_nntp_file_parts::const_iterator nfpi(parts.begin());\
			for (;nfpi!=parts.end();++nfpi){\
				b+=(*nfpi).second->T();\
			}\
			return b;\
		}
		t_id getfileid(void) const;
		HAPPYSIZEFUNC2(bytes)
		HAPPYSIZEFUNC2(lines)
		c_nntp_file(c_nntp_header *h);
		c_nntp_file(int r,ulong f,const char *s,const char *a,int po,int to,time_t update);
		virtual ~c_nntp_file();
};


struct ltfb {
	bool operator()(const c_nntp_file_base *fb1, const c_nntp_file_base *fb2) const {
		return *fb1 < *fb2;
	}
};
typedef multimap<c_nntp_file_base*, c_nntp_file::ptr, ltfb> t_nntp_files;

class c_nntp_file_retr : public c_refcounted<c_nntp_file_retr>{
	public:
		string path;
		string temppath;
		c_nntp_file::ptr file;
		bool dupecheck;
		c_nntp_file_retr(const string &p, const string &tp, const c_nntp_file::ptr &f, bool dupec):path(p),temppath(tp),file(f),dupecheck(dupec){}
};
typedef multimap<time_t,c_nntp_file_retr::ptr> t_nntp_files_u;
class c_nntp_files_u {
	public:
		uint_fast64_t bytes, lines;
		t_nntp_files_u files;
		void addfile(c_nntp_file::ptr f, const string &path, const string &temppath, bool dupecheck=true) {
			files.insert(t_nntp_files_u::value_type(f->badate(), new c_nntp_file_retr(path,temppath,f,dupecheck)));
			lines+=f->lines();
			bytes+=f->bytes();
		}
		c_nntp_files_u(void):bytes(0),lines(0){}
		~c_nntp_files_u();
};


class c_nntp_server_info {
	public:
		ulong serverid;
		ulong high,low,num;
		void reset(void){high=0;low=ULONG_MAX;num=0;}
		c_nntp_server_info(ulong sid):serverid(sid){reset();}
		c_nntp_server_info(ulong sid,ulong hig,ulong lo,ulong nu):serverid(sid),high(hig),low(lo),num(nu){}
};
typedef map<ulong,c_nntp_server_info> t_nntp_server_info;

class c_message_state : public c_refcounted<c_message_state>{
	public:
		string messageid;
		time_t date_added,date_removed;
		c_message_state(string mid,time_t da,time_t dr):messageid(mid),date_added(da),date_removed(dr){}
};

#ifdef HAVE_WORKING_HASH_MAP
typedef hash_map<const char*, c_message_state::ptr, hash<const char*>, eqstr> t_message_state_list;
#else
typedef map<const char*, c_message_state::ptr, ltstr> t_message_state_list;
#endif

//hrm.
#define TIME_T_MAX INT_MAX
#define TIME_T_MAX1 (TIME_T_MAX-1)
#define TIME_T_DEAD TIME_T_MAX

class c_mid_info {
	protected:
		string file;
		int changed;
		t_message_state_list states;
		void insert_full(string mid, time_t a, time_t d){
			t_message_state_list::iterator i=states.find(mid.c_str());
			c_message_state::ptr s;
			if (i!=states.end()){
				s=(*i).second;
//				if ((*i).second->changed)return;/arrrr
				if (d==TIME_T_MAX1 && s->date_removed!=TIME_T_MAX1) return;//ours has been deleted but not what we merging
				if (s->date_removed!=TIME_T_MAX1 && s->date_removed > d) return; //both are deleted, but ours has more recent time?
				states.erase(i);
			}
			s=new c_message_state(mid,a,d);
			states.insert(t_message_state_list::value_type(s->messageid.c_str(),s));
		}
	public:
		int check(const string &mid) const {
			if (states.find(mid.c_str())!=states.end())
				return 1;
			return 0;
		}
		void insert(const string &mid){
			if (check(mid))return;
			changed=1;
			c_message_state::ptr s=new c_message_state(mid,time(NULL),TIME_T_MAX1);
			states.insert(t_message_state_list::value_type(s->messageid.c_str(),s));
			return;
		}
		void remove(const string &mid){
			t_message_state_list::iterator i=states.find(mid.c_str());
			if (i==states.end())
				return;
			(*i).second->date_removed=TIME_T_DEAD;
			changed=1;
		}
		void clear(void){
			if (!states.empty()){
				states.clear();
				changed=1;
			}
		}
		void set_delete(const string &mid){
			t_message_state_list::iterator i=states.find(mid.c_str());
			if (i!=states.end()){
				(*i).second->date_removed=time(NULL);
			}
		}
		void do_delete_fun(const c_mid_info &rel_mid);
		void load(string path,bool merge=0,bool lock=1);
		void save(void);
		c_mid_info(string path);
		~c_mid_info();
};

typedef map<string, c_mid_info *> t_mid_info_list;
class meta_mid_info {
	protected:
		t_mid_info_list midinfos;
		void add_mid_info(const string &path, const c_group_info::ptr &group){
			midinfos.insert(t_mid_info_list::value_type(group->group, new c_mid_info(path + group->group + ",midinfo")));
		}
	public:
		bool check(const string &mid) const {
			for (t_mid_info_list::const_iterator mili=midinfos.begin(); mili!=midinfos.end(); ++mili)
				if ((*mili).second->check(mid))
					return true;
			return false;
		}
		void insert(const c_nntp_file::ptr &f){
			const string &mid=f->bamid();
			c_nntp_part *p = f->parts.begin()->second;
			for (t_nntp_server_articles::iterator sai=p->articles.begin(); sai!=p->articles.end(); ++sai)
				midinfos.find(sai->second->group->group)->second->insert(mid);
		}
		void remove(const string &mid){
			for (t_mid_info_list::iterator mili=midinfos.begin(); mili!=midinfos.end(); ++mili)
				(*mili).second->remove(mid);
		}
		void set_delete(const string &mid){
			for (t_mid_info_list::iterator mili=midinfos.begin(); mili!=midinfos.end(); ++mili)
				(*mili).second->set_delete(mid);
		}
		void do_delete_fun(const c_mid_info &rel_mid){
			for (t_mid_info_list::iterator mili=midinfos.begin(); mili!=midinfos.end(); ++mili)
				(*mili).second->do_delete_fun(rel_mid);
		}
		
		meta_mid_info(string path, const vector<c_group_info::ptr> &groups){
			for (vector<c_group_info::ptr>::const_iterator gi=groups.begin(); gi!=groups.end(); ++gi)
				add_mid_info(path, *gi);
		}
		meta_mid_info(string path, const c_group_info::ptr &group) {
			add_mid_info(path, group);
		}
		~meta_mid_info(){
			for (t_mid_info_list::iterator mili=midinfos.begin(); mili!=midinfos.end(); ++mili)
				delete mili->second;
		}
};

class c_xpat : public c_refcounted<c_xpat>{
	public:
		string field;
		string wildmat;
		c_xpat(const string &fiel,const string &wildma):field(fiel), wildmat(wildma){ }
};
typedef list<c_xpat::ptr> t_xpat_list;

class c_nntp_getinfo : public c_refcounted<c_nntp_getinfo>{
	public:
		string path;
		string temppath;
		nntp_file_pred *pred;
		int flags;
		dupe_file_checker flist;
		c_nntp_getinfo(const string &pat, const string &temppat, const vector<string> &dupepaths, nntp_file_pred *pre,int flag);
		~c_nntp_getinfo() { delete pred; }
};
typedef list<c_nntp_getinfo::ptr> t_nntp_getinfo_list;

class ParHandler;
class c_nntp_cache : public c_refcounted<c_nntp_cache>{
	public:
		string file;
		t_nntp_files files;
		ulong totalnum;
//		ulong high,low,num;
		t_nntp_server_info server_info;
		c_nntp_server_info*getserverinfo(ulong serverid);
		c_group_info::ptr group;
		int saveit;
		int fileread;
		bool ismultiserver(void) const;
		//int additem(ulong an,char *s,const char * a,time_t d, ulong b, ulong l){
		int additem(c_nntp_header *h);
		ulong flush(c_nntp_server_info *servinfo, c_nrange flushrange, meta_mid_info *midinfo);
		ulong flushlow(c_nntp_server_info *servinfo, ulong newlow, meta_mid_info *midinfo);
		void getxrange(c_nntp_server_info *servinfo, ulong newlow, ulong newhigh, c_nrange *range) const;
		void getxrange(c_nntp_server_info *servinfo, c_nrange *range) const;
		void getfiles(c_nntp_files_u *fc, ParHandler *parhandler, meta_mid_info *midinfo, const t_nntp_getinfo_list &getinfos);
		c_nntp_cache(void);
		c_nntp_cache(string path,c_group_info::ptr group,meta_mid_info*midinfo);
		virtual ~c_nntp_cache();
};

void nntp_cache_getfiles(c_nntp_files_u *fc, ParHandler *parhandler, bool *ismultiserver, string path, c_group_info::ptr group, meta_mid_info*midinfo, const t_nntp_getinfo_list &getinfos);
void nntp_cache_getfiles(c_nntp_files_u *fc, ParHandler *parhandler, bool *ismultiserver, string path, const vector<c_group_info::ptr> &groups, meta_mid_info*midinfo, const t_nntp_getinfo_list &getinfos);

#endif
