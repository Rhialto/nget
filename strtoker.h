/*
    strtoker.* - good strtok replacements
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

#ifndef _STRTOKER_H_incd_
#define _STRTOKER_H_incd_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

//goodstrtok: (NOTE: modifies cur!)
//*cur should be set to string to tokenize on first call.
//returns token, or NULL if finished.
//thread safe.
char * goodstrtok(char **cur, char sep);

//whee!
class strtoker {
	protected:
		char **toks;
		int maxtoks;
	public:
		int numtoks;
		char tokchar;
		char * operator[](int i){return toks[i];}
		int tok(char *str);
		strtoker(int num,char tok);
		~strtoker(){delete toks;}
};

#endif
