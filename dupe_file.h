/*
    dupe_file.* - dupe file detection code
    Copyright (C) 1999-2001  Matthew Mueller <donut@azstarnet.com>

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

#ifndef __DUPE_FILE_H__
#define __DUPE_FILE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <string>
#include SLIST_H

#include "myregex.h"

class file_match {
	public:
		c_regex_nosub reg;
		ulong size;
		file_match(const char *m,int a):reg(m,a){};
};
typedef slist<file_match *> filematchlist;

void buildflist(const string &path, filematchlist **l);
void flist_addfile(filematchlist &l, const string &path, const char *filename);
int flist_checkhavefile(filematchlist *fl,const char *f,string messageid,ulong bytes);

#endif
