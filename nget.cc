/*
    nget - command line nntp client
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
#include <signal.h>
#include <unistd.h>
#ifdef HAVE_LIBPOPT
extern "C" {
#include <popt.h>
}
#else
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#endif
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "_sstream.h"

#include "misc.h"
#include "termstuff.h"
#include "strreps.h"
#include "sockstuff.h"
#include "prot_nntp.h"
#include "log.h"
#include "nget.h"
#include "datfile.h"
#include "myregex.h"


static int errorflags=0, warnflags=0, okflags=0;

void fatal_exit(void) {
	set_fatal_error_status();
	exit(errorflags);
}

#define SET_x_x_STATUS(type, low, up, bit) static const int type ## _ ## up = bit; \
static int low ## _ ## type;\
void set_ ## type ## _ ## low ## _status(int incr){\
	low ## _ ## type += incr;\
	if (incr>0)	low ## flags|=type ## _ ## up;\
}

#define SET_x_ERROR_STATUS(type,bit) SET_x_x_STATUS(type, error, ERROR, bit)
#define SET_x_WARN_STATUS(type,bit) SET_x_x_STATUS(type, warn, WARN, bit)
#define SET_x_OK_STATUS(type,bit) SET_x_x_STATUS(type, ok, OK, bit)
SET_x_ERROR_STATUS(decode, 1);
SET_x_ERROR_STATUS(path, 2);
SET_x_ERROR_STATUS(user, 4);
SET_x_ERROR_STATUS(retrieve, 8);
SET_x_ERROR_STATUS(other, 64);
SET_x_ERROR_STATUS(fatal, 128);
SET_x_OK_STATUS(total, 1);
SET_x_OK_STATUS(uu, 2);
SET_x_OK_STATUS(base64, 4);
SET_x_OK_STATUS(xx, 8);
SET_x_OK_STATUS(binhex, 16);
SET_x_OK_STATUS(plaintext, 32);
SET_x_OK_STATUS(qp, 64);
SET_x_OK_STATUS(yenc, 128);
SET_x_OK_STATUS(dupe, 256);
SET_x_OK_STATUS(unknown, 512);
SET_x_OK_STATUS(group, 1024);
SET_x_OK_STATUS(skipped, 2048);
SET_x_WARN_STATUS(retrieve,1);
SET_x_WARN_STATUS(undecoded,2);
SET_x_WARN_STATUS(unequal_line_count,8);
SET_x_WARN_STATUS(dupe, 256);
SET_x_WARN_STATUS(group,1024);
SET_x_WARN_STATUS(cache,2048);
#define print_x_x_STATUS(type, low) if (low ## _ ## type) printf("%s %i " #type, cf++?",":"", low ## _ ## type)
#define print_x_ERROR_STATUS(type) print_x_x_STATUS(type, error)
#define print_x_WARN_STATUS(type) print_x_x_STATUS(type, warn)
#define print_x_OK_STATUS(type) print_x_x_STATUS(type, ok)
void print_error_status(void){
	int pf=0;
	if (okflags){
		int cf=0;
		pf++;
		printf("OK:");
		print_x_OK_STATUS(total);
		print_x_OK_STATUS(yenc);
		print_x_OK_STATUS(uu);
		print_x_OK_STATUS(base64);
		print_x_OK_STATUS(xx);
		print_x_OK_STATUS(binhex);
		print_x_OK_STATUS(plaintext);
		print_x_OK_STATUS(qp);
		print_x_OK_STATUS(unknown);
		print_x_OK_STATUS(group);
		print_x_OK_STATUS(dupe);
		print_x_OK_STATUS(skipped);
	}
	if (warnflags){
		int cf=0;
		if (pf++) printf(" ");
		printf("WARNINGS:");
		print_x_WARN_STATUS(group);
		print_x_WARN_STATUS(retrieve);
		print_x_WARN_STATUS(undecoded);
		print_x_WARN_STATUS(unequal_line_count);
		print_x_WARN_STATUS(dupe);
		print_x_WARN_STATUS(cache);
	}
	if (errorflags){
		int cf=0;
		if (pf++) printf(" ");
		printf("ERRORS:");
		print_x_ERROR_STATUS(decode);
		print_x_ERROR_STATUS(path);
		print_x_ERROR_STATUS(user);
		print_x_ERROR_STATUS(retrieve);
		print_x_ERROR_STATUS(other);
		print_x_ERROR_STATUS(fatal);
	}
	if (pf)
		printf("\n");
}


#define NUM_OPTIONS 31
#ifndef HAVE_LIBPOPT

#ifndef HAVE_GETOPT_LONG
struct option {
	const char *name;
	int has_arg;
	int *flag;
	int val;
};
#endif

static struct option long_options[NUM_OPTIONS+1];
//#define OPTIONS "-qh:g:G:r:R:p:@:Tt:s:l:iIkKcCd:DSLP:w:N?"
static string getopt_options="-";//lets generate this in addoption, to avoid forgeting to update a define. (like in v0.8, oops)
#define OPTIONS getopt_options.c_str()

#else //!HAVE_LIBPOPT

static struct poptOption optionsTable[NUM_OPTIONS+1];
#endif //HAVE_LIBPOPT

struct opt_help {
	int namelen;
	char *arg;
	char *desc;
};
static opt_help ohelp[NUM_OPTIONS+1];
static int olongestlen=0;

enum {
	OPT_TEST_MULTI=1,
	OPT_MIN_SHORTNAME
};

static void addoption(char *longo,int needarg,char shorto,char *adesc,char *desc){
	static int cur=0;
	assert(cur<NUM_OPTIONS);
#ifdef HAVE_LIBPOPT
	optionsTable[cur].longName=longo;
	optionsTable[cur].shortName=(shorto>OPT_MIN_SHORTNAME)?shorto:0;
	optionsTable[cur].argInfo=needarg?POPT_ARG_STRING:POPT_ARG_NONE;
	optionsTable[cur].val=shorto;
	optionsTable[cur].arg=NULL;
#else //HAVE_LIBPOPT
	long_options[cur].name=longo;
	long_options[cur].has_arg=needarg;
	long_options[cur].flag=0;
	long_options[cur].val=shorto;
	if (shorto>OPT_MIN_SHORTNAME){
		getopt_options+=shorto;
		if (needarg)
			getopt_options+=':';
	}
#endif //!HAVE_LIBPOPT
	ohelp[cur].namelen=longo?strlen(longo):0;
	ohelp[cur].arg=adesc;
	ohelp[cur].desc=desc;
	int l=(adesc?strlen(adesc):0)+ohelp[cur].namelen+2;
	if (l>olongestlen)
		olongestlen=l;
	cur++;
}
static void addoptions(void)
{
	addoption("quiet",0,'q',0,"supress extra info");
	addoption("host",1,'h',"HOSTALIAS","force nntp host to use (must be configured in .ngetrc)");
	addoption("group",1,'g',"GROUPNAME","newsgroup");
	addoption("quickgroup",1,'G',"GROUPNAME","use group without checking for new headers");
	addoption("flushserver",1,'F',"HOSTALIAS","flush all headers for server from current group");
	addoption("expretrieve",1,'R',"EXPRESSION","retrieve files matching expression(see man page)");
	addoption("retrieve",1,'r',"REGEX","retrieve files matching regex");
	addoption("list",1,'@',"LISTFILE","read commands from listfile");
	addoption("path",1,'p',"DIRECTORY","path to store subsequent retrieves");
	addoption("makedirs",1,'m',"no,yes,ask","make dirs specified by -p and -P");
//	addoption("mark",1,'m',"MARKNAME","name of high water mark to test files against");
//	addoption("testretrieve",1,'R',"REGEX","test what would have been retrieved");
	addoption("testmode",0,'T',0,"test what would have been retrieved");
	addoption("test-multiserver",1,OPT_TEST_MULTI,"OPT","make testmode display per-server completion info (no(default)/long/short)");
	addoption("tries",1,'t',"INT","set max retries (-1 unlimits, default 20)");
	addoption("delay",1,'s',"INT","seconds to wait between retry attempts(default 1)");
	addoption("limit",1,'l',"INT","min # of lines a 'file' must have(default 0)");
	addoption("maxlines",1,'L',"INT","max # of lines a 'file' must have(default -1)");
	addoption("incomplete",0,'i',0,"retrieve files with missing parts");
	addoption("complete",0,'I',0,"retrieve only files with all parts(default)");
	addoption("keep",0,'k',0,"keep temp files");
	addoption("keep2",0,'K',0,"keep temp files and don't even try to decode them");
	addoption("case",0,'c',0,"match casesensitively");
	addoption("nocase",0,'C',0,"match incasesensitively(default)");
	addoption("dupecheck",1,'d',"FLAGS","check to make sure you haven't already downloaded files(default -dfiM)");
	addoption("nodupecheck",0,'D',0,"don't check if you already have files(shortcut for -dFIM)");
	addoption("mark",0,'M',0,"mark matching articles as retrieved");
	addoption("unmark",0,'U',0,"mark matching articles as not retrieved (implies -dI)");
	addoption("temppath",1,'P',"DIRECTORY","path to store tempfiles");
	addoption("writelite",1,'w',"LITEFILE","write out a ngetlite list file");
	addoption("noconnect",0,'N',0,"don't connect, only try to decode what we have");
	addoption("help",0,'?',0,"this help");
	addoption(NULL,0,0,NULL,NULL);
};
static void print_help(void){
	printf("nget v0.18.2 - nntp command line fetcher\n");
	printf("Copyright 1999-2002 Matthew Mueller <donut@azstarnet.com>\n");
	printf("\n\
This program is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation; either version 2 of the License, or\n\
(at your option) any later version.\n\n\
This program is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
GNU General Public License for more details.\n\n\
You should have received a copy of the GNU General Public License\n\
along with this program; if not, write to the Free Software\n\
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n");
//      printf("\nUSAGE: nget [-q] [-h host] [-g group] [-r subject regex] ?\n\n"
//	       "-q	quiet mode.  Use twice for total silence\n"
//);
	printf("\nUSAGE: nget -g group [-r file [-r file] [-g group [-r ...]]]\n");
	//this is kinda ugly, but older versions of popt don't have the poptPrintHelp stuff, and it seemed to print out a bit of garbage for me anyway...
	for (int i=0;
#ifdef HAVE_LIBPOPT
			optionsTable[i].longName;
#else
			long_options[i].name;
#endif
			i++){
		if(
#ifdef HAVE_LIBPOPT
				optionsTable[i].shortName
#else
				long_options[i].val
#endif
				>=OPT_MIN_SHORTNAME)
			printf("-%c  ",
#ifdef HAVE_LIBPOPT
				optionsTable[i].shortName
#else
				long_options[i].val
#endif
			);
		else
			printf("    ");
		if (
#ifdef HAVE_LIBPOPT
				optionsTable[i].argInfo!=POPT_ARG_NONE
#else
				long_options[i].has_arg
#endif
		)
			printf("--%s=%-*s",
#ifdef HAVE_LIBPOPT
					optionsTable[i].longName,
#else
					long_options[i].name,
#endif
					olongestlen-ohelp[i].namelen-1,ohelp[i].arg);
		else
			printf("--%-*s",
					olongestlen,
#ifdef HAVE_LIBPOPT
					optionsTable[i].longName
#else
					long_options[i].name
#endif
			);
		printf("  %s\n",ohelp[i].desc);
	}
#ifndef HAVE_LIBPOPT
#ifndef HAVE_GETOPT_LONG
	printf("Note: long options not supported by this compile\n");
#endif
#endif //!HAVE_LIBPOPT
	//cache_dbginfo();
//	getchar();
}


c_prot_nntp nntp;
SockPool sockpool;

void nget_cleanup(void) {
	try {
		nntp.cleanup();
		sockpool.expire_connections(true);
	}catch(baseEx &e){
		printCaughtEx(e);
	}catch(exception &e){
		printf("nget_cleanup: caught std exception %s\n",e.what());
	}catch(...){
		printf("nget_cleanup: caught unknown exception\n");
	}
}

static void term_handler(int s){
	printf("\nterm_handler: signal %i, shutting down.\n",s);
	nget_cleanup();
	exit(errorflags);
}


string nghome;
string ngcachehome;

//c_server_list servers;
//c_group_info_list groups;
//c_server_priority_grouping_list priogroups;

c_nget_config nconfig;

void nget_options::do_get_path(string &s){char *p;goodgetcwd(&p);s=p;free(p);}

nget_options::nget_options(void){
	do_get_path(startpath);
	get_path();
	get_temppath();
}
nget_options::nget_options(nget_options &o):maxretry(o.maxretry),retrydelay(o.retrydelay),linelimit(o.linelimit),maxlinelimit(o.maxlinelimit),gflags(o.gflags),badskip(o.badskip),qstatus(o.qstatus),test_multi(o.test_multi),retr_show_multi(o.retr_show_multi),makedirs(o.makedirs),group(o.group),host(o.host)/*,user(o.user),pass(o.pass)*/,path(o.path),startpath(o.path),temppath(o.temppath),writelite(o.writelite){
	/*	if (o.path){
			path=new char[strlen(o.path)+1];
			strcpy(path,o.path);
		}else
			path=NULL;*/
	//printf("copy path=%s(%p-%p)\n",path,this,path);
}
//	void del_path(void){if (path){/*printf("deleteing %s(%p-%p)\n",path,this,path);*/free(path);path=NULL;}}
	//void get_path(void){/*printf("get_path\n");*/del_path();goodgetcwd(&path);}
