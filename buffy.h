/*
    buffy.* - simple buffering for low cpu gets() functionality (ex. for sockets).
    Copyright (C) 2000,2002  Matthew Mueller <donut@azstarnet.com>

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
#ifndef _BUFFY_H__
#define _BUFFY_H__
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <sys/types.h>
#include "log.h"

class CharBuffer {
	protected:
		char *cbuf;
		int bsize;
		int reserved;
	public:
		void reserve(int s) {
			if (s>reserved){
				cbuf=(char*)realloc(cbuf,s);
				if (!cbuf)
					throw ApplicationExFatal(Ex_INIT,"CharBuffer::resizebuf(%i) realloc returned NULL",s);
				reserved=s;
			}
		}
		int capacity(void)const{return reserved;}
		int size(void)const{return bsize;}
		bool empty(void)const{return !bsize;}
		void clear(void){bsize=0;}
		char *data(void){return cbuf;}
		char *c_str(void){
			if (bsize>=reserved)
				reserve(reserved*2);
			cbuf[bsize]=0;
			return cbuf;
		}
		void append(char b){
			if (bsize>=reserved)
				reserve(reserved*2);
			cbuf[bsize++]=b;
		}
		CharBuffer(int initialreserve=3) : cbuf(NULL),bsize(0),reserved(0) {
			reserve(initialreserve);
		}
		~CharBuffer() {
			if (cbuf) free(cbuf);
		}
};

class c_buffy {
	protected:
		int bhead,btail;
#define buffy_bufsize 2048 //must be a multiple of 2 so that we can use &=bufmask which is much faster than %=bufsize
		uchar buf[buffy_bufsize];
#define buffy_bufmask 2047

		int buftailfree(void){
			if (btail>=bhead)return buffy_bufsize-btail-(bhead==0?1:0);
			else return bhead-btail-1;
		}
		void inchead(int i){bhead+=i;bhead&=buffy_bufmask;}
		void inctail(int i){btail+=i;btail&=buffy_bufmask;}
		virtual int bfill(uchar *b,int l)=0;
		int dofill(void){
			int i=bfill(buf+btail,buftailfree());
			if (i<=0)return i;
			inctail(i);
			return i;
		}
	public:
		void clearbuf(void){bhead=btail=0;}
		int bpeek(void){
			if (bhead==btail)
				if (dofill()<=0)return -1;
			return buf[bhead];
		}
		int bget(void){
			if (bhead==btail)
				if (dofill()<=0)return -1;
			int r=buf[bhead];
			inchead(1);
			return r;
		}
/*		int read(char *b,int l){
			int i,j;
			for (i=0;i<l;i++){
				j=getc();
				if (j<0)return j;
				b[i]=j;
			}
			return i;
		}*/
		int bgets(CharBuffer &buf){
			int i,rcount=0;
			buf.clear();
			while (1){
				i=bget();
				rcount++;
				if (i<0)
					return -1;
				else if (i==13){
					continue;
				}else if (i==10){
					return rcount;//we return the real count of bytes swallowed, even though eol isn't returned.
				}
				buf.append(i);
			}
		}
		c_buffy(){bhead=btail=0;/*buf=NULL;buffy_bufsize=0;*/}
		virtual ~c_buffy(){/*if (buf) free(buf);*/}
};
#endif
