/*
    dupe_file.* - dupe file detection code
    Copyright (C) 1999-2003  Matthew Mueller <donut AT dakotacom.net>

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
#include "dupe_file.h"
#include "log.h"
#include "path.h"

#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

void dupe_file_checker::add(const char *filename, ulong size){
	file_match *fm;
	string buf;
	if (isalnum(filename[0])) //only add word boundry match if we are actually next to a word, or else there isn't a word boundry to match at all.
		buf+=regex_match_word_beginning();
	regex_escape_string(filename, buf);
	if (isalnum(buf[buf.size()-1]))
		buf+=regex_match_word_end();
	fm=new file_match(buf.c_str(),REG_EXTENDED|REG_ICASE|REG_NOSUB,size);
	flist.insert(filematchmap::value_type(fm->size, fm));
}

void dupe_file_checker::addfile(const string &path, const char *filename) {
	ulong size=0;
	struct stat stbuf;
	if (stat(path_join(path,filename).c_str(), &stbuf)==0)
		size=stbuf.st_size;
	add(filename, size);
}

void dupe_file_checker::addfrompath(const string &path){
	DIR *dir=opendir(path.c_str());
	struct dirent *de;
	if (!dir)
		throw PathExFatal(Ex_INIT,"opendir: %s(%i)",strerror(errno),errno);
	while ((de=readdir(dir))) {
		if (strcmp(de->d_name,"..")==0) continue;
		if (strcmp(de->d_name,".")==0) continue;
		addfile(path, de->d_name);
	}
	closedir(dir);
}

int dupe_file_checker::checkhavefile(const char *f, const string &messageid, ulong bytes) const {
	filematchmap::const_iterator curl = flist.upper_bound(bytes/2); //find first fm with size*2 > bytes
	file_match *fm;
	for (;curl!=flist.end() && curl->first<bytes;++curl){ //keep going as long as size < bytes
		fm=curl->second;
		if ((fm->reg.match(f)==0/* || fm->reg.match((messageid+".txt").c_str())==0*/)){//TODO: handle text files saved.
			PMSG("already have %s",f);
			return 1;
		}
	}

	return 0;
}

void dupe_file_checker::clear(void){
	filematchmap::iterator curl;
	for (curl=flist.begin();curl!=flist.end();++curl)
		delete curl->second;
	flist.clear();
}



