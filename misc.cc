/*
    misc.* - misc functions
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "misc.h"
#include "strreps.h"
#include "log.h"
#include "path.h"
#include "file.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "myregex.h"

#include <string>
#include "_sstream.h"
#include <iomanip>
#include "_fileconf.h"

#ifndef HAVE_TIMEGM
time_t timegm (const struct tm *gmtimein) {
	/* The timegm manpage suggests a strategy of setting the TZ env var to ""
	 * and then running tzset(), mktime() and then resetting the TZ var to its
	 * previous value, but unfortunatly it doesn't seem to work on all arches.
	 * So rather than try to figure out when it does we'll use this routine
	 * by Yitzchak Scott-Thoennes that should work on all arches.
	 * (found at http://ais.gmd.de/~veit/os2/mailinglist3/0863.html)
	 */
	struct tm tm;
	time_t t, t2;

	tm = *gmtimein; /* make a local copy to fiddle with */
	tm.tm_isdst = 0; /* treat it as standard time */

	t2 = t = mktime(&tm); /* calculate the time as a local time */

	tm = *gmtime(&t2); /* now calculate the difference between */
	tm.tm_isdst = 0; /* gm and local time */
	t2 = mktime(&tm);

	t += t - t2; /* and adjust our answer by that difference */
	return t; 	
}
#endif

bool direxists(const char *p) {
	bool ret=false;
	char *oldp;
	goodgetcwd(&oldp);
	if (chdir(p)==0)
		ret=true;
	if (chdir(oldp)) {
		free(oldp);
		throw ApplicationExFatal(Ex_INIT, "could not return to oldp: %s (%s)",oldp,strerror(errno));
	}
	free(oldp);
	return ret;
}
int fexists(const char * f){
	struct stat statbuf;
	return (!stat(f,&statbuf));
}
string fcheckpath(const char *fn, string path){
	if (!is_abspath(fn)) {
		string pfn = path_join(path,fn);
		if (fexists(pfn.c_str()))
			return pfn;
	}
	return fn;
}

int testmkdir(const char * dir,int mode){
	struct stat statbuf;
	if (stat(dir,&statbuf)){
		return (mkdir(dir,mode));
	} else if (!S_ISDIR(statbuf.st_mode)){
		return ENOTDIR;
	}
	return 0;
}
char *goodgetcwd(char **p){
	/*int t=-1;
	*p=NULL;//first we try to take advantage of the GNU extension to allocate it for us.
	while (!(*p=getcwd(*p,t))) {
		t+=100;
		if (!(*p=(char*)realloc(*p,t))){
			PERROR("realloc error %s",strerror(errno));exit(1);
		}
	}*/
	int t=0;
	*p=NULL;//ack, that extension seems to like segfaulting when you free() its return.. blah.
	do {
		t+=48;
		if (!(*p=(char*)realloc(*p,t))){
			PERROR("goodgetcwd: realloc error %s (size=%i)",strerror(errno), t);exit(128);
		}
	}while(!(*p=getcwd(*p,t)));
	PDEBUG(DEBUG_MED,"goodgetcwd %i %s(%p-%p)",t,*p,p,*p);
	return *p;
}


size_t tconv(char * timestr, int max, time_t *curtime,const char * formatstr, int local) {
//	static char timestr[80];
	struct tm *time_now;
	if (local)
		time_now = localtime(curtime);
	else
		time_now = gmtime(curtime);
	return strftime(timestr,max,formatstr,time_now);
//	return timestr;
}

template <class int_type>
string durationstr(int_type duration){
	int_type s = duration%60, m = duration/60%60, h = duration/60/60;
	ostringstream oss; 
	if (h)
		oss << h << 'h';
	if (h || m)
		oss << m << 'm';
	oss << s << 's';
	return oss.str();
}
#if (SIZEOF_LONG != SIZEOF_INT_FAST64_T)
template string durationstr(int_fast64_t);
#endif
template string durationstr(ulong);
template string durationstr(long);

char *text_month[13]={"Jan", "Feb", "Mar", "Apr",
	"May", "Jun", "Jul", "Aug",
	"Sep", "Oct", "Nov", "Dec"
};

int decode_textmonth(const char * buf){
	for (int i=0;i<12;i++){
		if (!strncasecmp(text_month[i],buf,3))
			return i;
	}
	return -1;
}
int decode_texttz(const char * buf){
	int i=0;
	if (*buf=='-' || *buf=='+'){
		i=atoi(buf+1);
		i=((i/100)*60+(i%100))*60;
		if (*buf=='-')
			return -i;
	}
	return i;
}

//Tue, 25 May 1999 06:23:23 GMT
//21 Jun 99 01:58:12


