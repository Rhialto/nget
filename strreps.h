/*
    strreps.* - replacements for some string funcs that aren't always available
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

#ifndef _STRREPS_H_incd_
#define _STRREPS_H_incd_


#ifdef HAVE_CONFIG_H
#include "config.h"

#ifndef HAVE_ASPRINTF
int asprintf(char **str,const char *format,...);
#endif

#ifndef HAVE_VASPRINTF
#include <stdarg.h>
int vasprintf(char **str,const char *format,va_list ap);
#endif

#ifndef HAVE_ATOUL
//inline ulong atoul(const char *str){return strtoul(str,NULL,10);}
#define atoul(str) strtoul(str,NULL,10)
#endif

#ifndef HAVE_STRERROR
const char * strerror(int err);
#endif

#endif //HAVE_CONFIG_H

#include <stdio.h>


#endif
