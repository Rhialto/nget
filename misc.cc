/*
    misc.* - misc functions
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "misc.h"
#include "strreps.h"
#include "log.h"
#include "file.h"
#include "path.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "myregex.h"

#include "_sstream.h"
#include <iomanip>


const char hexchar[] = "0123456789abcdef";
string hexstr(const string &s){
	string ret;
	for (string::const_iterator i=s.begin(); i!=s.end(); ++i) {
		uchar c=*i;
		ret += hexchar[c>>4];
		ret += hexchar[c&15];
	}
	return ret;
}

void parsestr_valcheck(const string &val, bool is_signed) {
	if (val.empty())
		throw parse_error("empty val");
	if (!is_signed && val.find('-')!=string::npos)
		throw parse_error("invalid unsigned value");
}
void parsestr_isscheck(istringstream &iss) {
	if (iss.fail() || iss.bad())
		throw parse_error("invalid value");
	if (!iss.eof() && iss.peek()!=EOF)
		throw parse_error("trailing junk");
}

string strtolower(const string &s){
	string sl = s;
	lowerstr(sl);
	return sl;
}

void lowerstr(string &s){
	for (string::iterator i=s.begin(); i!=s.end(); ++i)
		*i=tolower(*i);
}

bool strstartswith(const string &s, const string &t) {
	return s.substr(0,t.size()) == t;
}

string regex2wildmat(const string &repat, bool ignorecase){
	if (repat.empty())
		return "*";
	string wildmat;
	unsigned int pos=0;
	if (repat[0]=='^')
		pos++;
	else
		wildmat += '*'; //wildmats are anchored by default, while regexs are the opposite
	while (pos<repat.size()) {
		char c = repat[pos];
		++pos;
		if (c == '.') {
			if (pos<repat.size() && repat[pos] == '*') {
				wildmat += '*';
				++pos;
			}else
				wildmat += '?';
		}
		else if (c == '\\') {
			if (pos>=repat.size())
				throw RegexEx(Ex_INIT,"error converting regex(%s) to wildmat on char %i: %c", repat.c_str(), pos, c);
			char nc = repat[pos];
			if (nc == '*' || nc == '?' || nc == '[' || nc == ']') {
				wildmat += c;
				wildmat += nc;
				++pos;
			}
			else if (nc == '(' || nc == ')' || nc == '{' || nc == '}' || nc == '|' || nc == '.') {
				wildmat += nc;
				++pos;
			}
			else if (nc == '<' || nc == '>' || isalnum(nc))
				throw RegexEx(Ex_INIT,"error converting regex(%s) to wildmat on char %i: %c", repat.c_str(), pos, c);
			else {
				wildmat += nc;
				++pos;
			}
		}
		else if (c == '?' || c == '*' || c == '+' || c == '(' || c == ')' || c == '{' || c == '}' || c == '|')
			throw RegexEx(Ex_INIT,"error converting regex(%s) to wildmat on char %i: %c", repat.c_str(), pos, c);
		else if (pos==repat.size() && c == '$')
			break;//wildmats are anchored by default, while regexs are the opposite
		else if (ignorecase && isalpha(c)) {
			wildmat += '[';
			wildmat += tolower(c);
			wildmat += toupper(c);
			wildmat += ']';
		} else if (c == '[') {
			wildmat += '[';
			int nc = -1;
			unsigned int opos=pos;
			while (pos<repat.size()) {
				nc = repat[pos++];
				if (nc == ']' && opos+1!=pos){
					wildmat += ']';
					break;
				} else if (ignorecase && isalpha(nc)) {
					wildmat += tolower(nc);
					wildmat += toupper(nc);
				} else
					wildmat += nc;
			}
			if (nc!=']')
				throw RegexEx(Ex_INIT,"error converting regex(%s) to wildmat on char %i: %c", repat.c_str(), pos, nc);
		} else {
			wildmat += c;
		}
		if (pos==repat.size())
			wildmat += '*';//wildmats are anchored by default, while regexs are the opposite
	}
	//printf("converted %s->%s\n",repat.c_str(),wildmat.c_str());//######
	return wildmat;
}

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
//Sun, 7 Jul 2002 15:6:5 GMT //1 digit minutes, seconds
//Tue, 07 Aug 2002  0:21:00 GMT //1 digit hour with space pad
//Sun, 8 Sep 2002 0:19:2 GMT //1 digit hour, no pad

//Sunday,
// 06-Nov-94 08:49:37 GMT ; RFC 850, obsoleted by RFC 1036
//Sun Nov  6 08:49:37 1994       ; ANSI C's asctime() format (note, this is used by ls -l --full-time)
//Jan  4 17:11 			;ls -l format, 17:11 may be replaced by " 1998"

//easy format 06/11/94[ 23:34[:23][ -300]]
//c_regex rfc1123("^[A-Za-z, ]*([0-9]{1,2}) (...) ([0-9]{2,4}) ([0-9]{1,2}):([0-9]{2}):([0-9]{2}) (.*)$"),
//	rfc850("^[A-Za-z, ]*([0-9]{1,2})-(...)-([0-9]{2,4}) ([0-9]{1,2}):([0-9]{2}):([0-9]{2}) (.*)$"),
//#define TIME_REG "([0-9]{1,2}):([0-9]{2}):([0-9]{2})"

//allows for optional seconds
#define TIME_REG2 "([0-9 ]?[0-9]):([0-9]{1,2})(:([0-9]{1,2}))?"
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
	off_t old_size, new_size;
	if (!fsize(old_fn,&old_size) && !fsize(nfn, &new_size) && old_size!=new_size)
		return 0;
	c_file_fd old_f(old_fn, O_RDONLY|O_BINARY);
	c_file_fd new_f(nfn, O_RDONLY|O_BINARY);
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

