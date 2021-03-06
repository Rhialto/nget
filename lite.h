/*
    lite.* - ngetlite main files
    Copyright (C) 2000-2001  Matthew Mueller <donut AT dakotacom.net>

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
#ifndef _LITE_H__
#define _LITE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

char * newstrcpy(char *&dest, const char *src);
inline void safefree(char *&p){
    if (p){free(p);p=NULL;}
}
inline int safestrcmp(const char *a, const char *b){
    if (!a && !b) return 0;
    if (!a || !b) return a?1:-1;
    return strcmp(a,b);
}
#endif
