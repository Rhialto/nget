/*
    file.* - file io classes
    Copyright (C) 1999-2003  Matthew Mueller <donut@azstarnet.com>

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
#include "path.h"


void xxrename(const char *oldpath, const char *newpath) {
#ifdef WIN32
	//On windows rename will not replace an existing file, so check first and remove it.
	if (fexists(newpath)) {
		if (unlink(newpath))
			throw FileEx(Ex_INIT, "rename: unlink %s: %s(%i)\n",newpath,strerror(errno),errno);
	}
#endif
	if (rename(oldpath, newpath))
		throw FileEx(Ex_INIT, "rename %s > %s: %s(%i)\n",oldpath,newpath,strerror(errno),errno);
}

void copyfile(c_file *of, c_file *nf) {
	char buf[4096];
	int len;
	while ((len=of->read(buf, 4096)) != 0)
		nf->write(buf, len);
}

int c_file_buffy::bfill(uchar *b,int l){
	return fileptr->read(b,l);
}

c_file::c_file(const char *fname): m_name(fname){
	rbuffer=NULL;
}

c_file::~c_file(){
//	close();//cant call it here, since doclose is already gone
	delete rbuffer;
}

ssize_t c_file::vputf(const char *buf, va_list ap){
	char *fpbuf;
	int i,l;
	l=vasprintf(&fpbuf,buf,ap);
	try {
		i=write(fpbuf,l);
	} catch (...) {
		free(fpbuf);
		throw;
	}
	free(fpbuf);
	return i;
}
ssize_t c_file::putf(const char *data,...){
	va_list ap;
	va_start(ap,data);
	ssize_t r=vputf(data, ap);
	va_end(ap);
	return r;
}
ssize_t c_file::read(void *data,size_t len){
	ssize_t i=doread(data,len);
	if (i<0) throw FileEx(Ex_INIT,"read %s (%s)", name(), dostrerror());
	return i;
}
void c_file::readfull(void *data,size_t len){
	size_t cur=0;
	ssize_t i;
	while (cur < len) {
		i=doread((char*)data+cur, len-cur);
		if (i<0) throw FileEx(Ex_INIT,"readfull %s (%s)", name(), dostrerror());
		else if (i==0) throw FileEx(Ex_INIT,"readfull %s: unexpected EOF", name());
		cur += i;
	}
	assert(cur == len);
}
ssize_t c_file::write(const void *data,size_t len){
	size_t sent=0;
	ssize_t i;
	while (sent < len) {
		i=dowrite((char*)data+sent, len-sent);
		if (i <= 0) 
			throw FileEx(Ex_INIT,"write %s: %i!=%i (%s)", name(), sent, len, dostrerror());
		sent += i;
	}
	assert(sent == len);
	return sent;
}

void c_file::flush(int local){
//	int i=0;
//###########3 dowrite(buffers..)
	if (!local) {
		int r=doflush();
		if (r != 0) throw FileEx(Ex_INIT,"flush %s: %i (%s)", name(), r, dostrerror());
	}
}
void c_file::close(void){
	if (isopen()){
		flush(1);
		if (doclose() != 0)
			throw FileEx(Ex_INIT,"close %s (%s)", name(), dostrerror());
	}
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
	}
};


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
	fd=::open(name,flags,PUBMODE);
	if (fd<0)
		THROW_OPEN_ERROR("open %s (%s)", name, strerror(errno));
}
const char *c_file_fd::dostrerror(void) {
	return strerror(errno);
}
int c_file_fd::doflush(void){
#ifdef HAVE_FSYNC
	if (fd>=0)
		return fsync(fd);
#endif
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
int c_file_fd::seek(int offset, int whence){
	int r = lseek(fd, offset, whence);
	if (r<0)
		throw FileEx(Ex_INIT,"seek %s: %s", name(), dostrerror());
	return r;
}


#ifdef USE_FILE_STREAM
int c_file_stream::c_file_stream(const char *name,const char * mode):c_file(name){
	if (!(fs=fopen(name,mode)))
		THROW_OPEN_ERROR("fopen %s (%s)", name, strerror(errno));
}
const char c_file_stream::*dostrerror(void) {
	return strerror(errno);
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
#endif


c_file_tcp::c_file_tcp(const char *host,const char * port):c_file(host){
	if (m_name.find(':')<0){
		m_name+=':';
		m_name+=port;
	}
	try {
		if (rbuffer){
			rbuffer->cbuf.reserve(512);//ensure at least 512 bytes
			sock=make_connection(SOCK_STREAM,host,port,rbuffer->cbuf.data(),rbuffer->cbuf.capacity());
		}
		else{
			char buf[512];
			sock=make_connection(SOCK_STREAM,host,port,buf,512);
		}
	} catch (FileEx &e) {
		throw FileEx(Ex_INIT,"open %s (%s)", name(), e.getExStr());
	}
}
const char *c_file_tcp::dostrerror(void) {
	return sock_strerror(sock_errno);
}
int c_file_tcp::doflush(void){
#ifdef HAVE_FSYNC
	if (sock_isvalid(sock))
		return fsync(sock);
#endif
	return 0;
}
int c_file_tcp::doclose(void){
	int i=0;
	i=sock_close(sock);
	sock=SOCK_INVALID;
	return i;
}
inline int c_file_tcp::isopen(void)const{
	return sock_isvalid(sock);
}
inline ssize_t c_file_tcp::dowrite(const void *data,size_t len){
	//don't need to use sock_write_ensured since c_file::write handles the looping.
	return sock_write(sock,(char*)data, len);
}
inline ssize_t c_file_tcp::doread(void *data,size_t len){
	return sock_read(sock,data,len);
}
bool c_file_tcp::datawaiting(void) const {
	return sock_datawaiting(sock);
}