void nget_options::get_path(void){do_get_path(path);}
void nget_options::get_temppath(void){
	do_get_path(temppath);
	if (temppath[temppath.size()-1]!='/')
		temppath.append("/");
}
void nget_options::parse_dupe_flags(const char *opt){
	while(*opt){
		switch (*opt){
			case 'i':gflags&= ~GETFILES_NODUPEIDCHECK;break;
			case 'I':gflags|= GETFILES_NODUPEIDCHECK;break;
			case 'f':gflags&= ~GETFILES_NODUPEFILECHECK;break;
			case 'F':gflags|= GETFILES_NODUPEFILECHECK;break;
			case 'm':gflags|= GETFILES_DUPEFILEMARK;break;
			case 'M':gflags&= ~GETFILES_DUPEFILEMARK;break;
			default:
				//throw new c_error(EX_U_FATAL,"unknown dupe flag %c",*opt);
				throw UserExFatal(Ex_INIT,"unknown dupe flag %c",*opt);
		}
		opt++;
	}
}
int nget_options::set_test_multi(const char *s){
	if (!s) {
		//printf("set_makedirs s=NULL\n");
		return 0;
	}
	if (strcasecmp(s,"short")==0)
		test_multi=SHOW_MULTI_SHORT;
	else if (strcasecmp(s,"long")==0)
		test_multi=SHOW_MULTI_LONG;
	else if (strcasecmp(s,"no")==0)
		test_multi=NO_SHOW_MULTI;
	else{
		printf("set_test_multi invalid option %s\n",s);
		return 0;
	}
	return 1;
}
int nget_options::set_makedirs(const char *s){
	if (!s) {
		//printf("set_makedirs s=NULL\n");
		return 0;
	}
	if (strcasecmp(s,"yes")==0)
		makedirs=1;
	else if (strcasecmp(s,"ask")==0)
		makedirs=2;
	else if (strcasecmp(s,"no")==0)
		makedirs=0;
	else{
		printf("set_makedirs invalid option %s\n",s);
		return 0;
	}
	return 1;
}
//	~nget_options(){del_path();}

