#ifndef __ARGPARSER_H__
#define __ARGPARSER_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <list>
#include <string>

typedef list<string> arglist_t;
void parseargs(arglist_t &argl, const char *s, bool ignorecomments=false);

#endif
