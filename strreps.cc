/*
    strreps.* - replacements for some string funcs that aren't always available
    Copyright (C) 1999-2002  Matthew Mueller <donut@azstarnet.com>

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
#include "strreps.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_CONFIG_H

#ifndef HAVE_ASPRINTF
#include <stdarg.h>
int asprintf(char **str,const char *format,...){
	int l;
	va_list ap;

	va_start(ap,format);
	l=vasprintf(str,format,ap);
	va_end(ap);
	return l;
}
#endif

#ifndef HAVE_VASPRINTF
int vasprintf(char **str,const char *format,va_list ap){
#ifndef _REENTRANT
	static
#endif
		char buf[4096];
	int l;
	l=vsprintf(buf,format,ap);
	*str=(char*)malloc(l+1);
	memcpy(*str,buf,l+1);
	return l;
}
#endif

#ifdef NEED_CRAPPY_STRERROR
const char * crappy_strerror(int err){
	static char buf[12];
	sprintf(buf,"%i",err);
	return buf;
}
#endif

#endif //HAVE_CONFIG_H
