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
#include <multimap.h>

#include "myregex.h"

class file_match {
	public:
		c_regex_nosub reg;
		ulong size;
		file_match(const char *m,int a,ulong s):reg(m,a), size(s){};
};
typedef multimap<ulong, file_match *> filematchmap;

class dupe_file_checker {
	private:
		filematchmap flist;
	public:
		void addfrompath(const string &path);
		void addfile(const string &path, const char *filename);
		void add(const char *filename, ulong size);
		int checkhavefile(const char *f, const string &messageid, ulong bytes);
		bool empty (void) const {return flist.empty();}
		void clear(void);
		~dupe_file_checker() {clear();}
};

#endif