int makedirs(const char *dir, int mode){
	//eventually, maybe, this shall be made recursive. for now only the leaf can be made.
	return mkdir(dir,mode);
}
int maybe_mkdir_chdir(const char *dir, char makedir){
	if (chdir(dir)){
		if (errno==ENOENT){
			bool doit=0;
			if (makedir==2){
				char buf[40],*p;
				if (dir[0]!='/'){
					goodgetcwd(&p);
					printf("in %s, ",p);
					free(p);
				}
				printf("do you want to make dir %s?",dir);fflush(stdout);
				while (1){
					if (fgets(buf,39,stdin)){
						if ((p=strpbrk(buf,"\r\n"))) *p=0;
						if (strcasecmp(buf,"yes")==0 || strcasecmp(buf,"y")==0){
							doit=1;break;
						}
						if (strcasecmp(buf,"no")==0 || strcasecmp(buf,"n")==0){
							break;
						}
						printf("%s?? enter y[es], or n[o].\n",buf);
					}else{
						perror("fgets");
						return -1;
					}
				}
			}else if(makedir==1)
				doit=1;
			if (doit){
				if (makedirs(dir,PUBXMODE)){
					perror("mkdir");
					return -1;
				}
				if (chdir(dir)){
					perror("chdir");
					return -1;
				}
				return 0;
			}
		}
		return -1;
	}
	return 0;
}
struct s_arglist {
	int argc;
	char **argv;
};
static int do_args(int argc, char **argv,nget_options options,int sub){
	int c=0;
#ifdef HAVE_LIBPOPT
	poptContext optCon;
#else
#ifdef HAVE_GETOPT_LONG
	int opt_idx;
#endif
#endif
	int redo=0,redone=0,mustredo_on_skip=0;
	const char * loptarg=NULL;
	//printf("limit:%i tries:%i case:%i complete:%i dupcheck:%i\n",linelimit,maxretry,gflags&GETFILES_CASESENSITIVE,!(gflags&GETFILES_GETINCOMPLETE),!(gflags&GETFILES_NODUPECHECK));

#ifdef HAVE_LIBPOPT
	optCon = poptGetContext(NULL, argc, POPT_ARGV_T argv, optionsTable, sub?POPT_CONTEXT_KEEP_FIRST:0);
#endif
	while (1){
		if (redo){
			redo=0;
		}else {
//			if ((
			c=
#ifdef HAVE_LIBPOPT
						poptGetNextOpt(optCon)
#else //HAVE_LIBPOPT
#ifdef HAVE_GETOPT_LONG
						getopt_long(argc,argv,OPTIONS,long_options,&opt_idx)
#else
						getopt(argc,argv,OPTIONS)
#endif
#endif //!HAVE_LIBPOPT
						;
//				)==-1)
//				c=-12345;
#ifdef HAVE_LIBPOPT
			loptarg=poptGetOptArg(optCon);
#else
			loptarg=optarg;
#endif //HAVE_LIBPOPT
			//					break;
			redone=0;
		}
//		printf("arg:%c(%i)=%s(%p)\n",isprint(c)?c:'.',c,loptarg,loptarg);
		try {
			switch (c){
				//					case 'R':
				//						if(!group)
				//							perror("no group specified\n");
				//						nntp.nntp_queueretrieve(loptarg,linelimit,0,gflags);
				//						break;
				case 'T':
					options.gflags|=GETFILES_TESTMODE;
					PDEBUG(DEBUG_MIN,"testmode now %i",options.gflags&GETFILES_TESTMODE > 0);
					break;
				case OPT_TEST_MULTI:
					options.set_test_multi(loptarg);
					break;
				case 'M':
					options.gflags|= GETFILES_MARK;
					options.gflags&= ~GETFILES_UNMARK;
					break;
				case 'U':
					options.gflags|= GETFILES_UNMARK;
					options.gflags&= ~GETFILES_MARK;
					options.parse_dupe_flags("I");
					break;
				//case 1:
				case 'R':
					if (!options.badskip){
						if(options.group.isnull()){
							printf("no group specified\n");
							set_user_error_status();
						}else{
							generic_pred *p=make_pred(loptarg);
							if (p){
								nntp.filec=nntp.gcache->getfiles(options.path,options.temppath,nntp.filec,nntp.midinfo,p,options.gflags);
								delete p;
								options.qstatus=1;
							}
						}
					}
					break;
				case 'r':
					if (!options.badskip){
						if(options.group.isnull()){
							printf("no group specified\n");
							set_user_error_status();
						}else{
							/*c_regex_nosub *reg=new c_regex_nosub(loptarg,REG_EXTENDED + ((gflags&GETFILES_CASESENSITIVE)?0:REG_ICASE));
							  if (!reg)
							  throw new c_error(EX_A_FATAL,"couldn't allocate regex");
							  if (reg->geterror()){
							  char buf[256];
							  int e=reg->geterror();
							  reg->strerror(buf,256);
							  delete reg;
							//throw new c_error(EX_A_FATAL,"regex error %i:%s",e,buf);
							printf("regex error %i:%s\n",e,buf);break;
							}
							nntp_pred *p=ecompose2(new e_land<bool>,
							new e_binder2nd<e_nntp_file_lines<e_ge<ulong> > >(linelimit),
							new e_binder2nd_p<e_nntp_file_subject<e_eq<string,c_regex_nosub*> > >(reg));

							nntp.filec=nntp.gcache->getfiles(nntp.filec,nntp.grange,p,gflags);
							delete p;
							if (!nntp.filec)
							printf("nntp_retrieve: no match for %s\n",loptarg);
							//									throw new c_error(EX_U_WARN,"nntp_retrieve: no match for %s",match);
							//								nntp.nntp_queueretrieve(loptarg,linelimit,gflags);
							qstatus=1;*/
							ostringstream s; 
							s << "subject \"" << loptarg << "\" =~";
							if (options.linelimit > 0)
								s << " lines " << options.linelimit << " >= &&" ;
							if (options.maxlinelimit < ULONG_MAX)
								s << " lines " << options.maxlinelimit << " <= &&" ;
							generic_pred *p=make_pred(s.str().c_str());
							if (p){
								nntp.filec=nntp.gcache->getfiles(options.path,options.temppath,nntp.filec,nntp.midinfo,p,options.gflags);
								delete p;
								options.qstatus=1;
							}
						}
					}
					break;
				case 'i':
					options.gflags|= GETFILES_GETINCOMPLETE;
					break;
				case 'I':
					options.gflags&= ~GETFILES_GETINCOMPLETE;
					break;
				case 'k':
					options.gflags|= GETFILES_KEEPTEMP;
					break;
				case 'K':
					options.gflags|= GETFILES_NODECODE;
					break;
				case 'c':
					options.gflags|= GETFILES_CASESENSITIVE;
					break;
				case 'C':
					options.gflags&= ~GETFILES_CASESENSITIVE;
					break;
				case 'd':
					options.parse_dupe_flags(loptarg);
					break;
				case 'D':
					options.parse_dupe_flags("FIM");
					break;
				case 'N':
					options.gflags|= GETFILES_NOCONNECT;
					break;
				case 'q':
					quiet++;
					break;
				case 's':
					options.retrydelay=atoi(loptarg);
					if (options.retrydelay<0)
						options.retrydelay=1;
					printf("retry delay set to %i\n",options.retrydelay);
					break;
				case 't':
					options.maxretry=atoi(loptarg);
					if (options.maxretry==-1)
						options.maxretry=INT_MAX-1;
					printf("max retries set to %i\n",options.maxretry);
					break;
				case 'L':
					options.maxlinelimit=atoul(loptarg);
					printf("maximum line limit set to %lu\n",options.maxlinelimit);
					break;
				case 'l':
					options.linelimit=atoul(loptarg);
					printf("minimum line limit set to %lu\n",options.linelimit);
					break;
				case 'w':
					options.writelite=loptarg;
					printf("writelite to %s\n",options.writelite.c_str());
					break;
				case 'm':
					options.set_makedirs(loptarg);
					break;
				case 'P':
					if (!maybe_mkdir_chdir(loptarg,options.makedirs)){
						options.get_temppath();
						printf("temppath:%s\n",options.temppath.c_str());
						if (chdir(options.startpath.c_str())){
							printf("could not change to startpath: %s\n",options.startpath.c_str());
							set_path_error_status();
							return -1;
						}
					}else{
						printf("could not change temppath to %s\n",loptarg);
						set_path_error_status();
						return -1;
					}
					break;
				case 'p':
					if (!maybe_mkdir_chdir(loptarg,options.makedirs)){
						options.get_path();
						options.get_temppath();
						printf("(temp)path:%s\n",options.path.c_str());
						if (chdir(options.startpath.c_str())){
							printf("could not change to startpath: %s\n",options.startpath.c_str());
							set_path_error_status();
							return -1;
						}
					}else{
						printf("could not change to %s\n",loptarg);
						set_path_error_status();
						return -1;
					}
					break;
				case 'F':
					{
						c_server::ptr server=nconfig.getserver(loptarg);
						if (!server) {printf("no such server %s\n",loptarg);set_user_error_status();break;}
						c_nntp_server_info* servinfo=nntp.gcache->getserverinfo(server->serverid);

						nntp.gcache->flushlow(servinfo,ULONG_MAX,nntp.midinfo);
						servinfo->reset();
						break;
					}
#ifdef HAVE_LIBPOPT
#define POPT_ERR_CASE(a) case a: printf("%s: %s\n",#a,poptBadOption(optCon,0)); print_help(); return 1;
				POPT_ERR_CASE(POPT_ERROR_NOARG);
				POPT_ERR_CASE(POPT_ERROR_BADOPT);
				POPT_ERR_CASE(POPT_ERROR_OPTSTOODEEP);
				POPT_ERR_CASE(POPT_ERROR_BADQUOTE);
				POPT_ERR_CASE(POPT_ERROR_ERRNO);
				POPT_ERR_CASE(POPT_ERROR_BADNUMBER);
				POPT_ERR_CASE(POPT_ERROR_OVERFLOW);
#endif
				case ':':
				case '?':
					print_help();
					return 1;
				default:
					if (options.qstatus){
						mustredo_on_skip = 1;
						if (!options.badskip) nntp.nntp_retrieve(options);
						options.qstatus=0;
						mustredo_on_skip = 0;
					}
					switch (c){
						case '@':
#ifdef HAVE_LIBPOPT
							{
								const char *paths[2]={(nghome + "lists/").c_str(),NULL};
								const char *filename=fsearchpath(loptarg,paths,FSEARCHPATH_ALLOWDIRS);
								if (!filename) {
									printf("error opening %s: %s(%i)\n",loptarg,strerror(errno),errno);errno=0;
									set_user_error_status();
									break;
								}
								list<s_arglist> arglist;
								s_arglist larg;
								int totargc=0;
								try {
									c_file_fd f(filename,O_RDONLY);
									f.initrbuf();
									int pr;
									while (f.bgets()>0){
										if(!(pr=poptParseArgvString(f.rbufp(),&larg.argc,POPT_ARGV_p_T &larg.argv))){
											arglist.push_back(larg);
											totargc+=larg.argc;
										}else {
											printf("poptParseArgvString:%i\n",pr);
											set_user_error_status();
										}
									}
									f.close();
								}catch (FileNOENTEx &e){
									printf("error opening %s: %s(%i)\n",filename?:loptarg,strerror(errno),errno);errno=0;
									set_user_error_status();
									break;
								}
								if (!arglist.empty()){
									int tc=0,ic=0;
									s_arglist targ;
									larg.argc=totargc;
									larg.argv=(char**)malloc((totargc+1)*sizeof(char**));
									list<s_arglist>::iterator it=arglist.begin();
									for (;it!=arglist.end();++it){
										targ=*it;
										for(ic=0;ic<targ.argc;ic++,tc++)
											larg.argv[tc]=targ.argv[ic];
									}

									do_args(larg.argc,larg.argv,options,1);
									if (options.host){//####here we reset the stuff that may have been screwed in our recursiveness.  Perhaps it should reset it before returning, or something.. but I guess this'll do for now, since its the only place its called recursively.
										nntp.nntp_open(options.host);
										if (!options.group.isnull())
											nntp.nntp_group(options.group,0,options);
									}
									if (!chdir(options.path.c_str())){
										printf("path:%s\n",options.path.c_str());
									}else{
										printf("could not change to %s\n",options.path.c_str());
										set_path_error_status();
										return -1;
									}
									for (it=arglist.begin();it!=arglist.end();++it)
										free((*it).argv);
									free(larg.argv);
								}
								if (filename!=loptarg)
									free(const_cast<char*>(filename));//const_cast away the warning
							}
#else
							printf("This option is only available when libpopt is used.\n");
#endif
							break;
						case 'g':
							if (options.badskip<2){
								options.group=nconfig.getgroup(loptarg);
								nntp.nntp_group(options.group,1,options);
								if (options.badskip)
									options.badskip=0;
							}
							break;
						case 'G':
							if (options.badskip<2){
								options.group=nconfig.getgroup(loptarg);
								nntp.nntp_group(options.group,0,options);
								if (options.badskip)
									options.badskip=0;
							}
							break;
						case 'h':{
								if (*loptarg){
									options.host=nconfig.getserver(loptarg);
									if (options.host.isnull()){
										options.badskip=2;
										printf("invalid host %s (must be configured in .ngetrc first)\n",loptarg);
										set_user_error_status();
									}
									else
										options.badskip=0;
								}else{
									options.host=NULL;
									options.badskip=0;
								}
								nntp.nntp_open(options.host);
							}
							break;
						case -1://end of args.
							//								if (!badskip){
							//									nntp.nntp_retrieve(!testmode);
							//								}
							return 0;
						default:
							print_help();
							return 1;
					}
			}
		}catch(PathExFatal &e){
			printCaughtEx(e);
			set_path_error_status();
			return -1;
		}catch(ApplicationExFatal &e){
			printCaughtEx_nnl(e);
			printf(" (fatal application error, exiting..)\n");
			set_fatal_error_status();
			exit(errorflags);
		}catch(TransportExFatal &e){
			printCaughtEx_nnl(e);
			printf(" (fatal, aborting..)\n");
			if (options.host)
				options.badskip=2;//only set badskip if we are forcing a single host, otherwise we could exclude other hosts that are working
			if (mustredo_on_skip) {
				redo=1;
				mustredo_on_skip=0;
				options.qstatus=0;
			}
			set_other_error_status();
		}catch(baseEx &e){
			printCaughtEx_nnl(e);
			if (e.isfatal()){
				printf(" (fatal, aborting..)\n");
				//else if (n==EX_U_FATAL)
				if (options.host)
					options.badskip=1;//only set badskip if we are forcing a single host, otherwise we could exclude other hosts that are working
				if (mustredo_on_skip) {
					redo=1;
					mustredo_on_skip=0;
					options.qstatus=0;
				}
				set_other_error_status();
			}
			else{
				if(redone<options.maxretry){
					redo=1;redone++;
					printf(" (trying again. %i)\n",redone);
					if (options.retrydelay)
						sleep(options.retrydelay);
				}else{
					set_other_error_status();//set "fatal" error if non-fatal error didn't succeed even after retrying.
					printf("\n");
					if (c==-1)
						return 0;//end of args.
					redone=0;
					if (mustredo_on_skip) {
						redo=1;
						mustredo_on_skip=0;
						options.qstatus=0;
					}
				}
			}
		}
	}
	//		printf("quiet=%i host=%s\n",quiet,nntp_server);
	//		for (int i=optind;i<argc;i++)
	//			switch (mode){
	//				default:
	//					printf("%s\n",argv[i]);
	//					nntp_get(nntp_server,group,argv[i]);
	//			}

#ifdef HAVE_LIBPOPT
	poptFreeContext(optCon);
#endif
}

