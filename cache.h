/*
    cache.* - nntp header cache code
    Copyright (C) 1999-2002  Matthew Mueller <donut@azstarnet.com>

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
#include <string>
//#include <map.h>
#include <map>//#### hmmmm? needed for cygwin instead of map.h?
#ifdef HAVE_HASH_MAP_H
#include <hash_map.h>
//#else
//#include <multimap.h>
#endif
#include <multimap.h>
#include <list.h>
#include <vector>
#include "file.h"
#include "log.h"

#include "stlhelp.h"
#include "etree.h"

#include "rcount.h"

#include "server.h"

#include "nrange.h"

#define CACHE_VERSION "NGET4"

typedef vector<string> t_references;

typedef unsigned long t_id;
class c_nntp_header {
	private:
		int parsepnum(const char *str,const char *soff);
		void setfileid(char *refstr, unsigned int refstrlen);
	public:
		ulong serverid;
		int partnum, req;
		int partoff, tailoff;
		t_id fileid;
		string subject;
		string author;
		ulong articlenum;
		time_t date;
		ulong bytes,lines;
		string messageid;
		t_references references;
		void set(char *s,const char *a,ulong anum,time_t d,ulong b,ulong l,const char *mid,char *refstr);//note: modifies refstr
//		c_nntp_header(char *s,const char *a,ulong anum,time_t d,ulong b,ulong l);
};


class c_nntp_server_article {
	public:
		ulong serverid;
		ulong articlenum;
		ulong bytes,lines;
		c_nntp_server_article(ulong serverid,ulong articlenum,ulong bytes,ulong lines);
};
typedef multimap<ulong,c_nntp_server_article*> t_nntp_server_articles;
typedef multimap<float,c_nntp_server_article*,greater<float> > t_nntp_server_articles_prioritized;
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
				if ((f=nconfig.trustsizes->getserverpriority(sa->serverid)) > highprio){
					highest_sa=sa;
					highprio=f;
				}
			}
			return highest_sa;
		}
		ulong bytes(void) const {return get_best_sa()->bytes;}
		ulong lines(void) const {return get_best_sa()->lines;}
		c_nntp_part(int pn, time_t d,const char *mid):partnum(pn),date(d),messageid(mid){};
		c_nntp_part(c_nntp_header *h);
		~c_nntp_part();
		void addserverarticle(c_nntp_header *h);
		c_nntp_server_article *getserverarticle(ulong serverid);
};


typedef map<int,c_nntp_part*> t_nntp_file_parts;

//#define FILEFLAG_READ 1

typedef map<ulong,int> t_server_have_map;

class c_nntp_file : public c_refcounted<c_nntp_file>{
	public:
		t_nntp_file_parts parts;
		int req,have;
//		ulong bytes,lines;
		ulong flags;
		t_id fileid;
		string subject,author;
		int partoff,tailoff;
		t_references references;
		void addpart(c_nntp_part *p);
		bool iscomplete(void) const {return (have>=req) || (have<=1 && !references.empty() && lines()<1000);}
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
		HAPPYSIZEFUNC2(bytes)
		HAPPYSIZEFUNC2(lines)
		c_nntp_file(c_nntp_header *h);
		c_nntp_file(int r,ulong f,t_id fi,const char *s,const char *a,int po,int to);
		virtual ~c_nntp_file();
};

#if 0
typedef e_unary_function<c_nntp_file_ptr,bool> nntp_pred;

template <class Op>
struct e_nntp_file : public e_binary_function<c_nntp_file_ptr, typename Op::arg2_type, typename Op::ret_type> {
//        typedef typename Op::arg2_type arg2_type;
//      arg2_type val2;
        Op O;

//      e_binder2nd(const arg2_type v):val2(v){};
        virtual ret_type operator()(const arg1_type v,const arg2_type val2) const=0;
};
template <class Op>
struct e_nntp_file_lines : public e_nntp_file<Op> {
        ret_type operator()(const arg1_type v,const arg2_type val2) const {return O(v->lines,val2);}
};
template <class Op>
struct e_nntp_file_subject : public e_nntp_file<Op> {
        ret_type operator()(const arg1_type v,const arg2_type val2) const {return O(v->subject,val2);}
};
/*
template <class T1,class T2,class RT>
struct e_nntp_file : public e_binary_function<c_nntp_file *, T2, typename RT> {
//        typedef typename Op::arg2_type arg2_type;
//      arg2_type val2;
//        Op O;
	e_binary_function<T1,T2,RT> *O;

//      e_binder2nd(const arg2_type v):val2(v){};
        virtual ret_type operator()(const T1 v,const T2 val2) const=0;
};
template <class Op>
struct e_nntp_file_lines : public e_nntp_file<Op> {
        ret_type operator()(const arg1_type v,const arg2_type val2) const {return O(v->lines,val2);}
};
template <class Op>
struct e_nntp_file_subject : public e_nntp_file<typename Op::arg1_type,typename Op::arg2_type,typename Op::ret_type> {
        ret_type operator()(const arg1_type v,const arg2_type val2) const {return (*O)(v->subject,val2);}
};
*/
#endif


