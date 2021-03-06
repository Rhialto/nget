/*
    nget - command line nntp client
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
#include "_fileconf.h"

#include "path.h"
#include "misc.h"
#include "termstuff.h"
#include "strreps.h"
#include "sockstuff.h"
#include "prot_nntp.h"
#include "log.h"
#include "nget.h"
#include "cfgfile.h"
#include "myregex.h"
#include "status.h"


#define NUM_OPTIONS 44
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
			poptReadDefaultConfig(optCon, 0);
		}
		~c_poptContext() {
			poptFreeContext(optCon);
		}
};

static struct poptOption optionsTable[NUM_OPTIONS+1];
#endif //HAVE_LIBPOPT

struct opt_help {
	int namelen;
	const char *arg;
	const char *desc;
};
static opt_help ohelp[NUM_OPTIONS+1];
static int olongestlen=0;

enum {
	OPT_TEST_MULTI=2,
	OPT_TEXT_HANDLING,
	OPT_SAVE_TEXT_FOR_BINARIES,
	OPT_DECODE,
	OPT_TIMEOUT,
	OPT_DUPEPATH,
	OPT_AUTOPAR,
	OPT_NOAUTOPAR,
	OPT_FULLXOVER,
	OPT_BINDADDR,
	OPT_HELP,
	OPT_MIN_SHORTNAME //sentinel, must be last element.
};

static void addoption(const char *longo,int needarg,char shorto,const char *adesc,const char *desc){
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
	addoption("bindaddr",1,OPT_BINDADDR,"ADDR","local address to connect from");
	addoption("available",0,'a',0,"update/load available newsgroups list");
	addoption("quickavailable",0,'A',0,"load available newsgroups list");
	addoption("xavailable",0,'X',0,"search available newsgroups list without using cache files");
	addoption("group",1,'g',"GROUP(s)","update and use newsgroups (comma seperated)");
	addoption("quickgroup",1,'G',"GROUP(s)","use group(s) without checking for new headers");
	addoption("xgroup",1,'x',"GROUP(s)","use group(s) without using cache files (requires XPAT)");
	addoption("flushserver",1,'F',"HOSTALIAS","flush server from current group(s) or newsgroup list");
	addoption("expretrieve",1,'R',"EXPRESSION","retrieve files matching expression(see man page)");
	addoption("retrieve",1,'r',"REGEX","retrieve files matching regex");
	addoption("list",1,'@',"LISTFILE","read commands from listfile");
	addoption("path",1,'p',"DIRECTORY","path to store subsequent retrieves");
	addoption("temppath",1,'P',"DIRECTORY","path to store tempfiles");
	addoption("dupepath",1,OPT_DUPEPATH,"DIRECTORY","extra path to check for dupe files");
	addoption("makedirs",1,'m',"no,yes,ask,#","make dirs specified by -p and -P");
//	addoption("mark",1,'m',"MARKNAME","name of high water mark to test files against");
//	addoption("testretrieve",1,'R',"REGEX","test what would have been retrieved");
	addoption("testmode",0,'T',0,"test what would have been retrieved");
	addoption("test-multiserver",1,OPT_TEST_MULTI,"OPT","make testmode display per-server completion info (no(default)/long/short)");
	addoption("fullxover",1,OPT_FULLXOVER,"OPT","override fullxover setting (-1..2, default -1)");
	addoption("text",1,OPT_TEXT_HANDLING,"OPT","how to handle text posts (files(default)/mbox[:filename]/ignore)");
	addoption("save-binary-info",1,OPT_SAVE_TEXT_FOR_BINARIES,"OPT","save text files for posts that contained only binaries (yes/no(default))");
	addoption("tries",1,'t',"INT","set max retries (-1 unlimits, default 20)");
	addoption("delay",1,'s',"INT","seconds to wait between retry attempts(default 1)");
	addoption("timeout",1,OPT_TIMEOUT,"INT","seconds to wait for data from server(default 180)");
	addoption("limit",1,'l',"INT","min # of lines a 'file' must have(default 0)");
	addoption("maxlines",1,'L',"INT","max # of lines a 'file' must have(default -1)");
	addoption("incomplete",0,'i',0,"retrieve files with missing parts");
	addoption("complete",0,'I',0,"retrieve only files with all parts(default)");
	addoption("decode",0,OPT_DECODE,0,"decode and delete temp files (default)");
	addoption("keep",0,'k',0,"decode, but keep temp files");
	addoption("no-decode",0,'K',0,"keep temp files and don't even try to decode them");
	addoption("case",0,'c',0,"match casesensitively");
	addoption("nocase",0,'C',0,"match incasesensitively(default)");
	addoption("dupecheck",1,'d',"FLAGS","check to make sure you haven't already downloaded files(default -dfiM)");
	addoption("nodupecheck",0,'D',0,"don't check if you already have files(shortcut for -dFIM)");
	addoption("autopar",0,OPT_AUTOPAR,0,"only download as many par files as needed (default)");
	addoption("no-autopar",0,OPT_NOAUTOPAR,0,"disable special par file handling");
	addoption("mark",0,'M',0,"mark matching articles as retrieved");
	addoption("unmark",0,'U',0,"mark matching articles as not retrieved (implies -dI)");
	addoption("writelite",1,'w',"LITEFILE","write out a ngetlite list file");
	addoption("noconnect",0,'N',0,"don't connect, only try to decode what we have");
	addoption("help",0,OPT_HELP,0,"this help");
	addoption(NULL,0,0,NULL,NULL);
};
static void print_help(void){
	printf("nget v"PACKAGE_VERSION" - nntp command line fetcher\n");
	printf("Copyright 1999-2004 Matthew Mueller <donut AT dakotacom.net>\n");
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
		PERROR("nget_cleanup: caught std exception %s",e.what());
	}catch(...){
		PERROR("nget_cleanup: caught unknown exception");
	}
}

static void term_handler(int s){
	PERROR("\nterm_handler: signal %i, shutting down.",s);
	nget_cleanup();
	exit(get_exit_status());
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
void nget_options::do_get_path(string &s){char *p=goodgetcwd();s=p;free(p);}

nget_options::nget_options(void){
	do_get_path(startpath);
	get_path();
	get_temppath();

	makedirs=0;
	maxretry=20;
	retrydelay=1;
	badskip=0;
	linelimit=0;
	maxlinelimit=ULONG_MAX;
	gflags=0;
	test_multi=NO_SHOW_MULTI;
	retr_show_multi=SHOW_MULTI_LONG;//always display the multi-server info when retrieving, just because I think thats better
	cmdmode=RETRIEVE_MODE;
	host=NULL;
	texthandling=TEXT_FILES;
	save_text_for_binaries=false;
	mboxfname="nget.mbox";
	fullxover=-1;
}
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
				PERROR("unknown dupe flag %c",*opt);
				set_user_error_status_and_do_fatal_user_error();
		}
		opt++;
	}
}
int nget_options::set_save_text_for_binaries(const char *s){
	if (!s) {
		return 0;
	}
	if (strcasecmp(s,"yes")==0)
		save_text_for_binaries=true;
	else if (strcasecmp(s,"no")==0)
		save_text_for_binaries=false;
	else{
		PERROR("set_save_text_for_binaries invalid option %s",s);
		set_user_error_status_and_do_fatal_user_error();
		return 0;
	}
	return 1;
}
int nget_options::set_text_handling(const char *s){
	if (!s) {
		return 0;
	}
	if (strcasecmp(s,"files")==0)
		texthandling=TEXT_FILES;
	else if (strcasecmp(s,"mbox")==0)
		texthandling=TEXT_MBOX;
	else if (strncasecmp(s,"mbox:",5)==0) {
		texthandling=TEXT_MBOX;
		mboxfname=s+5;
	}
	else if (strcasecmp(s,"ignore")==0)
		texthandling=TEXT_IGNORE;
	else{
		PERROR("set_text_handling invalid option %s",s);
		set_user_error_status_and_do_fatal_user_error();
		return 0;
	}
	return 1;
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
		PERROR("set_test_multi invalid option %s",s);
		set_user_error_status_and_do_fatal_user_error();
		return 0;
	}
	return 1;
}
#define MAKEDIRS_YES -1
#define MAKEDIRS_ASK -2
int nget_options::set_makedirs(const char *s){
	if (!s) {
		//printf("set_makedirs s=NULL\n");
		return 0;
	}
	if (strcasecmp(s,"yes")==0)
		makedirs=MAKEDIRS_YES;
	else if (strcasecmp(s,"ask")==0)
		makedirs=MAKEDIRS_ASK;
	else if (strcasecmp(s,"no")==0)
		makedirs=0;
	else{
		char *erp;
		int numcreate = strtol(s,&erp,10);
		if (*s=='\0' || *erp!='\0' || numcreate < 0) {
			PERROR("set_makedirs invalid option %s",s);
			set_user_error_status_and_do_fatal_user_error();
			return 0;
		}
		makedirs = numcreate;
	}
	return 1;
}
//	~nget_options(){del_path();}

int makedirs(const char *dir, int mode){
	string head(dir), tail;
	list<string> parts;
	while (!head.empty() && !direxists(head)) {
		path_split(head, tail);
		parts.push_front(tail);
	}
	while (!parts.empty()) {
		path_append(head, parts.front());
		parts.pop_front();
		if (mkdir(head.c_str(),mode))
			return -1;
	}
	return 0;
}

static int missing_path_tail_count(const char *dir) {
	string head(dir), tail;
	int count=0;
	while (!head.empty() && !direxists(head)) {
		path_split(head, tail);
		count++;
	}
	return count;
}

int maybe_mkdir_chdir(const char *dir, int makedir){
	if (chdir(dir)){
		if (errno==ENOENT && makedir){
			int missing_count = missing_path_tail_count(dir);
			assert(missing_count>0);
			bool doit=0;
			if (makedir==MAKEDIRS_ASK){
				char buf[40],*p;
				if (!is_abspath(dir)){
					p=goodgetcwd();
					printf("in %s, ",p);
					free(p);
				}
				printf("do you want to make dir %s?",dir);
				if (missing_count>1)
					printf(" (%i non-existant parts) ",missing_count);
				fflush(stdout);
				while (1){
					if (fgets(buf,39,stdin)){
						if ((p=strpbrk(buf,"\r\n"))) *p=0;
						if (strcasecmp(buf,"yes")==0 || strcasecmp(buf,"y")==0){
							doit=1;break;
						}
						if (strcasecmp(buf,"no")==0 || strcasecmp(buf,"n")==0){
							break;
						}
						char *erp;
						int numcreate = strtol(buf,&erp,10);
						if (*buf!='\0' && *erp=='\0') {
							if (numcreate >= missing_count)
								doit=1;
							break;
						}
						printf("%s?? enter y[es], n[o], or max # of dirs to create.\n",buf);
					}else{
						perror("fgets");
						return -1;
					}
				}
			}else if(makedir==MAKEDIRS_YES)
				doit=1;
			else if (makedir>=missing_count)
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
	t_xpat_list patinfos;
	t_grouplist_getinfo_list grouplistgetinfos;

#ifdef HAVE_LIBPOPT
#ifndef POPT_CONTEXT_ARG_OPTS
#define POPT_CONTEXT_ARG_OPTS 0
#endif
	c_poptContext optCon("nget", argc, argv, optionsTable, (sub?POPT_CONTEXT_KEEP_FIRST:0) | POPT_CONTEXT_ARG_OPTS);
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
			case OPT_TEXT_HANDLING:
				options.set_text_handling(loptarg);
				break;
			case OPT_SAVE_TEXT_FOR_BINARIES:
				options.set_save_text_for_binaries(loptarg);
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
				if (options.cmdmode==NOCACHE_GROUPLIST_MODE) {
					PERROR("-R is not yet supported with -X");
					set_user_error_status_and_do_fatal_user_error();
					break;
				}
				if (options.cmdmode==GROUPLIST_MODE) {
					try {
						nntp_grouplist_pred *p=make_grouplist_pred(loptarg, options.gflags);
						grouplistgetinfos.push_back(new c_grouplist_getinfo(p, options.gflags));
					}catch(RegexEx &e){
						printCaughtEx(e);
						set_user_error_status_and_do_fatal_user_error();
					}catch(UserEx &e){
						printCaughtEx(e);
						set_user_error_status_and_do_fatal_user_error();
					}
					break;
				}
				if (!options.badskip){
					if (options.cmdmode==NOCACHE_RETRIEVE_MODE) {
						PERROR("-R is not yet supported with -x");
						set_user_error_status_and_do_fatal_user_error();
						break;
					}
					if(options.groups.empty()){
						PERROR("no group specified");
						set_user_error_status_and_do_fatal_user_error();
					}else{
						try {
							nntp_file_pred *p=make_nntpfile_pred(loptarg, options.gflags);
							getinfos.push_back(new c_nntp_getinfo(options.path, options.temppath, options.dupepaths, p, options.gflags));
						}catch(RegexEx &e){
							printCaughtEx(e);
							set_user_error_status_and_do_fatal_user_error();
						}catch(UserEx &e){
							printCaughtEx(e);
							set_user_error_status_and_do_fatal_user_error();
						}
					}
				}
				break;
			case 'r':
				if (options.cmdmode==GROUPLIST_MODE || options.cmdmode==NOCACHE_GROUPLIST_MODE) {
					arglist_t e_parts;
					e_parts.push_back("group");
					e_parts.push_back(loptarg);
					e_parts.push_back("=~");

					e_parts.push_back("desc");
					e_parts.push_back(loptarg);
					e_parts.push_back("=~");

					e_parts.push_back("||");
					try {
						if (options.cmdmode==NOCACHE_GROUPLIST_MODE) {
							patinfos.push_back(new c_xpat("", regex2wildmat(loptarg, !(options.gflags&GETFILES_CASESENSITIVE))));
						}
						nntp_grouplist_pred *p=make_grouplist_pred(e_parts, options.gflags);
						grouplistgetinfos.push_back(new c_grouplist_getinfo(p, options.gflags));
					}catch(RegexEx &e){
						printCaughtEx(e);
						set_user_error_status_and_do_fatal_user_error();
					}
					//if make_pred breaks during -r, it can't be the users fault, since they only supply the regex, so let UserEx be be caught by the main func
					break;
				}
				if (!options.badskip){
					if(options.groups.empty()){
						PERROR("no group specified");
						set_user_error_status_and_do_fatal_user_error();
					}else{
						arglist_t e_parts;
						e_parts.push_back("subject");
						e_parts.push_back(loptarg);
						e_parts.push_back("=~");
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
							if (options.cmdmode==NOCACHE_RETRIEVE_MODE) {
								patinfos.push_back(new c_xpat("Subject", regex2wildmat(loptarg, !(options.gflags&GETFILES_CASESENSITIVE))));
							}
							nntp_file_pred *p=make_nntpfile_pred(e_parts, options.gflags);
							getinfos.push_back(new c_nntp_getinfo(options.path, options.temppath, options.dupepaths, p, options.gflags));
						}catch(RegexEx &e){
							printCaughtEx(e);
							set_user_error_status_and_do_fatal_user_error();
						}
						//if make_pred breaks during -r, it can't be the users fault, since they only supply the regex, so let UserEx be be caught by the main func
					}
				}
				break;
			case OPT_AUTOPAR:
				options.gflags&= ~GETFILES_NOAUTOPAR;
				break;
			case OPT_NOAUTOPAR:
				options.gflags|= GETFILES_NOAUTOPAR;
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
			case OPT_DECODE:
				options.gflags&= ~(GETFILES_NODECODE|GETFILES_KEEPTEMP);
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
				if (parsestr(loptarg, options.retrydelay, 0, INT_MAX, "--delay"))
					PMSG("retry delay set to %i",options.retrydelay);
				break;
			case OPT_TIMEOUT:
				if (parsestr(loptarg, sock_timeout, 1, INT_MAX, "--timeout"))
					PMSG("sock timeout set to %i",sock_timeout);
				break;
			case 't':
				if (parsestr(loptarg, options.maxretry, -1, INT_MAX, "--tries")) {
					if (options.maxretry<=0)
						options.maxretry=INT_MAX-1;
					PMSG("max retries set to %i",options.maxretry);
				}
				break;
			case 'L':
				if (strcmp(loptarg,"-1")==0)
					options.maxlinelimit=ULONG_MAX;
				else if (!parsestr(loptarg, options.maxlinelimit, "--maxlines"))
					break;
				PMSG("maximum line limit set to %lu",options.maxlinelimit);
				break;
			case 'l':
				if (parsestr(loptarg, options.linelimit, "--limit"))
					PMSG("minimum line limit set to %lu",options.linelimit);
				break;
			case 'w':
				options.writelite=loptarg;
				PMSG("writelite to %s",options.writelite.c_str());
				break;
			case 'm':
				options.set_makedirs(loptarg);
				break;
			case 'P':
				if (!maybe_mkdir_chdir(loptarg,options.makedirs)){
					options.get_temppath();
					PMSG("temppath:%s",options.temppath.c_str());
					options.badskip &= ~BAD_TEMPPATH;
					if (chdir(options.startpath.c_str())){
						set_path_error_status();
						throw ApplicationExFatal(Ex_INIT, "could not change to startpath: %s",options.startpath.c_str());
					}
				}else{
					PERROR("could not change temppath to %s",loptarg);
					set_path_error_status_and_do_fatal_user_error();
					options.badskip |= BAD_TEMPPATH;
				}
				break;
			case 'p':
				if (!maybe_mkdir_chdir(loptarg,options.makedirs)){
					options.dupepaths.clear();
					options.get_path();
					options.get_temppath();
					PMSG("(temp)path:%s",options.path.c_str());
					options.badskip &= ~(BAD_TEMPPATH | BAD_PATH);
					if (chdir(options.startpath.c_str())){
						set_path_error_status();
						throw ApplicationExFatal(Ex_INIT, "could not change to startpath: %s",options.startpath.c_str());
					}
				}else{
					PERROR("could not change to %s",loptarg);
					set_path_error_status_and_do_fatal_user_error();
					options.badskip |= (BAD_TEMPPATH | BAD_PATH);
				}
				break;
			case OPT_DUPEPATH:
				if (direxists(loptarg)) {
					options.dupepaths.push_back(loptarg);
				} else {
					PERROR("dupepath %s does not exist",loptarg);
					set_path_error_status_and_do_fatal_user_error();
				}
				break;
			case OPT_FULLXOVER:
				parsestr(loptarg, options.fullxover, -1, 2, "--fullxover");
				break;
			case 'F':
				{
					c_server::ptr server=nconfig.getserver(loptarg);
					if (!server) {PERROR("no such server %s",loptarg);set_user_error_status_and_do_fatal_user_error();break;}
					if (options.cmdmode==NOCACHE_RETRIEVE_MODE || options.cmdmode==NOCACHE_GROUPLIST_MODE) {
						PERROR("nothing to flush in nocache mode");
						set_user_error_status_and_do_fatal_user_error();
						break;
					}
					if (options.cmdmode==GROUPLIST_MODE) {
						nntp.nntp_grouplist(0, options);
						nntp.glist->flushserver(server->serverid);
						break;
					}
					if (options.groups.empty()){
						PERROR("specify group before -F");
						set_user_error_status_and_do_fatal_user_error();
						break;
					}
					for (vector<c_group_info::ptr>::const_iterator gi=options.groups.begin(); gi!=options.groups.end(); gi++) {
						nntp.nntp_group(*gi, false, options);
						c_nntp_server_info* servinfo=nntp.gcache->getserverinfo(server->serverid);
						nntp.gcache->flushlow(servinfo,ULONG_MAX,nntp.midinfo);
						servinfo->reset();
					}
					break;
				}
#ifdef HAVE_LIBPOPT
#define POPT_ERR_CASE(a) case a: PERROR("%s: %s",#a,optCon.BadOption(0)); set_user_error_status_and_do_fatal_user_error(); break;
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
				//getopt prints the error message itself.
				set_user_error_status_and_do_fatal_user_error();
				break;
			case OPT_HELP:
				print_help();
				return 1;
			case 0://POPT_CONTEXT_ARG_OPTS
			case 1://getopt arg
				PERROR("invalid command line arg: %s", loptarg);
				set_user_error_status_and_do_fatal_user_error();
				return 1;
			default:
				if (!grouplistgetinfos.empty()) {
					if(!(options.gflags&GETFILES_TESTMODE)){
						PERROR("testmode required for grouplist");
						set_user_error_status_and_do_fatal_user_error();
					}else if (!patinfos.empty()){
						nntp.nntp_grouplist_search(grouplistgetinfos, patinfos, options);
					}else{
						nntp.nntp_grouplist_search(grouplistgetinfos, options);
					}
					grouplistgetinfos.clear();
					patinfos.clear();
				}
				if (!patinfos.empty()){
					nntp.nntp_retrieve(options.groups, getinfos, patinfos, options);
					getinfos.clear();
					patinfos.clear();
				}
				else if (!getinfos.empty()){
					nntp.nntp_retrieve(options.groups, getinfos, options);
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
										set_user_error_status_and_do_fatal_user_error();
										break;
									}
								}
								f.close();
							}catch (FileNOENTEx &e){
								PERROR("error: %s",e.getExStr());
								set_user_error_status_and_do_fatal_user_error();
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
									PMSG("path:%s",options.path.c_str());
								}else{
									set_path_error_status();
									throw ApplicationExFatal(Ex_INIT, "could not change to startpath: %s",options.startpath.c_str());
								}

								free(larg.argv);
							}
						}
#else
						PERROR("This option is only available when libpopt is used.");
#endif
						break;
					case 'a':
						//if BAD_HOST, don't try to -a, fall through to -A instead
						if (!(options.badskip & BAD_HOST)){
							options.cmdmode=GROUPLIST_MODE;
							nntp.nntp_grouplist(1, options);
							break;
						}
					case 'A':
						options.cmdmode=GROUPLIST_MODE;
						break;
					case 'X':
						options.cmdmode=NOCACHE_GROUPLIST_MODE;
						break;
					case 'g':
						//if BAD_HOST, don't try to -g, fall through to -G instead
						if (!(options.badskip & BAD_HOST)){
							options.cmdmode=RETRIEVE_MODE;
							nconfig.getgroups(options.groups, loptarg);
							for (vector<c_group_info::ptr>::const_iterator gi=options.groups.begin(); gi!=options.groups.end(); gi++)
								nntp.nntp_group(*gi, true, options);
							break;
						}
					case 'G':
						options.cmdmode=RETRIEVE_MODE;
						nconfig.getgroups(options.groups, loptarg);
						break;
					case 'x':
						options.cmdmode=NOCACHE_RETRIEVE_MODE;
						nconfig.getgroups(options.groups, loptarg);
						break;
					case 'h':{
							if (*loptarg){
								options.host=nconfig.getserver(loptarg);
								if (options.host.isnull()){
									options.badskip |= BAD_HOST;
									PERROR("invalid host %s (must be configured in .ngetrc first)",loptarg);
									set_user_error_status_and_do_fatal_user_error();
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
					case OPT_BINDADDR:
						options.bindaddr = loptarg;
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
#ifdef SIGPIPE
		signal(SIGPIPE,SIG_IGN); //don't die on broken connections (some platforms don't support MSG_NOSIGNAL)
#endif
		{
			char *home;
			home=getenv("NGETHOME");
			if (home && *home) {
				nghome=path_join(home,"");
			} else {
				home=getenv("HOME");
				if (!home || !*home)
					throw ConfigExFatal(Ex_INIT,"HOME or NGETHOME environment var not set.");
				nghome = home;
				if (direxists(path_join(home,".nget5","")))
					nghome=path_join(home,".nget5","");
				else if (direxists(path_join(home,"_nget5","")))
					nghome=path_join(home,"_nget5","");
				else
					throw ConfigExFatal(Ex_INIT,"neither %s nor %s exist", path_join(home,".nget5","").c_str(), path_join(home,"_nget5","").c_str());
			}
		}
		ngcachehome = nghome;

		srand(time(NULL));
		if (argc<2){
			print_help();
		}
		else {
			nget_options options;
			{
				char *cp;
				cp = getenv("NGETRC");
				string ngetrcfn;
				if (cp && *cp) {
					ngetrcfn = cp;
					if (!fexists(ngetrcfn))
						throw ConfigExFatal(Ex_INIT,"NGETRC %s: not found", ngetrcfn.c_str());
				}
				else {
					ngetrcfn = nghome + ".ngetrc";
					if (!fexists(ngetrcfn)) {
						ngetrcfn = nghome + "_ngetrc";
						if (!fexists(ngetrcfn))
							throw ConfigExFatal(Ex_INIT,"neither %s nor %s exist", (nghome + ".ngetrc").c_str(), ngetrcfn.c_str());
					}
				}
				CfgSection cfg(ngetrcfn);
				const CfgSection *galias,*halias,*hpriority;
				halias=cfg.getsection("halias");
				hpriority=cfg.getsection("hpriority");
				galias=cfg.getsection("galias");
				if (!halias)
					throw ConfigExFatal(Ex_INIT,"no halias section");
				nconfig.setlist(&cfg,halias,hpriority,galias);
				int t;
				/*			if (!halias || !(options.host=halias->getitema("default")))
							options.host=getenv("NNTPSERVER")?:"";
							nntp.nntp_open(options.host,NULL,NULL);*/
				/*			options.host=halias->getsection("default");
							nntp.nntp_open(options.host);*/
				cfg.get("timeout",sock_timeout,1,INT_MAX);
				cfg.get("debug",debug,0,DEBUG_ALL);
				cfg.get("quiet",quiet,0,2);
				cfg.get("limit",options.linelimit,0UL,ULONG_MAX);
				cfg.get("tries",options.maxretry,1,INT_MAX);
				cfg.get("delay",options.retrydelay,0,INT_MAX);
				if (cfg.get("case",t,0,1) && t==1)
					options.gflags|= GETFILES_CASESENSITIVE;
				if (cfg.get("complete",t,0,1) && t==0)
					options.gflags|= GETFILES_GETINCOMPLETE;
				if (cfg.get("dupeidcheck",t,0,1) && t==0)
					options.gflags|= GETFILES_NODUPEIDCHECK;
				if (cfg.get("dupefilecheck",t,0,1) && t==0)
					options.gflags|= GETFILES_NODUPEFILECHECK;
				if (cfg.get("dupefilemark",t,0,1) && t==1)
					options.gflags|= GETFILES_DUPEFILEMARK;
				if (cfg.get("tempshortnames",t,0,1) && t==1)
					options.gflags|= GETFILES_TEMPSHORTNAMES;
				if (cfg.get("save_binary_info",t,0,1) && t==1)
					options.save_text_for_binaries=true;
				if (cfg.get("autopar",t,0,1) && t==0)
					options.gflags|= GETFILES_NOAUTOPAR;
				options.set_test_multi(cfg.geta("test_multiserver"));
				options.set_text_handling(cfg.geta("text"));
				options.set_makedirs(cfg.geta("makedirs"));
				
				cfg.get("cachedir",ngcachehome);//.ngetrc setting overrides default
				cp=getenv("NGETCACHE");//environment var overrides .ngetrc
				if (cp && *cp)
					ngcachehome=cp;
				ngcachehome = path_join(ngcachehome, "");
				if (!direxists(ngcachehome))
					throw ConfigExFatal(Ex_INIT,"cache dir %s does not exist", ngcachehome.c_str());
				cfg.check_unused();
			}
			//check for user errors here rather than using set_user_error_status_and_do_fatal_user_error, so that all config entries can be checked before exiting.
			if (get_user_error_status() && nconfig.fatal_user_errors)
				throw FatalUserException();
			init_term_stuff();
			options.get_path();
			nntp.initready();
			do_args(argc,argv,options,0);
		}
	}catch(FatalUserException &e){
		PERROR("fatal_user_errors enabled, exiting");
	}catch(ConfigEx &e){
		set_fatal_error_status();
		printCaughtEx(e);
		PERROR("(see man nget for configuration info)");
	}catch(baseEx &e){
		set_fatal_error_status();
		PERROR_nnl("main():");printCaughtEx(e);
	}catch(exception &e){
		set_fatal_error_status();
		PERROR("caught std exception %s",e.what());
	}catch(...){
		set_fatal_error_status();
		PERROR("caught unknown exception");
	}
	nget_cleanup();

	return get_exit_status();
}

