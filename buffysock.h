/*
    buffysock.* - simple buffering for low cpu gets() functionality (ex. for sockets).
    Copyright (C) 2000  Matthew Mueller <donut@azstarnet.com>

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
#ifndef _BUFFYSOCK_H__
#define _BUFFYSOCK_H__

#include "buffy.h"
#include "sockstuff.h"
#include <unistd.h>
class sockbuffy :public c_buffy{
	private:
		virtual int bfill(uchar *b,int l){
			int i=sock_read(sock,b,l);
			if (i<0) throw TransportExError(Ex_INIT,"sock_read (%s)", strerror(errno));
			return i;
		}
		int sock;
	public:
		int write(const char *buf, size_t count){
			return sock_write_ensured(sock,buf,count);
		}
		int vprintf(const char *str, va_list ap){
			return sockvprintf(sock, str, ap);
		}
		int printf(const char *str,...){
			int r;
			va_list ap;
			va_start(ap,str);
			r=vprintf(str,ap);
			va_end(ap);
			return r;
		}
		int isopen(void)const{
			return sock>=0;
		}
		int open(const char *netaddress,const char *service){
			sock=make_connection(SOCK_STREAM,netaddress,service,(char*)buf,buffy_bufsize);
			clearbuf();
			if (isopen())return 0;
			else return -1;
		}
		int close(void){
			if (isopen()){
				int ret=sock_close(sock);
				sock=-1;
				return ret;
			}else
				return 0;
		}
		sockbuffy():sock(-1){/*bsetbuf(2048);*/};
		~sockbuffy(){close();}
};

#endif
