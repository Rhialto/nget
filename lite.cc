/*
    lite.* - ngetlite main files
    Copyright (C) 2000-2001  Matthew Mueller <donut@azstarnet.com>

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
#include <stdio.h>
#include <errno.h>
#include "log.h"
#include "lite.h"
#include "litenntp.h"
#include <string.h>
#include "strreps.h"

char *newstrcpy(char *&dest, const char *src){
	int l;
	if (dest) free(dest);
	l=strlen(src);
	dest=(char*)malloc(l+1);
	return strcpy(dest,src);
}

int curfd=-1;
c_prot_nntp nntp;

void showhelp(void){
	printf("ngetlite v0.15.1 - 'lite' nntp fetcher for nget\n"
			"Copyright 2000-2001 Matthew Mueller <donut@azstarnet.com>\n");
	printf("USAGE: ngetlite <listfiles ...>\n");
	printf("listfiles are generated with nget and the -w <listfile> param\n");
	exit (0);
}

char *Bfgets(char *buf,int len,FILE*f){
	char *r=fgets(buf,len,f);
	if (!r)return NULL;
	int l=strlen(buf);
	if (l>0 && (buf[l-1]=='\n' || buf[l-1]=='\r')){
		buf[--l]=0;
		if (l>0 && (buf[l-1]=='\n' || buf[l-1]=='\r'))
			buf[--l]=0;
	}
	return r;
}

void dofile(const char *arg){
	FILE *listf=fopen(arg,"r+");
	if (listf==NULL){
		printf("couldn't open %s\n",arg);
		return;
	}
	char buf[2048];
	printf("using litelist file: %s\n",arg);
	char *group=NULL;
	char *outfile=NULL;
	char *host=NULL;
	int tempi,i,partdone,retry,maxretry=20;
	long flagpos,temppos;
	ulong anum,lines,bytes;
#define FCHK(a) {if (a) {printf(__FILE__":%i ",__LINE__); goto dofile_ferror;}}
#define Lfgets(buf) {FCHK(Bfgets(buf,2048,listf)==NULL);}
	while (!feof(listf)){
		FCHK((flagpos=ftell(listf))<0);
		Lfgets(buf);
		if (strcmp(buf,"1")==0)
			partdone=1;
		else{
			if (strcmp(buf,"0")){
				printf("invalid litelist file\n");
				goto dofile_error;
				return;
			}

			partdone=0;
		}
		Lfgets(buf);
		newstrcpy(group,buf);
		Lfgets(buf);
		newstrcpy(outfile,buf);
		Lfgets(buf);
		tempi=atoi(buf);
		for (i=0;i<tempi;i++){
			Lfgets(buf);
			newstrcpy(host,buf);
			Lfgets(buf);
			anum=atol(buf);
			Lfgets(buf);
			bytes=atol(buf);
			Lfgets(buf);
			lines=atol(buf);
			if (partdone)
				continue;
			retry=0;
			while (retry<maxretry){
				try {
					nntp.doopen(host);
					nntp.dogroup(group);
					nntp.doarticle(anum,bytes,lines,outfile);
					partdone=1;
					FCHK((temppos=ftell(listf))<0);
					FCHK(fseek(listf,flagpos,SEEK_SET));
					FCHK(fputc('1',listf)<0);
					FCHK(fseek(listf,temppos,SEEK_SET));
					break;
				}catch(ApplicationExFatal &e){
					printCaughtEx_nnl(e);
					printf(" (fatal application error, exiting..)\n");
					exit(-1);
				}catch(ExFatal &e){
					printCaughtEx_nnl(e);
					printf(" (skipping)\n");
					break;
				}catch(ExError &e){
				//}catch(baseEx &e){//doesn't work..?
					printCaughtEx_nnl(e);
					retry++;
					printf(" (retrying %i)\n",retry);
					sleep(1);
					continue;
				}
			}
		}
	}
	goto dofile_done;

dofile_ferror:
	printf("file error %i: %s\n",errno,strerror(errno));
	goto dofile_error;

dofile_error:
dofile_done:
	if (listf)
		fclose(listf);
	safefree(group);
	return;
}

int main (int argc, char ** argv){
	char *timeout_str=getenv("NGETLITE_TIMEOUT");
	if (timeout_str)
		sock_timeout=atoi(timeout_str);
	
	quiet=0;debug=1;

	if (argc<2) showhelp();
	for (int i=1;i<argc;i++){
		dofile(argv[i]);
	}
}