//Last-modified: Friday, 13-Nov-98 20:41:28 GMT
//012345678901234567890123456789
//Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
//23 May 1999 22:46:41 GMT ; no textual day
//25 May 99 01:01:48 +0100 ; 2 digit year
//21 Jun 99 01:58:12 ; no timezone
//Mon, 24 May 99 11:53:47 GMT ; 2 digit year
//3 Jun 1999 12:35:14 -0500 ; non padded day. blah.
//Tue, 1 Jun 1999 20:36:29 +0100 ; blah again
//Sun, 23 May 1999 19:34:35 -0500 ; ack, timezone
//12 July 1999 01:23:05 GMT // full length month
//Sun, 15 Aug 1999 19:56 +0100 (BST) // no seconds

//Sunday,
// 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
//Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format (note, this is used by ls -l --full-time)
//Jan  4 17:11 			;ls -l format, 17:11 may be replaced by " 1998"

//easy format 06/11/94[ 23:34[:23][ -300]]
//c_regex rfc1123("^[A-Za-z, ]*([0-9]{1,2}) (...) ([0-9]{2,4}) ([0-9]{1,2}):([0-9]{2}):([0-9]{2}) (.*)$"),
//	rfc850("^[A-Za-z, ]*([0-9]{1,2})-(...)-([0-9]{2,4}) ([0-9]{1,2}):([0-9]{2}):([0-9]{2}) (.*)$"),
//#define TIME_REG "([0-9]{1,2}):([0-9]{2}):([0-9]{2})"

//allows for optional seconds
#define TIME_REG2 "([0-9]{1,2}):([0-9]{2})(:([0-9]{2}))?"
c_regex_r xrfc("^[A-Za-z, ]*([0-9]{1,2})[- ](.{3,9})[- ]([0-9]{2,4}) "TIME_REG2" *(.*)$"),
	xasctime("^[A-Za-z,]* *(...) +([0-9]{1,2}) "TIME_REG2" ([0-9]{2,4}) *(.*)$"),
	xlsl("^(...) +([0-9]{1,2}) +([0-9:]{4,5})$"),
	xeasy("^([0-9]{1,4})[-/]([0-9]{1,2})[-/]([0-9]{2,4})( *"TIME_REG2" *(.*))?$"),
	xiso("^([0-9]{4})-?([0-9]{2})-?([0-9]{2})([T ]?([0-9]{1,2})(:?([0-9]{2})(:?([0-9]{2}))?)?)? *([-+A-Z].*)?$");
c_regex_nosub xtime_t("^[0-9]+$")//seconds since 1970 (always gmt)
		;
time_t decode_textdate(const char * cbuf, bool local){
	struct tm tblock;
	memset(&tblock,0,sizeof(struct tm));
	char *tdt=NULL;
	int td_tz=local?0x7FFFFFFF:0;
	int yearlen=0;
	c_regex_subs rsubs;
	if (!xrfc.match(cbuf,&rsubs)){
		tdt="xrfc*-date";
		tblock.tm_mday=atoi(rsubs.subs(1));
		tblock.tm_mon=decode_textmonth(rsubs.subs(2));
		tblock.tm_year=atoi(rsubs.subs(3));
		yearlen=rsubs.sublen(3);
		tblock.tm_hour=atoi(rsubs.subs(4));
		tblock.tm_min=atoi(rsubs.subs(5));
		if(rsubs.sublen(6)>0)
			tblock.tm_sec=atoi(rsubs.subs(7));
		if(rsubs.sublen(8)>0)
			td_tz=decode_texttz(rsubs.subs(8));
	}else if (!xiso.match(cbuf,&rsubs)){
		tdt="iso";
		yearlen=rsubs.sublen(1);
		tblock.tm_year=atoi(rsubs.subs(1));
		tblock.tm_mon=atoi(rsubs.subs(2))-1;
		tblock.tm_mday=atoi(rsubs.subs(3));
		if(rsubs.sublen(4)>0){
			tblock.tm_hour=atoi(rsubs.subs(5));
			if(rsubs.sublen(6)>0){
				tblock.tm_min=atoi(rsubs.subs(7));
				if(rsubs.sublen(8)>0){
					tblock.tm_sec=atoi(rsubs.subs(9));
				}
			}
		}
		if(rsubs.sublen(10)>0)
			td_tz=decode_texttz(rsubs.subs(10));
	}else if (!xasctime.match(cbuf,&rsubs)){
		tdt="asctime-date";
		tblock.tm_mon=decode_textmonth(rsubs.subs(1));
		tblock.tm_mday=atoi(rsubs.subs(2));
		tblock.tm_hour=atoi(rsubs.subs(3));
		tblock.tm_min=atoi(rsubs.subs(4));
		if(rsubs.sublen(5)>0)
			tblock.tm_sec=atoi(rsubs.subs(6));
		tblock.tm_year=atoi(rsubs.subs(7));
		yearlen=rsubs.sublen(7);
	}else if (!xlsl.match(cbuf,&rsubs)){
		tdt="ls-l-date";
		tblock.tm_mon=decode_textmonth(rsubs.subs(1));
		tblock.tm_mday=atoi(rsubs.subs(2));
		if (rsubs.subs(3)[2]==':'){
			time_t curtime;
			time(&curtime);
			struct tm *lt = localtime(&curtime);

			tblock.tm_hour=atoi(rsubs.subs(3));
			tblock.tm_min=atoi(rsubs.subs(3)+3);

			if (lt->tm_mon>=tblock.tm_mon)
				tblock.tm_year=lt->tm_year;
			else
				tblock.tm_year=lt->tm_year-1;
		}else{
			yearlen=rsubs.sublen(3);
			tblock.tm_year=atoi(rsubs.subs(3));
		}
	}else if (!xeasy.match(cbuf,&rsubs)){
		tdt="easy-date";
		int a=atoi(rsubs.subs(1)),b=atoi(rsubs.subs(2)),c=atoi(rsubs.subs(3));
		if((rsubs.sublen(1)>2 || a>12 || a<=0) && (b>=1 && b<=12 && c>=1 && c<=31)){
			//year/mon/day format...
			tblock.tm_mon=b-1;
			tblock.tm_mday=c;
			tblock.tm_year=a;
			yearlen=rsubs.sublen(1);
		}else{
			//mon/day/year format...
			tblock.tm_mon=a-1;
			tblock.tm_mday=b;
			tblock.tm_year=c;
			yearlen=rsubs.sublen(3);
		}
		if(rsubs.sublen(4)>0){
			tblock.tm_hour=atoi(rsubs.subs(5));
			tblock.tm_min=atoi(rsubs.subs(6));
			if(rsubs.sublen(7)>0){
				tblock.tm_sec=atoi(rsubs.subs(8));
			}
			if(rsubs.sublen(9)>0)
				td_tz=decode_texttz(rsubs.subs(9));
		}
	}else if (!xtime_t.match(cbuf)){
		tdt="time_t-date";
		return atol(cbuf);
	}
	if(yearlen>=4)
		tblock.tm_year-=1900;
	else if(yearlen==2 && tblock.tm_year<70)
		tblock.tm_year+=100;//assume anything before (19)70 is 20xx
	if(!tdt){
		PERROR("decode_textdate: unknown %s",cbuf);
		return 0;
	}else
		PDEBUG(DEBUG_ALL,"decode_textdate: %s %i %i %i %i %i %i %i",tdt,tblock.tm_year,tblock.tm_mon,tblock.tm_mday,tblock.tm_hour,tblock.tm_min,tblock.tm_sec,td_tz);
	if (local && td_tz==0x7FFFFFFF){//if local=1 and time string didn't contain a timezone, just use mktime directly.
		tblock.tm_isdst = -1;
		return mktime(&tblock);
	}else
		return timegm(&tblock)-td_tz;
}

