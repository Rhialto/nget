/*
    log.* - debug/error logging and exception defines
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
#ifndef _LOG_H_
#define _LOG_H_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "strreps.h"
//#include <string.h>
#define PERROR(a, args...) fprintf(stderr,a "\n" , ## args)
#define PMSG(a, args...) printf(a "\n" , ## args)
//#define PDEBUG(a, args...) printf(a , ## args)
#define PDEBUG(d, a, args...) {if (debug>=d) printf(a "\n" , ## args);}
extern int debug;
#define DEBUG_MIN 1
#define DEBUG_MED 2
#define DEBUG_ALL 3
extern int quiet;

class baseEx {
	public:
		virtual bool isfatal(void)const=0;
		virtual const char* getExType(void)const=0;
		virtual const char* getExStr(void)const=0;
		virtual const char* getExFile(void)const=0;
		virtual int getExLine(void)const=0;
};

class ExError : public baseEx{
	public:
		virtual bool isfatal(void)const{return 0;}
};
class ExFatal : public baseEx{
	public:
		virtual bool isfatal(void)const{return 1;}
};
#define DEFINE_EX_SUBCLASS(name, sub) class name ## Ex ## sub : public Ex ## sub, public name ## Ex {\
		string str;\
		const char *mfile;\
		int mline;\
	public:\
		virtual const char* getExFile(void)const{return mfile;}\
		virtual int getExLine(void)const{return mline;}\
		virtual const char* getExStr(void)const{return str.c_str();}\
		virtual bool isfatal(void)const{return Ex ## sub::isfatal();}\
		virtual const char* getExType(void)const{return #name "Ex" #sub;}\
		name ## Ex ## sub(const char *file, int line, const char * s, ...):mfile(file),mline(line){\
			char *cstr;\
			va_list ap;\
			va_start(ap,s);\
			vasprintf(&cstr,s,ap);\
			va_end(ap);\
			str=cstr;\
			free(cstr);\
			/*PDEBUG(DEBUG_MIN,"%s:%i:Created exception %s with %s(%p)",mfile,mline,getExType(), getExStr(), getExStr());*/\
		}\
		virtual ~name ## Ex ## sub(){\
/*			printf("Destroying exception %s with %s(%p)\n",getExType(), getExStr(), getExStr());*/\
		}\
};

#define DEFINE_EX_CLASSES(name,base) class name ## Ex: public base {};\
DEFINE_EX_SUBCLASS(name, Fatal)\
DEFINE_EX_SUBCLASS(name, Error)

class baseCommEx: public baseEx {};
	
DEFINE_EX_CLASSES(Transport, baseCommEx);
DEFINE_EX_CLASSES(Protocol, baseCommEx);
DEFINE_EX_CLASSES(Path, baseEx);
DEFINE_EX_CLASSES(User, baseEx);
DEFINE_EX_CLASSES(Config, baseEx);
DEFINE_EX_CLASSES(Application, baseEx);

#define Ex_INIT __FILE__,__LINE__

#define printCaughtEx_nnl(e) printf("%s:%i:caught exception %s:%i:%s: %s",__FILE__,__LINE__,e.getExFile(),e.getExLine(),e.getExType(),e.getExStr());
#define printCaughtEx(e) printCaughtEx_nnl(e);printf("\n");


//#define FILE_DEBUG

#ifdef FILE_DEBUG
//#include "file.h"
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <list.h>
#include <zlib.h>