string path_join(string a, string b) {
	char c=a[a.size()-1];
	if (c!='/')
		return a + '/' + b;
	return a + b;
}
string path_join(string a, string b, string c) {
	return path_join(path_join(a,b), c);
}

int main(int argc, char ** argv){
#ifdef HAVE_SETLINEBUF
	setlinebuf(stdout); //force stdout to be line buffered, useful if redirecting both stdout and err to a file, to keep them from getting out of sync.
#endif
	atexit(print_error_status);
	try {
		//	atexit(cache_dbginfo);
		init_my_timezone();
		addoptions();
		signal(SIGTERM,term_handler);
		signal(SIGHUP,term_handler);
		signal(SIGINT,term_handler);
		signal(SIGQUIT,term_handler);
		{
			char *home;
			home=getenv("NGETHOME");
			if (home) {
				nghome=path_join(home,"");
			} else {
				home=getenv("HOME");
				if (!home)
					throw ConfigExFatal(Ex_INIT,"HOME or NGETHOME environment var not set.");
				nghome = home;
				if (fexists(path_join(home,".nget4","").c_str()))
					nghome=path_join(home,".nget4","");
				else if (fexists(path_join(home,"_nget4","").c_str()))
					nghome=path_join(home,"_nget4","");
				else
					throw ConfigExFatal(Ex_INIT,"neither %s nor %s exist", path_join(home,".nget4","").c_str(), path_join(home,"_nget4","").c_str());
			}
		}
		ngcachehome = nghome;

		srand(time(NULL));
		if (argc<2){
			print_help();
		}
		else {
			nget_options options;
			options.makedirs=0;
			options.maxretry=20;
			options.retrydelay=1;
			options.badskip=0;
			options.linelimit=0;
			options.maxlinelimit=ULONG_MAX;
			options.gflags=0;
			options.test_multi=NO_SHOW_MULTI;
			options.retr_show_multi=SHOW_MULTI_LONG;//always display the multi-server info when retrieving, just because I think thats better
			options.qstatus=0;
			options.group=NULL;
			options.host=NULL;
			{
				c_data_file cfg;
				c_data_section *galias,*halias,*hpriority;
				string ngetrcfn = nghome + ".ngetrc";
				if (!fexists(ngetrcfn.c_str())) {
					ngetrcfn = nghome + "_ngetrc";
					if (!fexists(ngetrcfn.c_str()))
							throw ConfigExFatal(Ex_INIT,"neither %s nor %s exist", (nghome + ".ngetrc").c_str(), ngetrcfn.c_str());
				}
				cfg.setfilename(ngetrcfn.c_str());
				cfg.read();
				halias=cfg.data.getsection("halias");
				hpriority=cfg.data.getsection("hpriority");
				galias=cfg.data.getsection("galias");
				if (!halias)
					throw ConfigExFatal(Ex_INIT,"no halias section");
				nconfig.setlist(&cfg.data,halias,hpriority,galias);
				int t;
				/*			if (!halias || !(options.host=halias->getitema("default")))
							options.host=getenv("NNTPSERVER")?:"";
							nntp.nntp_open(options.host,NULL,NULL);*/
				/*			options.host=halias->getsection("default");
							nntp.nntp_open(options.host);*/
				cfg.data.getitemi("timeout",&sock_timeout);
				cfg.data.getitemi("debug",&debug);
				cfg.data.getitemi("quiet",&quiet);
				cfg.data.getitemul("limit",&options.linelimit);
				cfg.data.getitemi("tries",&options.maxretry);
				cfg.data.getitemi("delay",&options.retrydelay);
				if (!cfg.data.getitemi("case",&t) && t==1)
					options.gflags|= GETFILES_CASESENSITIVE;
				if (!cfg.data.getitemi("complete",&t) && t==0)
					options.gflags|= GETFILES_GETINCOMPLETE;
				if (!cfg.data.getitemi("dupeidcheck",&t) && t==0)
					options.gflags|= GETFILES_NODUPEIDCHECK;
				if (!cfg.data.getitemi("dupefilecheck",&t) && t==0)
					options.gflags|= GETFILES_NODUPEFILECHECK;
				if (!cfg.data.getitemi("dupefilemark",&t) && t==1)
					options.gflags|= GETFILES_DUPEFILEMARK;
				if (!cfg.data.getitemi("tempshortnames",&t) && t==1)
					options.gflags|= GETFILES_TEMPSHORTNAMES;
				options.set_test_multi(cfg.data.getitema("test_multiserver"));
				options.set_makedirs(cfg.data.getitema("makedirs"));
				
				cfg.data.getitems("cachedir",&ngcachehome);//.ngetrc setting overrides default
				char *cp=getenv("NGETCACHE");//environment var overrides .ngetrc
				if (cp)
					ngcachehome=cp;
				ngcachehome = path_join(ngcachehome, "");
				if (!fexists(ngcachehome.c_str()))
					throw ConfigExFatal(Ex_INIT,"cache dir %s does not exist", ngcachehome.c_str());
			}
			init_term_stuff();
			options.get_path();
			nntp.initready();
			do_args(argc,argv,options,0);
		}
	}catch(ConfigEx &e){
		set_fatal_error_status();
		printCaughtEx(e);
		PERROR("(see man nget for configuration info)");
	}catch(baseEx &e){
		set_fatal_error_status();
		printf("main():");printCaughtEx(e);
	}catch(exception &e){
		set_fatal_error_status();
		printf("caught std exception %s\n",e.what());
	}catch(...){
		set_fatal_error_status();
		printf("caught unknown exception\n");
	}
	nget_cleanup();

	return errorflags;
}

