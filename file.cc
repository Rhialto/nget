/*
    file.* - file io classes
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "file.h"
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "sockstuff.h"
#include "strreps.h"


void xxrename(const char *oldpath, const char *newpath) {
	if (rename(oldpath, newpath))
		throw FileEx(Ex_INIT, "rename %s > %s: %s(%i)\n",oldpath,newpath,strerror(errno),errno);
}

#define THROW_OPEN_ERROR(fmt,args...) {if (errno==ENOENT) throw FileNOENTEx(Ex_INIT, fmt, ## args); else throw FileEx(Ex_INIT, fmt, ## args);}

int c_file_buffy::bfill(uchar *b,int l){
	return fileptr->read(b,l);
}

c_file::c_file(const char *fname): m_name(fname){
//	ftype=-1;
//	fh=-1;sh=NULL;
//#ifdef HAVE_LIBZ
//	gzh=NULL;
//#endif
	rbuffer=NULL;
#ifdef FILE_DEBUG
	file_debug=NULL;
#endif
}

c_file::~c_file(){
//	close();//cant call it here, since doclose is already gone
	delete rbuffer;
}

ssize_t c_file::putf(const char *data,...){
	char *fpbuf;
	int i,l;
	va_list ap;

	va_start(ap,data);
	l=vasprintf(&fpbuf,data,ap);
	va_end(ap);

	//	if (wbuf.bufput(data,len)){
	//		dowrite(wbuf.buf,wbuf.bufused);
	//		dowrite(data,len);
	//		wbuf.empty();
	//	}
	//	return len;
	i=write(fpbuf,l);
	free(fpbuf);
	return i;
}
ssize_t c_file::read(void *data,size_t len){
	int i=doread(data,len);
	if (i<0) throw FileEx(Ex_INIT,"read %s (%s)", name(), strerror(errno));
	return i;
}
ssize_t c_file::write(const void *data,size_t len){
	size_t sent=0;
	ssize_t i;
	while (sent < len) {
		i=dowrite((char*)data+sent, len-sent);
		if (i <= 0) 
			throw FileEx(Ex_INIT,"write %s: %i!=%i (%s)", name(), sent, len, strerror(errno));
		sent += i;
	}
	assert(sent == len);
	return sent;
}

//int c_file::open(const char *name,const char * mode){
//	close();
//	return doopen(name,mode);
//}
void c_file::flush(int local){
//	int i=0;
//###########3 dowrite(buffers..)
	if (!local) {
		int r=doflush();
		if (r != 0) throw FileEx(Ex_INIT,"flush %s: %i (%s)", name(), r, strerror(errno));
	}
}
void c_file::close(void){
	if (isopen()){
		flush(1);
		if (doclose() != 0)
			throw FileEx(Ex_INIT,"close %s (%s)", name(), strerror(errno));
	}
//	resetrbuf();rbufstate=0;
	if (rbuffer)rbuffer->clearbuf();
}
int c_file::close_noEx(void){
	try {
		close();
		return 0;
	} catch (FileEx &e) {
		return -1;
	}
}
void c_file::initrbuf(void){
	if (!rbuffer){
		rbuffer=new c_file_buffy(this);
#ifdef FILE_DEBUG
		rbuffer->file_debug=file_debug;
#endif
	}
};
ssize_t c_file::bread(size_t len){
	return -1;//unsupported with c_buffy yet.
}

#ifndef NDEBUG
int c_file_testpipe::open(void){
	o=1;
	return 0;
}
int c_file_testpipe::doflush(void){
	return 0;
}
int c_file_testpipe::doclose(void){
	o=0;
	return 0;
}
inline int c_file_testpipe::isopen(void)const{
	return (o);
}
inline ssize_t c_file_testpipe::dowrite(const void *dat,size_t len){
	data.append((char*)dat,len);
	return len;
}
inline ssize_t c_file_testpipe::doread(void *dat,size_t len){
//	if (rand()%2==0){
//		int ol=len;
		len=rand()%len+1;
//		printf("from %i to %i\n",ol,len);
//	}
#ifdef TESTPIPE_STRING
	int i=data.copy((char*)dat,len,0);
#else
	int i=data.copy(0,len,(char*)dat);
#endif
	data.erase(0,len);
	return i;
}
inline char * c_file_testpipe::dogets(char *dat,size_t len){
	return NULL;
}
#endif

#ifdef HAVE_LIBZ
c_file_gz::c_file_gz(const char *name,const char * mode):c_file(name){
	if (!(gzh=gzopen(name,mode)))
		THROW_OPEN_ERROR("gzopen %s (%s)", name, strerror(errno));
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
inline char * c_file_gz::dogets(char *data,size_t len){
	return gzgets(gzh,data,len);
}
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

c_file_fd::c_file_fd(int dfd, const char *name):c_file(name){
	fd=::dup(dfd);
	if (fd<0)
		throw FileEx(Ex_INIT,"dup %s(%i) (%s)", name, dfd, strerror(errno));
}

c_file_fd::c_file_fd(const char *name,int flags, int mode):c_file(name){
	fd=::open(name,flags,mode);
	if (fd<0)
		THROW_OPEN_ERROR("open %s (%s)", name, strerror(errno));
		//throw FileEx(Ex_INIT,"open %s (%s)", name, strerror(errno));
}
int fopen2open(const char *mode){
	if (strncmp(mode,"r+",2)==0)
		return O_RDWR;
	if (mode[0]=='r')
		return O_RDONLY;
	if (strncmp(mode,"w+",2)==0)
		return O_RDWR | O_CREAT | O_TRUNC;
	if (mode[0]=='w')
		return O_WRONLY | O_CREAT | O_TRUNC;
	if (strncmp(mode,"a+",2)==0)
		return O_RDWR | O_CREAT | O_APPEND;
	if (mode[0]=='a')
		return O_WRONLY | O_CREAT | O_APPEND;
	assert(0);
	return 0;
}
c_file_fd::c_file_fd(const char *name,const char *mode):c_file(name){
	int flags=fopen2open(mode);
	fd=::open(name,flags,S_IRWXU|S_IRWXG|S_IRWXO);
	if (fd<0)
		THROW_OPEN_ERROR("open %s (%s)", name, strerror(errno));
}
int c_file_fd::doflush(void){
	if (fd>=0)
		return fsync(fd);
	return 0;
}
int c_file_fd::doclose(void){
	int i=0;
	i=::close(fd);
	fd=-1;
	return i;
}
inline int c_file_fd::isopen(void)const{
	return (fd>=0);
}
inline ssize_t c_file_fd::dowrite(const void *data,size_t len){
	return ::write(fd,(char*)data,len);
}
inline ssize_t c_file_fd::doread(void *data,size_t len){
	return ::read(fd,data,len);
}
inline char * c_file_fd::dogets(char *data,size_t len){
	int i=sock_gets(fd,data,len);//well, I guess it'll work for non sockets too.
	if (i<=0)return NULL;
	return data;
	//return fd_gets(fd,data,len);
}

#ifdef USE_FILE_STREAM
int c_file_stream::c_file_stream(const char *name,const char * mode):c_file(name){
	if (!(fs=fopen(name,mode)))
		THROW_OPEN_ERROR("fopen %s (%s)", name, strerror(errno));
}
int c_file_stream::doflush(void){
	if (fs)
		return fflush(fs);
	return 0;
}
int c_file_stream::doclose(void){
	int i=0;
	i=fclose(fs);
	fs=NULL;
	return i;
}
inline int c_file_stream::isopen(void)const{
	return (fs!=0);
}
inline ssize_t c_file_stream::dowrite(const void *data,size_t len){
	return fwrite(data,1,len,fs);
}
inline ssize_t c_file_stream::doread(void *data,size_t len){
	return fread(data,1,len,fs);
}
inline char * c_file_stream::dogets(char *data,size_t len){
	return fgets(data,len,fs);
}
#endif


c_file_tcp::c_file_tcp(const char *host,const char * port):c_file(host){
	if (rbuffer){
		rbuffer->cbuf.reserve(512);//ensure at least 512 bytes
		sock=make_connection(SOCK_STREAM,host,port,rbuffer->cbuf.data(),rbuffer->cbuf.capacity());
	}
	else{
		char buf[512];
		sock=make_connection(SOCK_STREAM,host,port,buf,512);
	}
	if (m_name.find(':')<0){
		m_name+=':';
		m_name+=port;
	}
	if (sock<0)
		throw FileEx(Ex_INIT,"open %s (%s)", name(), strerror(errno));
}
int c_file_tcp::doflush(void){
	if (sock>=0)
		return fsync(sock);
	return 0;
}
int c_file_tcp::doclose(void){
	int i=0;
	i=::close(sock);
	sock=-1;
	return i;
}
inline int c_file_tcp::isopen(void)const{
	return (sock>=0);
}
inline ssize_t c_file_tcp::dowrite(const void *data,size_t len){
	//don't need to use sock_write anymore since c_file::write handles the looping.
	return ::write(sock,(char*)data, len);
}
inline ssize_t c_file_tcp::doread(void *data,size_t len){
	return sock_read(sock,data,len);
}
inline char * c_file_tcp::dogets(char *data,size_t len){
	int i=sock_gets(sock,data,len);
	if (i<=0)return NULL;
	return data;
	//return sock_gets(sock,data,len);
}
bool c_file_tcp::datawaiting(void) const {
	return sock_datawaiting(sock);
}