//typedef hash_map<const char*, c_nntp_file*, hash<const char*>, eqstr> t_nntp_files;
#ifdef HAVE_HASH_MAP_H
typedef hash_multimap<t_id, c_nntp_file::ptr> t_nntp_files;
#else
typedef multimap<t_id, c_nntp_file::ptr> t_nntp_files;
#endif
//typedef map<ulong,c_nntp_file*,less<ulong> > t_nntp_files_u;
class c_nntp_file_retr : public c_refcounted<c_nntp_file_retr>{
	public:
		string path;
		string temppath;
		c_nntp_file::ptr file;
		c_nntp_file_retr(const string &p, const string &tp, const c_nntp_file::ptr &f):path(p),temppath(tp),file(f){}
};
typedef multimap<time_t,c_nntp_file_retr::ptr> t_nntp_files_u;
class c_nntp_files_u {
	public:
		uint_fast64_t bytes, lines;
		t_nntp_files_u files;
		void addfile(c_nntp_file::ptr f, const string &path, const string &temppath) {
			files.insert(t_nntp_files_u::value_type(f->badate(), new c_nntp_file_retr(path,temppath,f)));
			lines+=f->lines();
			bytes+=f->bytes();
		}
		c_nntp_files_u(void):bytes(0),lines(0){}
		~c_nntp_files_u();
};

#define GETFILES_CASESENSITIVE	1
#define GETFILES_GETINCOMPLETE	2
#define GETFILES_NODUPEIDCHECK	4
#define GETFILES_NODUPEFILECHECK	128
#define GETFILES_DUPEFILEMARK		256

class c_nntp_server_info {
	public:
		ulong serverid;
		ulong high,low,num;
		void reset(void){high=0;low=ULONG_MAX;num=0;}
		c_nntp_server_info(ulong sid):serverid(sid){reset();}
		c_nntp_server_info(ulong sid,ulong hig,ulong lo,ulong nu):serverid(sid),high(hig),low(lo),num(nu){}
};
typedef map<ulong,c_nntp_server_info*> t_nntp_server_info;

class c_message_state : public c_refcounted<c_message_state>{
	public:
		string messageid;
		time_t date_added,date_removed;
		c_message_state(string mid,time_t da,time_t dr):messageid(mid),date_added(da),date_removed(dr){}
};

#ifdef HAVE_HASH_MAP_H
typedef hash_map<const char*, c_message_state::ptr, hash<const char*>, eqstr> t_message_state_list;
#else
typedef map<const char*, c_message_state::ptr, ltstr> t_message_state_list;
#endif

//hrm.
#define TIME_T_MAX INT_MAX
#define TIME_T_MAX1 (TIME_T_MAX-1)
#define TIME_T_DEAD TIME_T_MAX

class c_mid_info {
	public:
		string file;
		int changed;
		t_message_state_list states;
		int check(string mid) const {
			if (states.find(mid.c_str())!=states.end())
				return 1;
			return 0;
		}
		void insert(string mid){
			if (check(mid))return;
			changed=1;
			c_message_state::ptr s=new c_message_state(mid,time(NULL),TIME_T_MAX1);
			states.insert(t_message_state_list::value_type(s->messageid.c_str(),s));
			return;
		}
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
		int remove(string mid){
			t_message_state_list::iterator i=states.find(mid.c_str());
			if (i==states.end())
				return 0;
			(*i).second->date_removed=TIME_T_DEAD;
			changed=1;
			return 1;
		}
		void clear(void){
			if (!states.empty()){
				states.clear();
				changed=1;
			}
		}
		void set_delete(string mid){
			t_message_state_list::iterator i=states.find(mid.c_str());
			if (i!=states.end()){
				(*i).second->date_removed=time(NULL);
			}
		}
		void do_delete_fun(c_mid_info &rel_mid);
		int load(string path,bool merge=0,bool lock=1);
		int save(void);
		c_mid_info(string path);
		~c_mid_info();
};

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
		ulong flushlow(c_nntp_server_info *servinfo, ulong newlow, c_mid_info *midinfo);
		void getxrange(c_nntp_server_info *servinfo, ulong newlow, ulong newhigh, c_nrange *range) const;
		c_nntp_files_u* getfiles(const string &path, const string &temppath, c_nntp_files_u * fc,c_mid_info *midinfo,generic_pred *pred,int flags);
		c_nntp_cache(string path,c_group_info::ptr group,c_mid_info*midinfo);
		virtual ~c_nntp_cache();
};

#endif
