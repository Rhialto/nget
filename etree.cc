/*
    etree.* - handles expression trees..
    Copyright (C) 1999  Matthew Mueller <donut@azstarnet.com>

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
#include "etree.h"
#include <ctype.h>
#include "cache.h"
#include "myregex.h"
#include "misc.h"
#include "strreps.h"

template <class Op>
struct e_generic : public generic_pred {
	typedef Op op_type;
	typedef typename Op::arg1_type arg1_type;
	typedef typename Op::arg2_type arg2_type;
	arg2_type val2;
	const Op *O;
	int ofs;
//	void *val;

//	e_binder2nd(const arg2_type v):val2(v){};
	e_generic(const Op *aO,int aofs,arg2_type v){
		ofs=aofs;val2=v;O=aO;
	}
//	virtual ret_type operator()(const arg_type v) const {return (*O)(*((arg1_type*)(v+ofs)),val2);};
};
//typedef e_binary_function<ulong,ulong,bool> bin_fun_ulong;
template <class Type>
struct e_generic_type : public e_generic<e_binary_function<Type,Type,bool> > {
	e_generic_type(const op_type *aO,int aofs,arg2_type v):e_generic<op_type> (aO,aofs,v){};
	virtual ret_type operator()(const arg_type v) const {return (*O)(*((Type*)((ubyte*)v+ofs)),val2);};
};
typedef e_generic_type<ulong> e_generic_ulong;
typedef e_generic_type<int> e_generic_int;
typedef e_generic_type<time_t> e_generic_time_t;

//needs own type so it can free up the regexp when done
typedef e_binary_function<string&,c_regex_nosub*,bool> bin_fun_re;
struct e_generic_re : public e_generic<bin_fun_re> {
	e_generic_re(const bin_fun_re *aO,int aofs,arg2_type v):e_generic<bin_fun_re>(aO,aofs,v){};
	virtual ~e_generic_re(){delete val2;}
	virtual ret_type operator()(const arg_type v) const {return (*O)(*((string*)((ubyte*)v+ofs)),val2);};
};

//these cannot use generic_type, since they need to go into pointers and stuff
struct e_nntp_mid : public e_generic_re {
	e_nntp_mid(const bin_fun_re *aO,int aofs,arg2_type v):e_generic_re(aO, aofs, v){};
	virtual ret_type operator()(const arg_type f) const {
		if (f->parts.empty()) {
			string foobar("");
			return (*O)(foobar,val2);
		}
		return (*O)((*f->parts.begin()).second->messageid,val2);
	};
};

struct e_nntp_date : public e_generic<e_binary_function<time_t,time_t,bool> > {
	e_nntp_date(const op_type *aO,int aofs,arg2_type v):e_generic<e_binary_function<time_t,time_t,bool> >(aO,aofs,v){};
	virtual ret_type operator()(const arg_type f) const {
		if (f->parts.empty()) return (*O)(0,val2);
		return (*O)((*f->parts.begin()).second->date,val2);
		//return (*O)(((c_nntp_file*)v)->parts.begin(),val2);
	};
};
struct e_nntp_lines : public e_generic<e_binary_function<ulong,ulong,bool> > {
	e_nntp_lines(const op_type *aO,int aofs,arg2_type v):e_generic<e_binary_function<ulong,ulong,bool> >(aO,aofs,v){};
	virtual ret_type operator()(const arg_type f) const {
		return (*O)(f->lines(),val2);
	};
};
struct e_nntp_bytes : public e_generic<e_binary_function<ulong,ulong,bool> > {
	e_nntp_bytes(const op_type *aO,int aofs,arg2_type v):e_generic<e_binary_function<ulong,ulong,bool> >(aO,aofs,v){};
	virtual ret_type operator()(const arg_type f) const {
		return (*O)(f->bytes(),val2);
	};
};

/*struct e_nntp_banum : public e_generic<e_binary_function<ulong,ulong,bool> > {
	e_nntp_banum(const op_type *aO,int aofs,arg2_type v):e_generic<e_binary_function<ulong,ulong,bool> >(aO,aofs,v){};
	virtual ret_type operator()(const arg_type v) const {
		c_nntp_file*f=(c_nntp_file*)v;
		if (f->parts.empty()) return (*O)(0,val2);
		return (*O)((*f->parts.begin()).second->articlenum,val2);
		//return (*O)(((c_nntp_file*)v)->parts.begin(),val2);
	};
};*/

