#ifndef __NGET__PATH_H__
#define __NGET__PATH_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string>

#ifdef WIN32
inline bool is_pathsep(char c) {return c=='/' || c=='\\';}
bool is_abspath(const char *p);
#else
inline bool is_pathsep(char c) {return c=='/';}
inline bool is_abspath(const char *p) {return p[0]=='/';}
#endif

string& path_append(string &a, string b);//modifies and returns 'a'
string path_join(string a, string b);//returns a new string
string path_join(string a, string b, string c);//convenience func

#endif
