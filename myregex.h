/*
    myregex.* - regex() wrapper class
    Copyright (C) 1999-2000  Matthew Mueller <donut@azstarnet.com>

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
#ifndef _MYREGEX_H_
#define _MYREGEX_H_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>

#ifdef HAVE_PKG_pcre
#include <pcreposix.h>
#else
#include <regex.h>
#endif

#ifndef REG_EXTENDED
#define REG_EXTENDED (0)
#endif
#ifndef REG_NOSUB
#define REG_NOSUB (0)
#endif

#include <assert.h>
//#include "mystring.h"
#include <string>
#include <stdio.h>

#include "log.h"
DEFINE_EX_SUBCLASS(RegexEx, ApplicationExFatal, true);

const string& regex_match_word_beginning(void);
const string& regex_match_word_end(void);

//  use strerror for a textual description of problem.  same as regerror(), but without the first arg(s).

//throws a RegexEx if there is an error during the constructor
class c_regex_base{
	protected:
//icky, but pcreposix doesn't have regexec's args as const.  Blah.
#ifdef HAVE_PKG_pcre
		mutable
#endif
			regex_t regex;
	public:
		c_regex_base(const char * pattern,int cflags);
		~c_regex_base();
		size_t strerror(int re_err,char *buf,size_t bufsize){return regerror(re_err,&regex,buf,bufsize);}
};

//thread safe
class c_regex_nosub : public c_regex_base{
  public:
	int match(const char *str)const{return regexec(&regex,str,0,NULL,0);}
	c_regex_nosub(const char * pattern,int cflags=REG_EXTENDED);
//	~c_regex_nosub();
};
inline bool operator == (const string &a,const c_regex_nosub &r){return r.match(a.c_str())==0;}
inline bool operator == (const string &a,const c_regex_nosub *r){return r->match(a.c_str())==0;}
inline bool operator != (const string &a,const c_regex_nosub &r){return r.match(a.c_str())!=0;}
inline bool operator != (const string &a,const c_regex_nosub *r){return r->match(a.c_str())!=0;}
inline bool operator == (const char *a,const c_regex_nosub &r){return r.match(a)==0;}

//thread safe version
class c_regex_subs {
	private:
		regmatch_t *regmatch;
		int rnsub;
		//string **rsub;
		string *rsub;
		int nregmatch;
	public:
		void setnregmatch(int num);
		int doregex(regex_t *regex,const char *str);
		int re_err;
		void freesub(void);
		int nsub(void) const {return rnsub;}
		const string &operator [](int i) const {return rsub[i];}
		const string &sub(int i) const {return rsub[i];}
		const char * subs(int i) const {return rsub[i].c_str();}
		const int sublen(int i) const {return regmatch[i].rm_eo-regmatch[i].rm_so;}
		const int subeo(int i) const {return regmatch[i].rm_eo;}
		c_regex_subs(int nregmatch=0);
		~c_regex_subs();
};
class c_regex_r : public c_regex_base{
	protected:
		int nregmatch;
	public:
		int match(const char *str,c_regex_subs* subs);
		c_regex_subs* match(const char *str);
		c_regex_r(const char * pattern,int cflags=REG_EXTENDED);
	//	~c_regex_r();
};

//non thread safe (obviously, if two threads did a match at the same time, your subs would be screwed)
class c_regex : protected c_regex_r, public c_regex_subs{
	public:
		int match(const char *str){return c_regex_r::match(str,this);}
		c_regex(const char * pattern,int cflags=REG_EXTENDED):c_regex_r(pattern,cflags){setnregmatch(c_regex_r::nregmatch);}
//		~c_regex();
};
inline bool operator == (const string &a,c_regex &r){return !r.match(a.c_str());}
inline bool operator == (const string &a,c_regex *r){return !r->match(a.c_str());}
inline bool operator != (const string &a,c_regex *r){return r->match(a.c_str());}
inline bool operator == (const char *a,c_regex &r){return !r.match(a);}

#endif
