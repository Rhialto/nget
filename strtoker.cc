/*
    strtoker.* - good strtok replacements
    Copyright (C) 1999-2001  Matthew Mueller <donut AT dakotacom.net>

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

#include "strtoker.h"
#include <string.h>

// threadsafe.
char * goodstrtok(char **cur, char sep){
	char * tmp, *old;
	if (!*cur) return NULL;
	old=*cur;
	if ((tmp=strchr(*cur,sep))==NULL){
		*cur=NULL;
		return old;
	}
	tmp[0]=0;
	*cur=tmp+1;
	return old;
}
strtoker::strtoker(int num,char tok){
	toks=new char*[num];
	maxtoks=num;
	numtoks=0;
	tokchar=tok;
}
int strtoker::tok(char *str){
	for (numtoks=0;numtoks<maxtoks;numtoks++)
		if ((toks[numtoks]=goodstrtok(&str,tokchar))==NULL)
			return 0;
	if (str)
		return -1;//maxtoks wasn't enough.
	else
		return 0;//done.
}

