/*
    myregex.* - regex() wrapper class
    Copyright (C) 1999-2003  Matthew Mueller <donut@azstarnet.com>

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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include "myregex.h"


static string regex_match_word_beginning_str;
static string regex_match_word_end_str;
static bool regex_initialized=0;
static string regex_test_op(const char **ops, const char *pat, const char *match1,  const char *match2){
	char buf[100];
	for (; *ops; ops++) {
		sprintf(buf, pat, *ops);
		try {
			c_regex_nosub rx(buf, REG_EXTENDED);
			if (match1==rx && match2==rx)
				return *ops;
		} catch (RegexEx &e) {
			//ignore any errors here, since they just would just mean that op didn't work, so go on and try the next one.
		}
	}
	return "";
}
static void regex_init(void) {
	const char *wbeg_ops[]={"\\<", "\\b", "(^|[^A-Za-z0-9])", NULL};
	regex_match_word_beginning_str = regex_test_op(wbeg_ops, "%sfoo", "a fooa", "fooa");

	const char *wend_ops[]={"\\>", "\\b", "($|[^A-Za-z0-9])", NULL};
	regex_match_word_end_str = regex_test_op(wend_ops, "foo%s", "afoo a", "afoo");

	PDEBUG(DEBUG_MIN,"regex_init regex_match_word_beginning:%s regex_match_word_end:%s",regex_match_word_beginning_str.c_str(), regex_match_word_end_str.c_str());
	regex_initialized=true;
}
const string& regex_match_word_beginning(void) {
	if (!regex_initialized) regex_init();
	return regex_match_word_beginning_str;
}
const string& regex_match_word_end(void){
	if (!regex_initialized) regex_init();
	return regex_match_word_end_str;
}

void regex_escape_string(const string &s, string &buf){
	for (string::const_iterator cp=s.begin(); cp!=s.end(); ++cp) {
		if (strchr("{}()|[]\\.+*^$",*cp))
			buf+='\\';//escape special chars
		buf+=*cp;
	}
}

c_regex_base::c_regex_base(const char * pattern,int cflags){
	if (!pattern)
		pattern="";
	int re_err;
	if ((re_err=regcomp(&regex,pattern,cflags))){
		char buf[256];
		regerror(re_err,&regex,buf,256);
		regfree(&regex);
		throw RegexEx(Ex_INIT, "regcomp: %s", buf);
	}
}
c_regex_base::~c_regex_base(){
	regfree(&regex);
}

c_regex_nosub::c_regex_nosub(const char * pattern,int cflags):c_regex_base(pattern,cflags|REG_NOSUB){
}

int c_regex_subs::doregex(regex_t *regex,const char *str){
	if ((re_err=regexec(regex,str,nregmatch,regmatch,0)))
		return re_err;
	freesub();
	rnsub=regex->re_nsub;
	assert(nregmatch>=rnsub);
	if (rnsub>=0 && nregmatch){
		int i;
		//rsub=new (string*)[rnsub+1];
		rsub=new string[rnsub+1];
		for (i=0;i<=rnsub;i++){
			assert(regmatch[i].rm_eo>=regmatch[i].rm_so);
//			printf("doregex: i=%i/%i so=%i eo=%i\n",i,rnsub,regmatch[i].rm_so, regmatch[i].rm_eo);
//
			if (regmatch[i].rm_so>=0)
				rsub[i].assign(str+regmatch[i].rm_so,regmatch[i].rm_eo-regmatch[i].rm_so);
		}
	}
	return 0;
}
void c_regex_subs::setnregmatch(int num){
	if (nregmatch!=num){
		if (regmatch)
			delete [] regmatch;
		nregmatch=num;
		if (nregmatch>0)
			regmatch=new regmatch_t[nregmatch+1];
		else
			regmatch=NULL;
	}
}
c_regex_subs::c_regex_subs(int nregm):nregmatch(-1){
	regmatch=NULL;
	setnregmatch(nregm);
	rsub=NULL;
	rnsub=-1;
}
c_regex_subs::~c_regex_subs(){
	freesub();
	if (regmatch){
		delete [] regmatch;
//		free(regmatch);
	}
}
void c_regex_subs::freesub(void){
	if (rsub){
//		for (int i=0;i<=rnsub;i++){
//			delete rsub[i];
//		}
		delete [] rsub;
	}
	rsub=NULL;
	rnsub=-1;
}

int c_regex_r::match(const char *str,c_regex_subs*subs){
	subs->setnregmatch(nregmatch);
	return subs->doregex(&regex,str);
}
c_regex_subs * c_regex_r::match(const char *str){
	c_regex_subs *subs=new c_regex_subs(nregmatch);
//	subs->doregex(regex,str,nregmatch);
	match(str,subs);
	return subs;
}
c_regex_r::c_regex_r(const char * pattern,int cflags):c_regex_base(pattern,cflags){
	if (cflags&REG_NOSUB){
		nregmatch=0;
	}else{
		nregmatch=1;
		for (;*pattern!=0;pattern++){
			if (*pattern=='(')//this could give more regmatches than we need, considering 
				nregmatch++;//escaping and such, but its a lot better than a static number
		}
//pcre needs more matching space for something?
#ifdef HAVE_PKG_pcre
		//nregmatch=nregmatch*15/10;
		nregmatch=nregmatch*2;
#endif
	}
}
