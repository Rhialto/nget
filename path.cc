#include "path.h"
#include <errno.h>
#include <unistd.h>
#include "log.h"
#include "_fileconf.h"

#ifdef WIN32
#include <ctype.h>
bool is_abspath(const char *p) {
	if (is_pathsep(p[0]))
		return true;
	if (p[0]!=0 && p[1]!=0 && p[2]!=0 && isalpha(p[0]) && p[1]==':' && is_pathsep(p[2]))
		return true;
	return false;
}
#endif

string& path_append(string &a, string b) {
	char c=a[a.size()-1];
	if (!is_pathsep(c))
		a.push_back(PATHSEP);
	return a.append(b);
}

string path_join(string a, string b) {
	return path_append(a, b);
}
string path_join(string a, string b, string c) {
	return path_append(path_append(a,b), c);
}


bool direxists(const char *p) {
	bool ret=false;
	char *oldp;
	goodgetcwd(&oldp);
	if (chdir(p)==0)
		ret=true;
	if (chdir(oldp)) {
		free(oldp);
		throw ApplicationExFatal(Ex_INIT, "could not return to oldp: %s (%s)",oldp,strerror(errno));
	}
	free(oldp);
	return ret;
}
int fexists(const char * f){
	struct stat statbuf;
	return (!stat(f,&statbuf));
}
string fcheckpath(const char *fn, string path){
	if (!is_abspath(fn)) {
		string pfn = path_join(path,fn);
		if (fexists(pfn.c_str()))
			return pfn;
	}
	return fn;
}

int testmkdir(const char * dir,int mode){
	struct stat statbuf;
	if (stat(dir,&statbuf)){
		return (mkdir(dir,mode));
	} else if (!S_ISDIR(statbuf.st_mode)){
		return ENOTDIR;
	}
	return 0;
}
char *goodgetcwd(char **p){
	/*int t=-1;
	*p=NULL;//first we try to take advantage of the GNU extension to allocate it for us.
	while (!(*p=getcwd(*p,t))) {
		t+=100;
		if (!(*p=(char*)realloc(*p,t))){
			PERROR("realloc error %s",strerror(errno));exit(1);
		}
	}*/
	int t=0;
	*p=NULL;//ack, that extension seems to like segfaulting when you free() its return.. blah.
	do {
		t+=48;
		if (!(*p=(char*)realloc(*p,t))){
			PERROR("goodgetcwd: realloc error %s (size=%i)",strerror(errno), t);exit(128);
		}
	}while(!(*p=getcwd(*p,t)));
	PDEBUG(DEBUG_MED,"goodgetcwd %i %s(%p-%p)",t,*p,p,*p);
	return *p;
}

