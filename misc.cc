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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "myregex.h"

#include <string>
#include "_sstream.h"
#include <iomanip>

#ifdef HAVE_CONFIG_H
#ifndef HAVE_LOCALTIME_R
struct tm * localtime_r(const time_t *t,struct tm * tmu){
	struct tm *t1=localtime(t);
	if (!t1) return NULL;
	memcpy(tmu,t1,sizeof(struct tm));
	return tmu;
}
#endif
#endif//HAVE_CONFIG_H

long my_timezone=0;
#if (defined(TIMEZONE_IS_VAR) || defined(_TIMEZONE_IS_VAR))
void init_my_timezone(void){
	tzset();
#ifdef TIMEZONE_IS_VAR
	my_timezone=-timezone; //timezone var is seconds west of UTC, not east... weird.
#else
	my_timezone=-_timezone;
#endif
//	printf("my_timezone1=%li\n",my_timezone);
}
#else
void init_my_timezone(void){
	struct tm tm;
	time_t t;
	time(&t);
	localtime_r(&t,&tm);
	my_timezone=tm.tm_gmtoff;
//	printf("my_timezone2=%li\n",my_timezone);
}
#endif

int doopen(int &handle,const char * name,int access,int mode=0) {
	if ((handle=open(name,access,mode))==-1){
//      if (domiscquiet)
//      domiscquiet--;
//      else
		PERROR("Error opening %s: %s",name,strerror(errno));
		return -1;
	}
	else return 0;
}

int dofopen(FILE * &f,const char * name,const char * mode,int domiscquiet=0) {
//   FILE *f;
	if ((f=fopen(name,mode))==NULL){
		if (!domiscquiet)
			PERROR("Error opening %s: %s",name,strerror(errno));
		return -1;
	}
	return 0;
}