/*template <class Operation1>
inline e_generic<Operation1> *
egeneric(const Operation1 *op1, const int ofs,ubyte *v) {
	  return new e_generic<Operation1>(op1,ofs,(typename Operation1::arg2_type)v);
};*/

static bin_fun_re * e_str_;
static e_eq<string&,c_regex_nosub*> str_eq;
static e_ne<string&,c_regex_nosub*> str_ne;
#define MAKE_E_BIN_FUNCS(type) \
static e_binary_function<type,type,bool> * e_ ## type ## _;\
static e_gt<type,type> type ## _gt;	\
static e_ge<type,type> type ## _ge;	\
static e_lt<type,type> type ## _lt;	\
static e_le<type,type> type ## _le;	\
static e_eq<type,type> type ## _eq;	\
static e_ne<type,type> type ## _ne;
MAKE_E_BIN_FUNCS(ulong);
MAKE_E_BIN_FUNCS(time_t);
MAKE_E_BIN_FUNCS(int);
static e_binary_function<bool,bool,bool> *e_logic_;


#define E_STRING 0
#define E_ULONG 1
#define E_INT 2
#define E_TIME_T 3
typedef generic_pred*(pred_maker)(const void *op,const void*v);
static generic_pred* make_e_nntp_date(const void *op,const void*v) {
	return new e_nntp_date((e_binary_function<time_t,time_t,bool>*)op, 0, (time_t)v);
}
static generic_pred* make_e_nntp_lines(const void *op,const void*v) {
	return new e_nntp_lines((e_binary_function<ulong,ulong,bool>*)op, 0, (ulong)v);
}
static generic_pred* make_e_nntp_bytes(const void *op,const void*v) {
	return new e_nntp_bytes((e_binary_function<ulong,ulong,bool>*)op, 0, (ulong)v);
}
/*static generic_pred* make_e_nntp_banum(const void *op,const void*v) {
	return new e_nntp_banum((e_binary_function<ulong,ulong,bool>*)op, 0, (ulong)v);
}*/
static generic_pred* make_e_nntp_mid(const void *op,const void*v) {
	return new e_nntp_mid((bin_fun_re*)op, 0, (c_regex_nosub*)v);
}
struct {
	char * name;
	int type;
	int ofs;
	//generic_pred *specp;
	pred_maker *specp;//special pred_maker to use, else use generic_pred
}nntp_matches[]={
	{"subject",E_STRING,(ubyte*)(&((c_nntp_file*)NULL)->subject)-(ubyte*)NULL,NULL},
	{"author",E_STRING,(ubyte*)(&((c_nntp_file*)NULL)->author)-(ubyte*)NULL,NULL},
	//{"lines",E_ULONG,(ubyte*)(&((c_nntp_file*)NULL)->lines)-(ubyte*)NULL,NULL},
	//{"bytes",E_ULONG,(ubyte*)(&((c_nntp_file*)NULL)->bytes)-(ubyte*)NULL,NULL},
	{"lines",E_ULONG,0,make_e_nntp_lines},
	{"bytes",E_ULONG,0,make_e_nntp_bytes},
	{"req",E_INT,(ubyte*)(&((c_nntp_file*)NULL)->req)-(ubyte*)NULL,NULL},
	{"have",E_INT,(ubyte*)(&((c_nntp_file*)NULL)->have)-(ubyte*)NULL,NULL},
	{"date",E_TIME_T,0,make_e_nntp_date},
//	{"anum",E_ULONG,0,make_e_nntp_banum},
	{"messageid",E_STRING,0,make_e_nntp_mid},
	{"mid",E_STRING,0,make_e_nntp_mid}, //same as messageid
	{0,0,0,0}
};

