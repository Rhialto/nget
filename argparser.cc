/*
    argparser.* - parse a string into a list of args
    Copyright (C) 1999,2002  Matthew Mueller <donut AT dakotacom.net>

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
#include "argparser.h"
#include <ctype.h>
#include "log.h"

void parseargs(arglist_t &argl, const char *optarg, bool ignorecomments){
	string curpart;
	const char *cur;
	char quote=0;
	bool esc=0;
	for(cur=optarg;*cur;cur++){
		if (esc){
			switch (*cur){
				case '"':case '\'':
					break;
				default:
					curpart+='\\';//avoid having to double escape stuff other than quotes..
					break;
			}
			esc=0;
			curpart+=*cur;
			continue;
		}
		if (isspace(*cur))
			if(!quote){
				if (!curpart.empty()){
					argl.push_back(curpart);
					curpart="";
				}
				continue;
			}
		if (ignorecomments && !quote && *cur=='#' && curpart.empty()) {
			return;
		}
		switch(*cur){
			case '\\':esc=1;continue;
			case '"':case '\'':
				if (quote==*cur) {quote=0;continue;}
				else if (!quote) {quote=*cur;continue;}
				//else drop through
			default:
				curpart+=*cur;
		}
	}
	if (quote) throw UserExFatal(Ex_INIT,"unterminated quote (%c)", quote);
	if (esc) throw UserExFatal(Ex_INIT,"expression ended with escape");
	if (!curpart.empty())
		argl.push_back(curpart);
}
