/*
    misc.* - misc functions
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
#ifndef _MISC_H_
#define _MISC_H_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>
#include <sys/types.h>
#include <stdio.h>

#include <string>
#include "_sstream.h"


int fexists(const char * f);
#define FSEARCHPATH_ALLOWDIRS 1 //search for f in the paths even if it contains a slash
const char *fsearchpath(const char * f,const char **paths,int flags);
int testmkdir(const char * dir,int mode);
char *goodgetcwd(char **p);

#define TCONV_DEF_BUF_LEN 60
size_t tconv(char * timestr, int max, time_t *curtime,const char * formatstr="%Y%m%dT%H%M%S", int local=1);

template <class Type>
string tostr(const Type &arg){
    ostringstream buffer;
    buffer << arg;
    return buffer.str();
}

template <class int_type>
string durationstr(int_type duration);

time_t decode_textdate(const char * cbuf, bool local=true);
time_t decode_textage(const char *cbuf);
int decode_textmonth(const char * buf);
int decode_texttz(const char * buf);

#ifdef	USE_FILECOMPARE					// check for duplicate files
int filecompare(const char *old_fn,const char *nfn);
#endif

#endif