c_regex_r xduration("^ *([0-9]+ *ye?a?r?s?)? *([0-9]+ *mon?t?h?s?)? *([0-9]+ *we?e?k?s?)? *([0-9]+ *da?y?s?)? *([0-9]+ *ho?u?r?s?)? *([0-9]+ *mi?n?u?t?e?s?)? *([0-9]+ *se?c?o?n?d?s?)? *$", REG_ICASE|REG_EXTENDED);
time_t decode_textage(const char *cbuf) {
	time_t now=time(NULL);
	struct tm tblock = *localtime(&now);
	c_regex_subs rsubs;
	if (!xduration.match(cbuf,&rsubs)){
//		if(rsubs.sublen(1)>0)
//			age+=atol(rsubs.subs(1))*31556952; //365.2425*24*60*60
		if(rsubs.sublen(1)>0)
			tblock.tm_year-=atol(rsubs.subs(1));
		if(rsubs.sublen(2)>0)
			tblock.tm_mon-=atol(rsubs.subs(2));
		if(rsubs.sublen(3)>0)
			tblock.tm_mday-=atol(rsubs.subs(3))*7;
		if(rsubs.sublen(4)>0)
			tblock.tm_mday-=atol(rsubs.subs(4));
		if(rsubs.sublen(5)>0)
			tblock.tm_hour-=atol(rsubs.subs(5));
		if(rsubs.sublen(6)>0)
			tblock.tm_min-=atol(rsubs.subs(6));
		if(rsubs.sublen(7)>0)
			tblock.tm_sec-=atol(rsubs.subs(7));
	}else {
		PERROR("decode_textage: unknown %s",cbuf);
		return 0;
	}
	//return now - mktime(&tblock);
	return mktime(&tblock);
}

int filecompare(const char *old_fn,const char *nfn){
	c_file_fd old_f(old_fn, O_RDONLY);
	c_file_fd new_f(nfn, O_RDONLY);
	char	old_buf[4096], new_buf[4096];
	int	old_len, new_len;
	// read and compare the files
	while(1){
		old_len=old_f.read(old_buf, 4096);
		new_len=new_f.read(new_buf, 4096);
		if (old_len == new_len){
			if (old_len == 0){
				return 1;
			}
			if (memcmp(old_buf, new_buf, old_len)){
				return 0;
			}
		} else {
			return 0;
		}
	}
}

