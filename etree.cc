/*
    etree.* - handles expression trees..
    Copyright (C) 1999,2002  Matthew Mueller <donut@azstarnet.com>

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
#include "cache.h"
#include "grouplist.h"
#include "myregex.h"
#include "misc.h"
#include "strreps.h"
#include "getter.h"
#include <stack>

#define MAKE_BINARY_OP(name,op) template <class T,class T2=T>			\
struct Op_ ## name {			\
	bool operator()(const T v1,const T2 v2) const {return v1 op v2;}	\
};
MAKE_BINARY_OP(gt,>);
MAKE_BINARY_OP(ge,>=);
MAKE_BINARY_OP(lt,<);
MAKE_BINARY_OP(le,<=);
MAKE_BINARY_OP(eq,==);
MAKE_BINARY_OP(ne,!=);

//use real operators here (rather than a predComparison with Op template) to take advantage of short-circuit evaluation.
template <class ClassType>
class predAnd : public pred<ClassType> {
	private:
		pred<ClassType> *p1, *p2;
	public:
		predAnd(pred<ClassType> *n1, pred<ClassType> *n2):p1(n1), p2(n2) { }
		virtual ~predAnd() {delete p1; delete p2;}
		virtual bool operator()(ClassType* f) const {
			return (*p1)(f) && (*p2)(f);
		}
};
template <class ClassType>
class predOr : public pred<ClassType> {
	private:
		pred<ClassType> *p1, *p2;
	public:
		predOr(pred<ClassType> *n1, pred<ClassType> *n2):p1(n1), p2(n2) { }
		virtual ~predOr() {delete p1; delete p2;}
		virtual bool operator()(ClassType* f) const {
			return (*p1)(f) || (*p2)(f);
		}
};

template <template <class A, class B> class Op, template <class D, class E> class Getter, class T, class ClassType, class T2=T>
class Comparison : public pred<ClassType> {
	private:
		typedef Getter<T, ClassType> getterT;
		Op<typename getterT::T,T2> op;
		getterT getter;
		T2 v;
	public:
		Comparison(typename getterT::init_t gette, const T2 v2):getter(gette), v(v2){}
		virtual bool operator()(ClassType* f) const {
			return op(getter(f), v);
		}
};
template <template <class A, class B> class Op, class ClassType, class RetType>
pred<ClassType> *new_comparison(RetType (ClassType::*member), RetType v){
	return new Comparison<Op, MemGetter, RetType, ClassType>(member, v);
}
template <template <class A, class B> class Op, class ClassType, class RetType>
pred<ClassType> *new_comparison(RetType (ClassType::*memberf)(void), RetType v){
	return new Comparison<Op, MemfuncGetter, RetType, ClassType>(memberf, v);
}
template <class ClassType, class getterT, class T2>
pred<ClassType> *comparison(const string &opstr, getterT get, T2 v) {
	if      (opstr.compare("==")==0) return new_comparison<Op_eq,ClassType>(get, v);
	else if (opstr.compare("!=")==0) return new_comparison<Op_ne,ClassType>(get, v);
	else if (opstr.compare("<")==0)  return new_comparison<Op_lt,ClassType>(get, v);
	else if (opstr.compare("<=")==0) return new_comparison<Op_le,ClassType>(get, v);
	else if (opstr.compare(">")==0)  return new_comparison<Op_gt,ClassType>(get, v);
	else if (opstr.compare(">=")==0) return new_comparison<Op_ge,ClassType>(get, v);
	throw UserExFatal(Ex_INIT, "invalid op %s for comparison", opstr.c_str());
}

//template <template <class A, class B> class Op, template <class D, class E, class F> class Getter, class getterT, class T>
template <template <class A, class B> class Op, template <class D, class E> class Getter, class T, class ClassType>
class Comparison_re : public pred<ClassType> {
	private:
		typedef Getter<T, ClassType> getterT;
		Op<typename getterT::T,const c_regex_nosub&> op;
		getterT getter;
		c_regex_nosub re;
	public:
		Comparison_re(typename getterT::init_t gette, const char *pattern, int flags):getter(gette), re(pattern, flags){}
		virtual bool operator()(ClassType* f) const {
			return op(getter(f), re);
		}
};
template <template <class A, class B> class Op, class ClassType, class RetType>
pred<ClassType> *new_comparison_re(RetType (ClassType::*member), const char *pattern, int flags){
	return new Comparison_re<Op, MemGetter, RetType, ClassType>(member, pattern, flags);
}
template <template <class A, class B> class Op, class ClassType, class RetType>
pred<ClassType> *new_comparison_re(RetType (ClassType::*memberf)(void), const char *pattern, int flags){
	return new Comparison_re<Op, MemfuncGetter, RetType, ClassType>(memberf, pattern, flags);
}
template <class ClassType, class getterT>
pred<ClassType> *comparison_re(const string &opstr, getterT get, const char *pattern, int flags) {
	//typedef const string &T;
	if (opstr.compare("==")==0 || opstr.compare("=~")==0)
		return new_comparison_re<Op_eq,ClassType>(get, pattern, flags);
		//return new Comparison_re<Op_eq,Getter,const getterT,T>(get, pattern, flags);
	else if (opstr.compare("!=")==0 || opstr.compare("!~")==0)
		return new_comparison_re<Op_ne,ClassType>(get, pattern, flags);
		//return new Comparison_re<Op_ne,Getter,const getterT,T>(get, pattern, flags);
	throw UserExFatal(Ex_INIT, "invalid op %s for comparison_re", opstr.c_str());
}

string invert_op(string o) {
	if (o == "<") return ">";
	else if (o == "<=") return ">=";
	else if (o == ">")  return "<";
	else if (o == ">=") return "<=";
	else return o;
}


template <class ClassType, pred<ClassType> *(*comparison_maker)(const string &i, const string *x, const string *y, int re_flags)>
pred<ClassType> * make_pred(const arglist_t &e_parts, int gflags) {
	const string *x=NULL,*y=NULL;
	arglist_t::const_iterator i=e_parts.begin();
	int re_flags = REG_EXTENDED | ((gflags&GETFILES_CASESENSITIVE)?0:REG_ICASE);
	pred<ClassType> * p=NULL;
	stack<pred<ClassType> *> pstack;
	for (;i!=e_parts.end();++i){
		if (!x){
			PDEBUG(DEBUG_MIN,"x %s",(*i).c_str());
			x=&(*i);
			if (*x=="&&" || *x=="and" || *x=="||" || *x=="or"){
				pred<ClassType> *py=pstack.top(); pstack.pop();
				pred<ClassType> *px=pstack.top(); pstack.pop();
				if (x->compare("&&")==0 || x->compare("and")==0)
					p = new predAnd<ClassType>(px, py);
				else
					p = new predOr<ClassType>(px, py);
				pstack.push(p);
				x=NULL;
			}
		}else if (!y){
			PDEBUG(DEBUG_MIN,"y %s",(*i).c_str());
			y=&(*i);
		} else {
			PDEBUG(DEBUG_MIN,"z %s",(*i).c_str());
			p = comparison_maker((*i), x, y, re_flags);
			if (!p)
				throw UserExFatal(Ex_INIT, "no match type %s", x->c_str());
			pstack.push(p);
			x=y=NULL;
		}
	}
	if (pstack.size()>1 || x || y){
		throw UserExFatal(Ex_INIT, "unfinished expression");
	}
	if (pstack.empty())
		throw UserExFatal(Ex_INIT, "empty expression");
	return pstack.top();
}

inline bool operator == (const t_references &a, const c_regex_nosub &r) {
	for (t_references::const_iterator i = a.begin(); i!=a.end(); ++i)
		if (*i == r)
			return true;
	return false;
}
inline bool operator != (const t_references &a, const c_regex_nosub &r) {
	return (!(a==r));
}
nntp_file_pred *nntp_file_comparison_maker(const string &i, const string *x, const string *y, int re_flags) {
	const char *n = x->c_str();
	if (strcasecmp(n, "subject")==0)
		return comparison_re<const c_nntp_file>(i, &c_nntp_file::subject, y->c_str(), re_flags);
	else if (strcasecmp(n, "author")==0)
		return comparison_re<const c_nntp_file>(i, &c_nntp_file::author, y->c_str(), re_flags);
	else if (strcasecmp(n, "mid")==0 || strcasecmp(n, "messageid")==0)
		return comparison_re<const c_nntp_file>(i, &c_nntp_file::bamid, y->c_str(), re_flags);
	else if (strcasecmp(n, "bytes")==0)
		return comparison<const c_nntp_file>(i, &c_nntp_file::bytes, atoul(y->c_str()));
	else if (strcasecmp(n, "lines")==0)
		return comparison<const c_nntp_file>(i, &c_nntp_file::lines, atoul(y->c_str()));
	else if (strcasecmp(n, "req")==0)
		return comparison<const c_nntp_file>(i, &c_nntp_file::req, atoi(y->c_str()));
	else if (strcasecmp(n, "have")==0)
		return comparison<const c_nntp_file>(i, &c_nntp_file::have, atoi(y->c_str()));
	else if (strcasecmp(n, "date")==0)
		return comparison<const c_nntp_file>(i, &c_nntp_file::badate, decode_textdate(y->c_str()));
	else if (strcasecmp(n, "age")==0)
		//rather than taking a relative age and converting each date into a relative age and then comparing, decode_textage returns an absolute date (time_t) which simplifies comparisons, but to get intuitive usage we have to invert <,> operators.
		return comparison<const c_nntp_file>(invert_op(i), &c_nntp_file::badate, decode_textage(y->c_str()));
	else if (strcasecmp(n, "references")==0)
		return comparison_re<const c_nntp_file>(i, &c_nntp_file::references, y->c_str(), re_flags);
	else
		return NULL;
}

nntp_file_pred * make_nntpfile_pred(const arglist_t &e_parts, int gflags) {
	return make_pred<const c_nntp_file, nntp_file_comparison_maker>(e_parts, gflags);
}
nntp_file_pred * make_nntpfile_pred(const char *optarg, int gflags) {
	arglist_t e_parts;
	parseargs(e_parts, optarg);
	return make_nntpfile_pred(e_parts, gflags);
}

//Somewhat hacky way to allow desc keyword.  Works quite nicely, but this method won't be usable if more than a single attribute of the class needs to be tested.
inline bool operator == (const t_server_group_description_map &m, const c_regex_nosub &r) {
	for (t_server_group_description_map::const_iterator i = m.begin(); i!=m.end(); ++i)
		if (i->second->description == r)
			return true;
	return false;
}
inline bool operator != (const t_server_group_description_map &m, const c_regex_nosub &r) {
	return (!(m==r));
}
nntp_grouplist_pred *grouplist_comparison_maker(const string &i, const string *x, const string *y, int re_flags) {
	const char *n = x->c_str();
	if (strcasecmp(n, "group")==0)
		return comparison_re<const c_group_availability>(i, &c_group_availability::groupname, y->c_str(), re_flags);
	else if (strcasecmp(n, "desc")==0)
		return comparison_re<const c_group_availability>(i, &c_group_availability::servergroups, y->c_str(), re_flags);
	else
		return NULL;
}

nntp_grouplist_pred * make_grouplist_pred(const arglist_t &e_parts, int gflags) {
	return make_pred<const c_group_availability, grouplist_comparison_maker>(e_parts, gflags);
}
nntp_grouplist_pred * make_grouplist_pred(const char *optarg, int gflags) {
	arglist_t e_parts;
	parseargs(e_parts, optarg);
	return make_grouplist_pred(e_parts, gflags);
}

