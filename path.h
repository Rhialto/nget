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
#ifndef __NGET__PATH_H__
#define __NGET__PATH_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <string>
#include <list>

#ifdef WIN32
inline bool is_pathsep(char c) {return c=='/' || c=='\\';}
bool is_abspath(const char *p);
#define PATHSEP '\\'
#else
inline bool is_pathsep(char c) {return c=='/';}
inline bool is_abspath(const char *p) {return p[0]=='/';}
#define PATHSEP '/'
#endif

string& path_append(string &a, string b);//modifies and returns 'a'
string path_join(string a, string b);//returns a new string
string path_join(string a, string b, string c);//convenience func

void path_split(string &head, string &tail);

bool direxists(const string &p);
int fexists(const char * f);
int fsize(const char * f, off_t *size);
string fcheckpath(const char *fn,string path);
int testmkdir(const char * dir,int mode);
char *goodgetcwd(char **p);

#endif
