/*
    file.* - file io classes
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
#ifndef _FILE_H_
#define _FILE_H_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
//#include <unistd.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#include "log.h"
#ifdef FILE_DEBUG
#define FILEDEBUG(a, args...) if (file_debug) file_debug->DEBUGLOGPUTF(a, ## args)
#define FILEDEBUGRBUF(a) if (file_debug) file_debug->DEBUGLOGPUTF("%u %u %u %u %i %i "a,rbufsize,rbufused,rbufofp,rbufoff,rbufmatch,rbuft)
#else
#define FILEDEBUG(a, args...)
#define FILEDEBUGRBUF(a)
#endif

//DEFINE_EX_CLASSES(File, baseEx);
DEFINE_EX_SUBCLASS(FileEx, ApplicationExFatal, true);
DEFINE_EX_SUBCLASS(FileNOENTEx, FileEx, true);

void xxrename(const char *oldpath, const char *newpath);

#include "buffy.h"

class c_file;
class c_file_buffy : public c_buffy{
	private:
		c_file *fileptr;
		virtual int bfill(uchar *b,int l);
	public:
		CharBuffer cbuf;
		char *cbufp(void){return cbuf.c_str();}
		int bgets(void){return c_buffy::bgets(cbuf);}
		c_file_buffy(c_file*f):fileptr(f) {}
		//~c_file_buffy();
};
class c_file {
//	unsigned long size;
  private:
//	c_buffer rbuf,wbuf;
//	int ftype;
//	int fh;
//	FILE *sh;
//#ifdef HAVE_LIBZ
//	gzFile gzh;
//#endif

	virtual char * dogets(char *data,size_t len)=0;
	virtual ssize_t dowrite(const void *data,size_t len)=0;
	virtual ssize_t doread(void *data,size_t len)=0;
//	virtual int doopen(const char *name,const char * mode)=0;
	virtual int doflush(void)=0;
	virtual int doclose(void)=0;
	
  protected:
#ifdef FILE_DEBUG
	c_debug_file *file_debug;
#endif
//	c_rbuffer *rbuffer;
	c_file_buffy *rbuffer;

	string m_name;

  public:
	const char *name(void){return m_name.c_str();}
	char * rbufp(void){return rbuffer->cbufp();}

	c_file(const char *fname);
	virtual ~c_file();
	ssize_t putf(const char *buf,...)
		__attribute__ ((format (printf, 2, 3)));
	char * gets(char *data,size_t len){return dogets(data,len);}
	ssize_t write(const void *data,size_t len);
	ssize_t read(void *data,size_t len);
	//buffered funcs: must call initrbuf first.
	ssize_t bread(size_t len);//buffered read, must be used instead of normal read, if you are using bgets
	//char * bgets(void);//buffered gets, should be faster than normal gets, definatly for tcp or gz. maybe not for stream.
	int bgets(void){return rbuffer->bgets();}//buffered gets, should be faster than normal gets, definatly for tcp or gz. maybe not for stream.
	void initrbuf(void);
//	int open(const char *name,const char * mode);
	void flush(int local=0);
	void close(void);
	int close_noEx(void); //same as close, but never throws an exception

	virtual int isopen(void) const = 0;
};

#ifndef NDEBUG
#define TESTPIPE_STRING
#ifdef TESTPIPE_STRING
#include <string>
#else
#include <rope.h>
#endif
class c_file_testpipe : public c_file {
  private:
#ifdef TESTPIPE_STRING
	string data;
#else
	crope data;
#endif
	int o;

	virtual char * dogets(char *data,size_t len);
	virtual ssize_t dowrite(const void *buf,size_t len);
	virtual ssize_t doread(void *buf,size_t len);
	virtual int doflush(void);
	virtual int doclose(void);
  public:
	virtual int isopen(void) const;
	int open(void);
	int datasize(void){return data.size();}
	c_file_testpipe(void):c_file("<testpipe>") {o=0;};
	~c_file_testpipe(){close_noEx();};
};
#endif

class c_file_fd : public c_file {
  private:
	int fd;

	virtual char * dogets(char *data,size_t len);
	virtual ssize_t dowrite(const void *buf,size_t len);
	virtual ssize_t doread(void *buf,size_t len);
	virtual int doflush(void);
	virtual int doclose(void);
  public:
	virtual int isopen(void) const;
	c_file_fd(const char *name,int flags,int mode=S_IRWXU|S_IRWXG|S_IRWXO);
	c_file_fd(const char *name,const char * mode);
	c_file_fd(int dfd, const char *name="");
	~c_file_fd(){close_noEx();};
};
#ifdef USE_FILE_STREAM
class c_file_stream : public c_file {
  private:
	FILE *fs;

	virtual char * dogets(char *data,size_t len);
	virtual ssize_t dowrite(const void *buf,size_t len);//fwrite doesn't seem to be able to notice out of disk errors. be warned.
	virtual ssize_t doread(void *buf,size_t len);
	virtual int doflush(void);
	virtual int doclose(void);
  public:
	virtual int isopen(void) const;
	c_file_stream(const char *name,const char * mode);
	~c_file_stream(){close_noEx();};
};
#endif
class c_file_tcp : public c_file {
  private:
	int sock;

	virtual char * dogets(char *data,size_t len);
	virtual ssize_t dowrite(const void *buf,size_t len);
	virtual ssize_t doread(void *buf,size_t len);
	virtual int doflush(void);
	virtual int doclose(void);
  public:
	bool datawaiting(void) const;
	virtual int isopen(void) const;
	c_file_tcp(const char *name,const char * port);
	~c_file_tcp(){close_noEx();};
};


#ifdef HAVE_LIBZ
class c_file_gz : public c_file {
  private:
	gzFile gzh;

	virtual char * dogets(char *data,size_t len);
	virtual ssize_t dowrite(const void *buf,size_t len);
	virtual ssize_t doread(void *buf,size_t len);
	virtual int doflush(void);
	virtual int doclose(void);
  public:
	virtual int isopen(void) const;
	c_file_gz(const char *host,const char * mode);
	~c_file_gz(){close_noEx();};
};
//class c_file_optgz : public c_file_gz, c_file_stream {
//  private:
//
//	virtual size_t dowrite(void *buf,size_t len);
//	virtual size_t doread(void *buf,size_t len);
//	virtual int doflush(void);
//	virtual int doclose(void);
//  public:
//	virtual int isopen(void) const;
//	int open(const char *name,const char * mode,int gz);
//};
//#else
//class c_file_optgz : public c_file_stream {
//  private:
//	int open(const char *name,const char * mode,int gz);
//};
#endif

#endif