#define DEBUGLOGPUTF(a, args...) putf(__FILE__,__FUNCTION__,__LINE__,a "\n" , ## args)
#if 1 //in memory version
class c_debug_file {
	public:
		z_stream zstrm;
		list<char*> data;
		ulong size;
		char *savename;
		char *curout;
		uLong crc;
		char inbuf[1024];
		c_debug_file(){
			size=0;
			savename=NULL;
		}
		~c_debug_file(){
			purge();
		}
		void flushdata(void){
			if (!savename)
				return;
			int r;
			zstrm.next_in=NULL;
			zstrm.avail_in=0;
			while (1){
				r=deflate(&zstrm, Z_FINISH);
				//				if (zstrm.next_out==NULL || zstrm.avail_out==0){
				if (zstrm.next_out)
					data.push_back(curout);
				if (zstrm.avail_out==0 && r==Z_OK){
					curout=(char*)malloc(4096);
					zstrm.next_out=(unsigned char*)curout;
					zstrm.avail_out=4096;
				}else{
					zstrm.next_out=NULL;curout=NULL;
				}
				//				}
				if (r==Z_STREAM_END)break;
				assert(r==Z_OK || r==Z_STREAM_END);
			}
		}
		void adddata(char *dat, int len){
			crc=crc32(crc,(unsigned char*)dat,len);
			size+=len;
			zstrm.next_in=(unsigned char*)dat;
			zstrm.avail_in=len;
			while (zstrm.avail_in){
				if (zstrm.next_out==NULL || zstrm.avail_out==0){
					if (zstrm.next_out)
						data.push_back(curout);
					curout=(char*)malloc(4096);
					zstrm.next_out=(unsigned char *)curout;
					zstrm.avail_out=4096;
				}
				assert(deflate(&zstrm, Z_NO_FLUSH)==Z_OK);
			}
		}
		void start(const char *name){
			crc=crc32(0L, Z_NULL, 0);
			savename=new char[strlen(name)+1];
			strcpy(savename,name);
			memset(&zstrm,0,sizeof(z_stream));
			zstrm.zalloc=Z_NULL;
			zstrm.zfree=Z_NULL;
			curout=NULL;
#define DEF_MEM_LEVEL 8
			assert(deflateInit2(&zstrm, Z_DEFAULT_COMPRESSION,Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY)==Z_OK);
			/* windowBits is passed < 0 to suppress zlib header - got this from zlib code. */

//			assert(deflateInit(&zstrm,Z_DEFAULT_COMPRESSION)==Z_OK);
//			printf("in %p %i %li\n",zstrm.next_in,zstrm.avail_in,zstrm.total_in);
//			printf("out %p %i %li\n",zstrm.next_out,zstrm.avail_out,zstrm.total_out);
//			exit(1);
		}
		int save(void){
			flushdata();
//			gzFile gzh;
//			c_file_fd f;
			int f;
			char *d;
			printf("saving debug log to %s\n",savename);
			f=open(savename,O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR | S_IWUSR);
//			gzh=gzopen(savename,"w");
//			f.open(name.c_str(),O_CREAT|O_WRONLY|O_TRUNC,PUBMODE);

			/* Write a very simple .gz header: - got this from zlib code too. */
			static unsigned char gz_magic[10] = {0x1f, 0x8b, Z_DEFLATED,0 /*flags*/, 0,0,0,0 /*time*/, 0 /*xflags*/, 0x03 /*OS_CODE*/};
			write(f,gz_magic,10);
			size=zstrm.total_out;
			while (data.begin()!=data.end()){
				d=(*data.begin());
//				f.write(d,strlen(d));
				write(f,d,size<4096?size:4096);
				size-=size<4096?size:4096;
//				gzwrite(gzh,d,strlen(d));
				free(d);
				data.pop_front();
			}
			unsigned char a;
			a=crc&0xFF;write(f,&a,1);
			a=(crc>>8)&0xFF;write(f,&a,1);
			a=(crc>>16)&0xFF;write(f,&a,1);
			a=(crc>>24)&0xFF;write(f,&a,1);
			a=zstrm.total_in&0xFF;write(f,&a,1);
			a=(zstrm.total_in>>8)&0xFF;write(f,&a,1);
			a=(zstrm.total_in>>16)&0xFF;write(f,&a,1);
			a=(zstrm.total_in>>24)&0xFF;write(f,&a,1);
			close(f);
			deflateEnd(&zstrm);
//			gzclose(gzh);
			assert(size==0);
			delete [] savename;savename=NULL;
			return 0;
		}
		void purge(void){
			size=0;
			if (savename){
				delete [] savename;savename=NULL;
			}
			flushdata();
			while (data.begin()!=data.end()){
				free(*data.begin());
				data.pop_front();
			}
			deflateEnd(&zstrm);
		}
		void putf(const char * file,const char *func, int line, const char *buf,...) {
			if (!savename) return;
			char *fpbuf=inbuf;
			int l;
			va_list ap;


			struct timeval tv;
			gettimeofday(&tv,NULL);
			l=sprintf(fpbuf,"%li.%06li %s:%3i:%s\t",tv.tv_sec,tv.tv_usec,file,line,func);
//			data.push_back(fpbuf);
//			adddata(fpbuf,l);

			va_start(ap,buf);
			l+=vsprintf(fpbuf+l,buf,ap);
			va_end(ap);
			adddata(fpbuf,l);
		}
};
#endif //inmem

#if 0 //tofile
class c_debug_file {
	public:
		ulong size;
		char *savename;
		gzFile gzh;
		c_debug_file(){
			size=0;
			savename=NULL;
		}
		~c_debug_file(){
			purge();
		}
		void start(const char *name){
			savename=new char[strlen(name)+1];
			strcpy(savename,name);
			gzh=gzopen(savename,"w");
		}
		int save(void){
			gzclose(gzh);
			size=0;
			delete savename;savename=NULL;
			return 0;
		}
		void purge(void){
			size=0;
			if (savename){
				gzclose(gzh);
				unlink(savename);
				delete savename;savename=NULL;
			}
		}
		void putf(const char * file,const char *func, int line, const char *buf,...) {
			if (!savename) return;
			char *fpbuf;
			int l;
			va_list ap;

			struct timeval tv;
			gettimeofday(&tv,NULL);
			l=asprintf(&fpbuf,"%li.%06li %s:%3i:%s\t",tv.tv_sec,tv.tv_usec,file,line,func);
			gzwrite(gzh,fpbuf,l);
			size+=l;
			va_start(ap,buf);
			l=vasprintf(&fpbuf,buf,ap);
			va_end(ap);
			gzwrite(gzh,fpbuf,l);
			size+=l;
		}
};
#endif //tofile

#endif //FILE_DEBUG

#endif
