/*
    path.* - attempt to have portable path manipulation
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
#include "path.h"
#include <errno.h>
#include <unistd.h>
#include "log.h"
#include "_fileconf.h"

#ifdef WIN32
#include <ctype.h>
bool is_abspath(const char *p) {
	if (is_pathsep(p[0]))
		return true;
	if (p[0]!=0 && p[1]!=0 && p[2]!=0 && isalpha(p[0]) && p[1]==':' && is_pathsep(p[2]))
		return true;
	return false;
}
#endif

string& path_append(string &a, string b) {
	if (a.empty()) {
		a=b;
		return a;
	}
	char c=a[a.size()-1];
	if (!is_pathsep(c))
		a += PATHSEP;
	return a.append(b);
}

string path_join(string a, string b) {
	return path_append(a, b);
}
string path_join(string a, string b, string c) {
	return path_append(path_append(a,b), c);
}

void path_split(string &head, string &tail) {
	tail = "";
	while (!head.empty()) {
		char c = *head.rbegin();
		head.resize(head.size()-1);
		if (is_pathsep(c)) {
			if (head.empty())
				tail = c + tail;
			if (!tail.empty())
				return;
		}else
			tail = c + tail;
	}
}

bool direxists(const string &p) {
	bool ret=false;
	char *oldp=goodgetcwd();
	if (chdir(p.c_str())==0)
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
int fsize(const char * f, off_t *size){
	struct stat statbuf;
	if (stat(f,&statbuf))
		return -1;
	*size=statbuf.st_size;
	return 0;
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
char *goodgetcwd(void){
	/*int t=-1;
	*p=NULL;//first we try to take advantage of the GNU extension to allocate it for us.
	while (!(*p=getcwd(*p,t))) {
		t+=100;
		if (!(*p=(char*)realloc(*p,t))){
			PERROR("realloc error %s",strerror(errno));exit(1);
		}
	}*/
	int t=32;
	char *p=NULL;//ack, that extension seems to like segfaulting when you free() its return.. blah.
	do {
		t*=2;
		if (!(p=(char*)realloc(p,t))){
			PERROR("goodgetcwd: realloc error %s (size=%i)",strerror(errno), t);exit(128);
		}
	}while(!getcwd(p,t));
	PDEBUG(DEBUG_MED,"goodgetcwd %i %s(%p)",t,p,p);
	return p;
}

