/*
    litenntp.* - ngetlite nntp protocol handler
    Copyright (C) 2000-2002  Matthew Mueller <donut@azstarnet.com>

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
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "lite.h"
#include "litenntp.h"
#include "log.h"
#include <stdio.h>
#include <unistd.h>
#include "strreps.h"
//needed for getpid on mingw32
#ifdef HAVE_PROCESS_H
#include <process.h>
#endif

int c_prot_nntp::putline(int echo,const char * str,...){
	va_list ap;
	va_start(ap,str);
	int i=doputline(echo,str,ap);
	va_end(ap);
	return i;
}
int c_prot_nntp::stdputline(int echo,const char * str,...){
	va_list ap;
	int i;
	va_start(ap,str);
	doputline(echo,str,ap);
	va_end(ap);
	i=getreply(echo);
	if (i==450 || i==480) {
		nntp_auth();
		va_start(ap,str);
		doputline(echo,str,ap);
		va_end(ap);
		i=getreply(echo);
	}
	return i;
}
int c_prot_nntp::doputline(int echo,const char * str,va_list ap){
	int i;
	try {
		i = cursock->vputf(str, ap) + cursock->write("\r\n",2);
	} catch (FileEx &e) {
		doclose();
		throw TransportExError(Ex_INIT,"nntp_putline: %s:%i: %s",e.getExFile(), e.getExLine(), e.getExStr());
	}
	if (echo){
		printf(">");
		vprintf(str,ap);
		printf("\n");
	}
//	time(&lasttime);
	return i;
}

int c_prot_nntp::getline(int echo){
	int i;
	try {
		i=cursock->bgets();
	} catch (FileEx &e) {
		doclose();
		throw TransportExError(Ex_INIT,"nntp_getline: %s:%i: %s",e.getExFile(), e.getExLine(), e.getExStr());
	}
	if (i==0) {
		doclose();
		throw TransportExError(Ex_INIT,"nntp_getline: connection closed unexpectedly");
	}
	cbuf = cursock->rbufp();
	if (echo){
		printf("%s\n",cbuf);
		fflush(stdout);
	}
	return i;
}

int c_prot_nntp::chkreply(int reply){
//	int i=getreply(echo);
	if (reply/100!=2)
		throw ProtocolExFatal(Ex_INIT,"bad reply %i: %s",reply,cbuf);
	return reply;
}

int c_prot_nntp::getreply(int echo){
	int code;
	if ((code=getline(echo))>=0)
		code=atoi(cbuf);
//	if (cbuf[3]=='-')
//		do{
//			ftp_getline(cbuf,cbuf_size);
//		}while((atoi(cbuf)!=code)||(cbuf[3]!=' '));
	return code;
}

#define tempfilename_base "ngetlite"

void c_prot_nntp::doarticle(ulong anum,ulong bytes,ulong lines,const char *outfile){
	chkreply(stdputline(debug>=DEBUG_MED,"ARTICLE %lu",anum));
	printf(".");fflush(stdout);
	ulong rbytes=0,rlines=0,hlines=0;
	time_t starttime,donetime;
	time(&starttime);
	int header=1;
	long glr;
	char *lp;
	char tempfilename[100];
	sprintf(tempfilename,"%s.%i",tempfilename_base,
#ifdef HAVE_GETPID
			getpid()
#else
			rand()
#endif
			);
	c_file_fd f(tempfilename,"w");
	while(1) {
		glr=getline(debug>=DEBUG_ALL);
		if (cbuf[0]=='.'){
			if(cbuf[1]==0)
				break;
			lp=cbuf+1;
		}else
			lp=cbuf;
		rlines++;
		rbytes+=glr;
		if (header && lp[0]==0){
			header=0;
			hlines=rlines;
			rlines=0;
		}
		f.putf("%s\n",lp);
	}
	f.close();
	xxrename(tempfilename,outfile);
	time(&donetime);
	if (!quiet){
		long d=donetime-starttime;
		printf("got article %lu in %li sec, %li B/s (%lu/%lu lines, %lu/%lu bytes, %s)",anum,d,d?rbytes/(d):0,rlines,lines,rbytes,bytes,outfile);
		if (rlines!=lines)printf(" Warning! lines not equal to expected!");
		//if (rbytes!=bytes)printf(" bne!");
		if ((rbytes > bytes + 3) || (rbytes + 3 < bytes)) printf(" bne!");
		printf("\n");
		fflush(stdout);
	}
}

void c_prot_nntp::dogroup(const char *group){
	if (curgroup && strcmp(group,curgroup)==0)
		return;
	chkreply(stdputline(!quiet,"GROUP %s",group));
	newstrcpy(curgroup,group);
}
void c_prot_nntp::doclose(void){
	if (cursock.get())
		try {
			cursock->close();
		} catch (FileEx &e) {//ignore transport errors while closing
			printCaughtEx_nnl(e);printf(" (ignored)\n");
		}
	safefree(curhost);
	safefree(curgroup);
	safefree(curuser);
	safefree(curpass);
}
void c_prot_nntp::doopen(const char *host, const char *user, const char *pass){
	if (cursock.get() && cursock->isopen() && curhost && strcmp(host,curhost)==0)
		return;

	doclose();
	try {
		cursock.reset(new c_file_tcp(host,"nntp"));
		cursock->initrbuf();
	} catch (FileEx &e) {
		throw TransportExError(Ex_INIT,"nntp_doopen: %s",e.getExStr());
	}
	chkreply(getreply(!quiet));
	putline(debug>=DEBUG_MED,"MODE READER");
	getline(debug>=DEBUG_MED);
	newstrcpy(curhost,host);
	newstrcpy(curuser,user);
	newstrcpy(curpass,pass);
}

void c_prot_nntp::nntp_auth(void){
	nntp_doauth(curuser,curpass);
}
void c_prot_nntp::nntp_doauth(const char *user, const char *pass){
	int i;

	if(!user || !*user){
		throw TransportExFatal(Ex_INIT,"nntp_doauth: no authorization info known");
	}
	putline(quiet<2,"AUTHINFO USER %s",user);
	i=getreply(quiet<2);
	if (i==350 || i==381){
		if(!pass || !*pass){
			throw TransportExFatal(Ex_INIT,"nntp_doauth: no password known");
		}
		if (quiet<2)
			printf(">AUTHINFO PASS *\n");
		putline(0,"AUTHINFO PASS %s",pass);
		i=getreply(quiet<2);
	}
	chkreply(i);
}

c_prot_nntp::c_prot_nntp(){
	curhost=NULL;
	curgroup=NULL;
	curuser=NULL;
	curpass=NULL;
}
c_prot_nntp::~c_prot_nntp(){
	doclose();
}
