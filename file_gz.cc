/*
    file.* - file io classes
    Copyright (C) 1999-2002  Matthew Mueller <donut AT dakotacom.net>

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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "file.h"
#include <errno.h>
#include <string.h>
#include "strreps.h"


#ifdef HAVE_LIBZ
c_file_gz::c_file_gz(const char *name,const char * mode):c_file(name){
	if (!(gzh=gzopen(name,mode)))
		THROW_OPEN_ERROR("gzopen %s (%s)", name, strerror(errno));
}
const char *c_file_gz::dostrerror(void) {
	if (gzh) {
		int foo;
		const char *err = gzerror(gzh, &foo);
		if (foo!=Z_ERRNO)
			return err;
	}
	return strerror(errno);
}
int c_file_gz::doflush(void){
	if (gzh)
		return gzflush(gzh,Z_SYNC_FLUSH);
	return 0;
}
int c_file_gz::doclose(void){
	int i=0;
	i=gzclose(gzh);
	gzh=NULL;
	return i;
}
inline int c_file_gz::isopen(void)const{
	return (gzh!=0);
}
inline ssize_t c_file_gz::dowrite(const void *data,size_t len){
	return gzwrite(gzh,(void*)data,len);
}
inline ssize_t c_file_gz::doread(void *data,size_t len){
	return gzread(gzh,data,len);
}
#endif

