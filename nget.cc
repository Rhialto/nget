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
#include "argparser.h" //only needed for with-popt build
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
#include "_fileconf.h"

#include "path.h"
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
SET_x_ERROR_STATUS(group, 16);
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
		print_x_ERROR_STATUS(group);
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
static string getopt_options="-";//lets generate this in addoption, to avoid forgeting to update a define. (like in v0.8, oops)
#define OPTIONS getopt_options.c_str()

#else //!HAVE_LIBPOPT

class c_poptContext {
	protected:
		poptContext optCon;
	public:
		int GetNextOpt(void) {return poptGetNextOpt(optCon);}
		const char * GetOptArg(void) {return poptGetOptArg(optCon);}
		const char * const * GetArgs(void) {return poptGetArgs(optCon);}
		const char * BadOption(int flags) {return poptBadOption(optCon, flags);}
		c_poptContext(const char *name, int argc, const char **argv, const struct poptOption *options, int flags) {
			optCon = poptGetContext(POPT_NAME_T name, argc, POPT_ARGV_T argv, options, flags);
		}
		~c_poptContext() {
			poptFreeContext(optCon);
		}
};

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
	OPT_TEST_MULTI=2,
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
	printf("nget v"PACKAGE_VERSION" - nntp command line fetcher\n");
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

#define BAD_HOST 1
#define BAD_PATH 2
#define BAD_TEMPPATH 4
void nget_options::do_get_path(string &s){char *p;goodgetcwd(&p);s=p;free(p);}

