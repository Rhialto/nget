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
#ifndef _MISC_H_
#define _MISC_H_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>
#include <sys/types.h>
#include <stdio.h>

#ifdef HAVE_LIMITS
#include <limits>
#endif
#include <stdexcept>
#include <string>
#include "status.h"
#include "_sstream.h"
#include "log.h"


#define TCONV_DEF_BUF_LEN 60
size_t tconv(char * timestr, int max, time_t *curtime,const char * formatstr="%Y%m%dT%H%M%S", int local=1);

string hexstr(const string &s);

template <class Type>
string tostr(const Type &arg){
    ostringstream buffer;
    buffer << arg;
    return buffer.str();
}

class parse_error : public runtime_error {
    public:
        explicit parse_error(const string &arg) : runtime_error(arg) {}
};

// make these seperate functions to cut down how much has to be inlined.
void parsestr_valcheck(const string &val, bool is_signed);
void parsestr_isscheck(istringstream &iss);

template <class T>
void parsestr(const string &val, T &dest) {
#ifdef HAVE_LIMITS
    parsestr_valcheck(val, numeric_limits<T>::is_signed);
#else
    parsestr_valcheck(val, true);
#endif
    istringstream iss(val);
    T v;
    iss >> v;
    parsestr_isscheck(iss);
    dest = v;
}

template <class T>
bool parsestr(const string &val, T &dest, const char *name) {
    try {
        parsestr(val, dest);
        return true;
    } catch (parse_error &e) {
        PERROR("%s: %s", name, e.what());
        set_user_error_status_and_do_fatal_user_error();
    }
    return false;
}

template <class T>
void parsestr(const string &val, T &dest, T min, T max) {
    T v;
    parsestr(val, v);
    if (v<min || v>max)
        throw parse_error("not in range "+tostr(min)+".."+tostr(max));
    dest = v;
}

template <class T>
bool parsestr(const string &val, T &dest, T min, T max, const char *name) {
    try {
        parsestr(val, dest, min, max);
        return true;
    } catch (parse_error &e) {
        PERROR("%s: %s", name, e.what());
        set_user_error_status_and_do_fatal_user_error();
    }
    return false;
}


string strtolower(const string &s);
void lowerstr(string &s);

bool strstartswith(const string &s, const string &t);

string regex2wildmat(const string &repat, bool ignorecase=false);

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


time_t decode_textdate(const char * cbuf, bool local=true);
time_t decode_textage(const char *cbuf);
int decode_textmonth(const char * buf);
int decode_texttz(const char * buf);

int filecompare(const char *old_fn,const char *nfn);

#endif
