/*
    file.* - file io classes
    Copyright (C) 1999-2004  Matthew Mueller <donut AT dakotacom.net>

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

#include "_fileconf.h"
#include "sockstuff.h"

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#include "log.h"

//DEFINE_EX_CLASSES(File, baseEx);
DEFINE_EX_SUBCLASS(FileEx, ApplicationExFatal, true);
DEFINE_EX_SUBCLASS(FileNOENTEx, FileEx, true);

#define THROW_OPEN_ERROR(fmt,args...) {if (errno==ENOENT) throw FileNOENTEx(Ex_INIT, fmt, ## args); else throw FileEx(Ex_INIT, fmt, ## args);}

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
		bool beof(void){return c_buffy::beof();}
		int bpeek(void){return c_buffy::bpeek();}
		int bgets(void){return c_buffy::bgets(cbuf);}
		int btoks(char tok, char **toks, int max, bool tokfinal=true){return c_buffy::btoks(cbuf,tok,toks,max,tokfinal);}
		c_file_buffy(c_file*f):fileptr(f) {}
		//~c_file_buffy();
};
class c_file {
  private:

	virtual ssize_t dowrite(const void *data,size_t len)=0;
	virtual ssize_t doread(void *data,size_t len)=0;
	virtual int doflush(void)=0;
	virtual int doclose(void)=0;
	virtual const char *dostrerror(void)=0;
	
  protected:
	c_file_buffy *rbuffer;

	string m_name;

  public:
	const char *name(void){return m_name.c_str();}
	char * rbufp(void){return rbuffer->cbufp();}

	c_file(const char *fname);
	virtual ~c_file();
	ssize_t putf(const char *buf,...)
		__attribute__ ((format (printf, 2, 3)));
	ssize_t vputf(const char *buf, va_list ap);
	ssize_t write(const void *data,size_t len);
	ssize_t read(void *data,size_t len);
	void readfull(void *data,size_t len);
	void read_le_u32(uint32_t *i){
		readfull(i, 4);
#ifdef WORDS_BIGENDIAN
		*i = SWAP32(*i);
#endif
	}
	void read_le_u64(uint64_t *i){
		readfull(i, 8);
#ifdef WORDS_BIGENDIAN
		*i = SWAP64(*i);
#endif
	}
	//buffered funcs: must call initrbuf first.
	int bgets(void){return rbuffer->bgets();}//buffered gets, should be faster than normal gets, definatly for tcp or gz. maybe not for stream.
	char *bgetsp(void){rbuffer->bgets(); return rbuffer->cbufp();}
	int btoks(char tok, char **toks, int max, bool tokfinal=true){return rbuffer->btoks(tok,toks,max,tokfinal);}
	int bpeek(void){return rbuffer->bpeek();}
	bool beof(void){return rbuffer->beof();}
	void initrbuf(void);
	void flush(int local=0);
	void close(void);
	int close_noEx(void); //same as close, but never throws an exception

	virtual int isopen(void) const = 0;
};


void copyfile(c_file *of, c_file *nf);


class c_file_fd : public c_file {
  private:
	int fd;

	virtual ssize_t dowrite(const void *buf,size_t len);
	virtual ssize_t doread(void *buf,size_t len);
	virtual int doflush(void);
	virtual int doclose(void);
	virtual const char *dostrerror(void);
  public:
	int seek(int offset, int whence);
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

	virtual ssize_t dowrite(const void *buf,size_t len);
	virtual ssize_t doread(void *buf,size_t len);
	virtual int doflush(void);
	virtual int doclose(void);
	virtual const char *dostrerror(void);
  public:
	virtual int isopen(void) const;
	c_file_stream(const char *name,const char * mode);
	~c_file_stream(){close_noEx();};
};
#endif

class c_file_tcp : public c_file {
  private:
	sock_t sock;

	virtual ssize_t dowrite(const void *buf,size_t len);
	virtual ssize_t doread(void *buf,size_t len);
	virtual int doflush(void);
	virtual int doclose(void);
	virtual const char *dostrerror(void);
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

	virtual ssize_t dowrite(const void *buf,size_t len);
	virtual ssize_t doread(void *buf,size_t len);
	virtual int doflush(void);
	virtual int doclose(void);
	virtual const char *dostrerror(void);
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