nget_options::nget_options(void){
	do_get_path(startpath);
	get_path();
	get_temppath();
}
nget_options::nget_options(nget_options &o):maxretry(o.maxretry),retrydelay(o.retrydelay),linelimit(o.linelimit),maxlinelimit(o.maxlinelimit),gflags(o.gflags),badskip(o.badskip),test_multi(o.test_multi),retr_show_multi(o.retr_show_multi),makedirs(o.makedirs),group(o.group),host(o.host)/*,user(o.user),pass(o.pass)*/,path(o.path),startpath(o.path),temppath(o.temppath),writelite(o.writelite){
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
	path_append(temppath,"");//ensure temppath ends with seperator
	assert(is_pathsep(temppath[temppath.size()-1]));
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
				if (!is_abspath(dir)){
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
struct s_argv {
	int argc;
	const char **argv;
};
static int do_args(int argc, const char **argv,nget_options options,int sub){
	int c=0;
	const char * loptarg=NULL;
	t_nntp_getinfo_list getinfos;

#ifdef HAVE_LIBPOPT
#ifndef POPT_CONTEXT_ARG_OPTS
#define POPT_CONTEXT_ARG_OPTS 0
#endif
	c_poptContext optCon(NULL, argc, argv, optionsTable, (sub?POPT_CONTEXT_KEEP_FIRST:0) | POPT_CONTEXT_ARG_OPTS);
#else
#ifdef HAVE_GETOPT_LONG
	int opt_idx;
#endif
#endif
	while (1){
		c=
#ifdef HAVE_LIBPOPT
					optCon.GetNextOpt()
#else //HAVE_LIBPOPT
#ifdef HAVE_GETOPT_LONG
					getopt_long(argc,GETOPT_ARGV_T argv,OPTIONS,long_options,&opt_idx)
#else
					getopt(argc,GETOPT_ARGV_T argv,OPTIONS)
#endif
#endif //!HAVE_LIBPOPT
					;
#ifdef HAVE_LIBPOPT
		loptarg=optCon.GetOptArg();
#else
		loptarg=optarg;
#endif //HAVE_LIBPOPT
//		printf("arg:%c(%i)=%s(%p)\n",isprint(c)?c:'.',c,loptarg,loptarg);
#if (HAVE_LIBPOPT && !POPT_CONTEXT_ARG_OPTS)
		if (optCon.GetArgs()) { //hack for older version of popt without POPT_CONTEXT_ARG_OPTS
			c = 0;
			loptarg = *optCon.GetArgs();
		}
#endif
		switch (c){
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
			case 'R':
				if (!options.badskip){
					if(options.group.isnull()){
						printf("no group specified\n");
						set_user_error_status();
					}else{
						try {
							nntp_file_pred *p=make_pred(loptarg, options.gflags);
							getinfos.push_back(new c_nntp_getinfo(options.path, options.temppath, p, options.gflags));
						}catch(RegexEx &e){
							printCaughtEx(e);
							set_user_error_status();
						}catch(UserEx &e){
							printCaughtEx(e);
							set_user_error_status();
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
						arglist_t e_parts;
						e_parts.push_back("subject");
						e_parts.push_back(loptarg);
						e_parts.push_back("=~");
						ostringstream s; 
						//use push_front for lines tests, to exploit short circuit evaluation (since comparing integers is faster than regexs)
						if (options.linelimit > 0) {
							e_parts.push_front(">=");
							e_parts.push_front(tostr(options.linelimit));
							e_parts.push_front("lines");
							e_parts.push_back("&&");
						}
						if (options.maxlinelimit < ULONG_MAX) {
							e_parts.push_front("<=");
							e_parts.push_front(tostr(options.maxlinelimit));
							e_parts.push_front("lines");
							e_parts.push_back("&&");
						}
						try {
							nntp_file_pred *p=make_pred(e_parts, options.gflags);
							getinfos.push_back(new c_nntp_getinfo(options.path, options.temppath, p, options.gflags));
						}catch(RegexEx &e){
							printCaughtEx(e);
							set_user_error_status();
						}catch(UserEx &e){
							printCaughtEx(e);
							set_fatal_error_status();//if make_pred breaks during -r, it can't be the users fault, since they only supply the regex
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
					options.badskip &= ~BAD_TEMPPATH;
					if (chdir(options.startpath.c_str())){
						set_path_error_status();
						throw ApplicationExFatal(Ex_INIT, "could not change to startpath: %s",options.startpath.c_str());
					}
				}else{
					printf("could not change temppath to %s\n",loptarg);
					set_path_error_status();
					options.badskip |= BAD_TEMPPATH;
				}
				break;
			case 'p':
				if (!maybe_mkdir_chdir(loptarg,options.makedirs)){
					options.get_path();
					options.get_temppath();
					printf("(temp)path:%s\n",options.path.c_str());
					options.badskip &= ~(BAD_TEMPPATH | BAD_PATH);
					if (chdir(options.startpath.c_str())){
						set_path_error_status();
						throw ApplicationExFatal(Ex_INIT, "could not change to startpath: %s",options.startpath.c_str());
					}
				}else{
					printf("could not change to %s\n",loptarg);
					set_path_error_status();
					options.badskip |= (BAD_TEMPPATH | BAD_PATH);
				}
				break;
			case 'F':
				{
					c_server::ptr server=nconfig.getserver(loptarg);
					if (!server) {printf("no such server %s\n",loptarg);set_user_error_status();break;}
					if (!options.group){
						printf("specify group before -F\n");
						set_user_error_status();
						break;
					}
					nntp.nntp_group(options.group,0,options);
					c_nntp_server_info* servinfo=nntp.gcache->getserverinfo(server->serverid);

					nntp.gcache->flushlow(servinfo,ULONG_MAX,nntp.midinfo);
					servinfo->reset();
					break;
				}
#ifdef HAVE_LIBPOPT
#define POPT_ERR_CASE(a) case a: printf("%s: %s\n",#a,optCon.BadOption(0)); print_help(); return 1;
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
			case 0://POPT_CONTEXT_ARG_OPTS
			case 1://getopt arg
				printf("invalid command line arg: %s\n", loptarg);
				set_user_error_status();
				return 1;
			default:
				if (!getinfos.empty()){
					nntp.nntp_retrieve(options.group, getinfos, options);
					getinfos.clear();
				}
				switch (c){
					case '@':
#ifdef HAVE_LIBPOPT
						{
							string filename=fcheckpath(loptarg,path_join(nghome,"lists"));
							s_argv larg;
							arglist_t arglist;
							try {
								c_file_fd f(filename.c_str(),O_RDONLY);
								f.initrbuf();
								while (f.bgets()>0){
									try{
										parseargs(arglist, f.rbufp(), true);
									} catch(UserExFatal &e) {
										printCaughtEx(e);
										set_user_error_status();
										break;
									}
								}
								f.close();
							}catch (FileNOENTEx &e){
								printf("error: %s\n",e.getExStr());
								set_user_error_status();
								break;
							}
							if (!arglist.empty()){
								int tc=0;
								larg.argc=arglist.size();
								larg.argv=(const char**)malloc((larg.argc+1)*sizeof(char**));
								arglist_t::iterator it=arglist.begin();
								for (;it!=arglist.end();++it,++tc){
									larg.argv[tc]=it->c_str();
								}

								do_args(larg.argc,larg.argv,options,1);
								//here we reset the stuff that may have been screwed in our recursiveness.  Perhaps it should reset it before returning, or something.. but I guess this'll do for now, since its the only place its called recursively.
								if (options.host)
									nntp.nntp_open(options.host);
								if (!chdir(options.startpath.c_str())){
									printf("path:%s\n",options.path.c_str());
								}else{
									set_path_error_status();
									throw ApplicationExFatal(Ex_INIT, "could not change to startpath: %s",options.startpath.c_str());
								}

								free(larg.argv);
							}
						}
#else
						printf("This option is only available when libpopt is used.\n");
#endif
						break;
					case 'g':
						//if BAD_HOST, don't try to -g, fall through to -G instead
						if (!(options.badskip & BAD_HOST)){
							options.group=nconfig.getgroup(loptarg);
							nntp.nntp_group(options.group,1,options);
							break;
						}
					case 'G':
						options.group=nconfig.getgroup(loptarg);
						break;
					case 'h':{
							if (*loptarg){
								options.host=nconfig.getserver(loptarg);
								if (options.host.isnull()){
									options.badskip |= BAD_HOST;
									printf("invalid host %s (must be configured in .ngetrc first)\n",loptarg);
									set_user_error_status();
								}
								else
									options.badskip &= ~BAD_HOST;
							}else{
								options.host=NULL;
								options.badskip &= ~BAD_HOST;
							}
							nntp.nntp_open(options.host);
						}
						break;
					case -1://end of args.
						return 0;
					default:
						print_help();
						return 1;
				}
		}
	}
}

int main(int argc, const char ** argv){
#ifdef HAVE_SETLINEBUF
	setlinebuf(stdout); //force stdout to be line buffered, useful if redirecting both stdout and err to a file, to keep them from getting out of sync.
#endif
	atexit(print_error_status);
	try {
		sockstuff_init();
		//	atexit(cache_dbginfo);
		addoptions();
		signal(SIGTERM,term_handler);
#ifdef SIGHUP
		signal(SIGHUP,term_handler);
#endif
		signal(SIGINT,term_handler);
#ifdef SIGQUIT
		signal(SIGQUIT,term_handler);
#endif
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