generic_pred * make_pred(const char *optarg){
	list<string> e_parts;
	string curpart;
	//const char /**curstart=NULL,*/*cur;
	const char *cur;
	char quote=0;
	bool esc=0;
	for(cur=optarg;*cur;cur++){
		if (esc){
			switch (*cur){
				case '"':case '\'':
					break;
				default:
					curpart+='\\';//avoid having to double escape stuff other than quotes..
					break;
			}
			esc=0;
			curpart+=*cur;
			continue;
		}
		if (isspace(*cur))
			if(!quote){
				e_parts.push_back(curpart);
				//e_parts.push_back(string(curstart,cur-curstart));
				curpart="";
				//				curstart=NULL;
				continue;
			}
		//		if (curstart==NULL)curstart=cur;
		switch(*cur){
			case '\\':esc=1;continue;
			case '"':case '\'':
				if (quote==*cur) {quote=0;continue;}
				else if (!quote) {quote=*cur;continue;}
				//else drop through
			default:
				curpart+=*cur;
		}
	}
	if (esc || quote){printf("bad. %i %i\n",esc,quote);return NULL;}
	if (!curpart.empty())
		e_parts.push_back(curpart);
	string *x=NULL,*y=NULL;
	//								const char *x=NULL,*y=NULL;
	list<string>::iterator i=e_parts.begin();
	int match;
	generic_pred * p=NULL,*px=NULL,*py=NULL;
	for (;i!=e_parts.end();++i){
		if (!x){
			PDEBUG(DEBUG_MIN,"x %s",(*i).c_str());
			x=&(*i);
			if (px && py){
				if (x->compare("&&")==0 || x->compare("and")==0)
					e_logic_=new e_land<bool>;
				else if (x->compare("||")==0 || x->compare("or")==0)
					e_logic_=new e_lor<bool>;
				else {
					printf("error: bad op %s\n",x->c_str());
					return NULL;
				}
				p=ecompose2(e_logic_,px,py);
				py=NULL;px=p;
				x=NULL;
			}
		}else if (!y){
			PDEBUG(DEBUG_MIN,"y %s",(*i).c_str());
			y=&(*i);
		} else {
			PDEBUG(DEBUG_MIN,"z %s",(*i).c_str());
			for (match=0;nntp_matches[match].name;match++){
				if (strcasecmp(nntp_matches[match].name,x->c_str())==0)
					break;
			}
			if (nntp_matches[match].name){
				PDEBUG(DEBUG_MIN,"%s %i %i",nntp_matches[match].name,nntp_matches[match].type,nntp_matches[match].ofs);
				if (nntp_matches[match].type==E_STRING){
					//c_regex_nosub *reg=new c_regex_nosub(optarg,REG_EXTENDED + ((gflags&GETFILES_CASESENSITIVE)?0:REG_ICASE));
					c_regex_nosub *reg=NULL;
					try{
						reg=new c_regex_nosub(y->c_str(),REG_EXTENDED|REG_ICASE);
						if (!reg)
							throw ApplicationExFatal(Ex_INIT,"couldn't allocate regex");
					}catch (c_regex_error *regerr) {
						char buf[256];
						regerr->strerror(buf,256);
						int e=regerr->geterror();
						delete reg;//shouldn't be needed, but shouldn't hurt either
						delete regerr;
						throw ApplicationExFatal(Ex_INIT,"regex error %i:%s",e,buf);
					}
					if ((*i).compare("=~")==0 || (*i).compare("==")==0)
						e_str_=&str_eq;
					else if ((*i).compare("!~")==0 || (*i).compare("!=")==0)
						e_str_=&str_ne;
					else{
						printf("error: bad op %s\n",(*i).c_str());
						delete reg;
						return NULL;
					}

					//p=egeneric(e_str_,nntp_matches[match].ofs,(ubyte*)reg);
					if (nntp_matches[match].specp)
						p=nntp_matches[match].specp(e_str_,(void*)reg);
					else
						p=new e_generic_re(e_str_,nntp_matches[match].ofs,reg);
				}
				else if (nntp_matches[match].type==E_INT){
					if ((*i).compare(">")==0)	e_int_=&int_gt;
					else if ((*i).compare(">=")==0)	e_int_=&int_ge;
					else if ((*i).compare("<")==0)	e_int_=&int_lt;
					else if ((*i).compare("<=")==0)	e_int_=&int_le;
					else if ((*i).compare("==")==0)	e_int_=&int_eq;
					else if ((*i).compare("!=")==0)	e_int_=&int_ne;
					else{
						printf("error: bad op %s\n",(*i).c_str());
						return NULL;
					}
					//p=egeneric(e_int_,nntp_matches[match].ofs,(ubyte*)atoul(y->c_str()));
					int v=atoi(y->c_str());
					if (nntp_matches[match].specp)
						p=nntp_matches[match].specp(e_int_,(void*)v);
					else
						p=new e_generic_int(e_int_,nntp_matches[match].ofs,v);
				}
				else if (nntp_matches[match].type==E_ULONG){
					if ((*i).compare(">")==0)	e_ulong_=&ulong_gt;
					else if ((*i).compare(">=")==0)	e_ulong_=&ulong_ge;
					else if ((*i).compare("<")==0)	e_ulong_=&ulong_lt;
					else if ((*i).compare("<=")==0)	e_ulong_=&ulong_le;
					else if ((*i).compare("==")==0)	e_ulong_=&ulong_eq;
					else if ((*i).compare("!=")==0)	e_ulong_=&ulong_ne;
					else{
						printf("error: bad op %s\n",(*i).c_str());
						return NULL;
					}
					//p=egeneric(e_ulong_,nntp_matches[match].ofs,(ubyte*)atoul(y->c_str()));
					ulong v=atoul(y->c_str());
					if (nntp_matches[match].specp)
						p=nntp_matches[match].specp(e_ulong_,(void*)v);
					else
						p=new e_generic_ulong(e_ulong_,nntp_matches[match].ofs,v);
				}
				else if (nntp_matches[match].type==E_TIME_T){
					if ((*i).compare(">")==0)	e_time_t_=&time_t_gt;
					else if ((*i).compare(">=")==0)	e_time_t_=&time_t_ge;
					else if ((*i).compare("<")==0)	e_time_t_=&time_t_lt;
					else if ((*i).compare("<=")==0)	e_time_t_=&time_t_le;
					else if ((*i).compare("==")==0)	e_time_t_=&time_t_eq;
					else if ((*i).compare("!=")==0)	e_time_t_=&time_t_ne;
					else{
						printf("error: bad op %s\n",(*i).c_str());
						return NULL;
					}
					//p=egeneric(e_time_t_,nntp_matches[match].ofs,(ubyte*)atoul(y->c_str()));
					//time_t t=atol(y->c_str());
					time_t t=decode_textdate(y->c_str());
					PDEBUG(DEBUG_MIN,"t=%li",t);
					if (nntp_matches[match].specp)
						p=nntp_matches[match].specp(e_time_t_,(void*)t);
					else
						p=new e_generic_time_t(e_time_t_,nntp_matches[match].ofs,t);
				}
				if (px)
					py=p;
				else
					px=p;
				//										if (strncasecmp(x->c_str(),"subject",x->size())==0)
			}else{
				printf("error: no match type %s\n",x->c_str());
				return NULL;
			}
			x=y=NULL;
		}
	}
	if (py || x || y){
		printf("error: unfinished\n");
		return NULL;
	}
	return px;
}
								//nntp_pred *p=ecompose2(new e_land<bool>,
								//		new e_binder2nd<e_nntp_file_lines<e_ge<ulong> > >(linelimit),
								//		new e_binder2nd_p<e_nntp_file_subject<e_eq<string,c_regex_nosub*> > >(reg));