int fexists(const char * f){
	struct stat statbuf;
	return (!stat(f,&statbuf));
}
const char *fsearchpath(const char * f,const char **paths,int flags){
	char *s;
	int i;
	if (!f) return NULL;
	if (f[0]!='~' && f[0]!='/' && (flags&FSEARCHPATH_ALLOWDIRS || strchr(f,'/')==NULL))
		for (i=0;paths[i];i++){
			asprintf(&s,"%s/%s",paths[i],f);
//			PDEBUG(DEBUG_MIN,"fsearchpath:%s",s);
			if (fexists(s))
				return s;
			free(s);
		}
//	PDEBUG(DEBUG_MIN,"fsearchpath:%s",f);
	if (fexists(f)){
//		asprintf(&s,"%s",f);
//		return s;
		return f;//not dynamically allocated
	}
	return NULL;
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
template string durationstr(int_fast64_t);
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
	xasctime("^[A-Za-z, ]*(...) +([0-9]{1,2}) "TIME_REG2" ([0-9]{2,4}) *(.*)$"),
	xlsl("^(...) +([0-9]{1,2}) +([0-9:]{4,5})$"),
	xeasy("^([0-9]{1,4})[-/]([0-9]{1,2})[-/]([0-9]{2,4})( *"TIME_REG2" *(.*))?$"),
	xiso("^([0-9]{4})-?([0-9]{2})-?([0-9]{2})([T ]?([0-9]{1,2})(:?([0-9]{2})(:?([0-9]{2}))?)?)? *([-+A-Z].*)?$");
c_regex_nosub xtime_t("^[0-9]+$")//seconds since 1970 (always gmt)
		;
time_t decode_textdate(const char * cbuf, bool local){
	struct tm tblock;
	memset(&tblock,0,sizeof(struct tm));
	char *tdt=NULL;
	int td_tz=local?my_timezone:0;
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
			struct tm lt;
			time_t curtime;
			time(&curtime);
			localtime_r(&curtime,&lt);

			tblock.tm_hour=atoi(rsubs.subs(3));
			tblock.tm_min=atoi(rsubs.subs(3)+3);

			if (lt.tm_mon>=tblock.tm_mon)
				tblock.tm_year=lt.tm_year;
			else
				tblock.tm_year=lt.tm_year-1;
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
	return mktime(&tblock)+my_timezone-td_tz;
}
#ifdef NO_REGEXPS
time_t decode_textdate(const char * cbuf){
	int len=strlen(cbuf);
	struct tm tblock;
	memset(&tblock,0,sizeof(struct tm));
	char *tdt=NULL;
	int td_tz=0;
	//      PERROR("decode_textdate(%s)",cbuf);
	if (len==12 && cbuf[3]==' ' && cbuf[6]==' '){
		tdt="ls-l-date";
		tblock.tm_mon=decode_textmonth(cbuf);
		if (cbuf[4]==' ')
			tblock.tm_mday=atoi(cbuf+5);
		else
			tblock.tm_mday=atoi(cbuf+4);
		if (cbuf[9]==':'){
			struct tm lt;
			time_t curtime;
			time(&curtime);
			localtime_r(&curtime,&lt);

			tblock.tm_hour=atoi(cbuf+7);
			tblock.tm_min=atoi(cbuf+10);

			if (lt.tm_mon>=tblock.tm_mon)
				tblock.tm_year=lt.tm_year;
			else
				tblock.tm_year=lt.tm_year-1;
		}else{
			tblock.tm_year=atoi(cbuf+8)-1900;
		}
	}
	else if ((len>28 && cbuf[3]==',' && cbuf[19]==':' && cbuf[22]==':')||
			(len>27 && cbuf[3]==',' && cbuf[18]==':' && cbuf[21]==':')||
			(len>23 && cbuf[2]==' ' && cbuf[14]==':' && cbuf[17]==':') ||
			(len>22 && cbuf[1]==' ' && cbuf[13]==':' && cbuf[16]==':')){
		//		PERROR("decode_textdate:  rfc1123-date");
		tdt="rfc1123-date";
		int ofs,nopad=0;
		if (cbuf[1]==' '){
			ofs=-1;nopad=1;
		}else if (cbuf[2]==' ')
			ofs=0;
		else if (cbuf[18]==':') {
			ofs=4;nopad=1;
		}else
			ofs=5;
		tblock.tm_mday=atoi(cbuf+ofs+nopad);
		tblock.tm_mon=decode_textmonth(cbuf+3+ofs);
		tblock.tm_year=atoi(cbuf+7+ofs)-1900;
		tblock.tm_hour=atoi(cbuf+12+ofs);
		tblock.tm_min=atoi(cbuf+15+ofs);
		tblock.tm_sec=atoi(cbuf+18+ofs);
		td_tz=decode_texttz(cbuf+21+ofs);
	}
	else if (len>23 && cbuf[3]==' ' && cbuf[13]==':' && cbuf[16]==':'){
//			PERROR("decode_textdate:  asctime-date");
		tdt="asctime-date";
		if (len>=24){
			tblock.tm_mon=decode_textmonth(cbuf+4);
			if (cbuf[8]==' ')
				tblock.tm_mday=atoi(cbuf+9);
			else
				tblock.tm_mday=atoi(cbuf+8);
			tblock.tm_hour=atoi(cbuf+11);
			tblock.tm_min=atoi(cbuf+14);
			tblock.tm_sec=atoi(cbuf+17);
			tblock.tm_year=atoi(cbuf+20)-1900;
		}
	}
	else if (len>27){
		char *p=strchr(cbuf,' ');
		if (p && strlen(p)>=19 && p[13]==':' && p[16]==':'){
			tdt="rfc850-date";
//		PERROR("decode_textdate:  rfc850-date");
			tblock.tm_mday=atoi(p+1);
			tblock.tm_mon=decode_textmonth(p+4);
			tblock.tm_year=atoi(p+8);
			if (tblock.tm_year>99){
				tblock.tm_year-=1900;
				p+=2;//account for four digit year
			}else if (tblock.tm_year<70){
				tblock.tm_year+=100;//avoid y2k probs with 2 digit years
			}
			tblock.tm_hour=atoi(p+11);
			tblock.tm_min=atoi(p+14);
			tblock.tm_sec=atoi(p+17);
			td_tz=decode_texttz(p+20);
		}
	}
	if(!tdt){
		PERROR("decode_textdate: unknown %s",cbuf);
		return 0;
	}else
		PDEBUG("decode_textdate: %s %i %i %i %i %i %i %i",tdt,tblock.tm_year,tblock.tm_mon,tblock.tm_mday,tblock.tm_hour,tblock.tm_min,tblock.tm_sec,td_tz);
	return mktime(&tblock)-my_timezone-td_tz;
}
#endif

#ifdef	USE_FILECOMPARE					// check for duplicate files
int filecompare(const char *old_fn,const char *nfn){
	int	same=0;
	FILE	*old_f, *new_f;
	// open both files
	if (!dofopen(old_f, old_fn, "r")){
		if (!dofopen(new_f, nfn, "r")){
			char	old_buf[4096], new_buf[4096];
			int	old_len, new_len;
			// read and compare the files
			while(!feof(old_f)&&!ferror(old_f)&&!feof(new_f)&&!ferror(new_f)){
				old_len=read(fileno(old_f), old_buf, 4096);
				new_len=read(fileno(new_f), new_buf, 4096);
				if (old_len == new_len && old_len >= 0){
					if (old_len == 0){
						same=1;
						break;
					}
					if (memcmp(old_buf, new_buf, old_len)){
						same=-1;
						break;
					}
				} else {
					same=-1;
					break;
				}
			}
			if (same>=0&&feof(old_f)&&feof(new_f)){
				// passed all tests!
				same=1;
			}
			fclose(new_f);
			fclose(old_f);
		} else {
			same = -1;
			fclose(old_f);
		}
	} else {
		same = -1;
	}
	return same;
}
#endif

