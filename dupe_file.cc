#include "dupe_file.h"
#include "log.h"

#include <glob.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

void flist_addfile(filematchlist &l, const string &path, const char *filename) {
	struct stat stbuf;
	file_match *fm;
	string buf;
	const char *cp = filename;
	if (isalnum(*cp)){ //only add word boundry match if we are actually next to a word, or else there isn't a word boundry to match at all.
		buf+='\\';
#ifdef HAVE_PCREPOSIX_H
		buf+='b';//match word boundary
#else	
		buf+='<';//match beginning of word
#endif
	}
	while (*cp){
		if (strchr("{}()|[]\\.+*^$",*cp))
			buf+='\\';//escape special chars
		buf+=*cp;
		cp++;
	}
	if (isalnum(*(cp-1))){
		buf+='\\';
#ifdef HAVE_PCREPOSIX_H
		buf+='b';//match word boundary
#else
		buf+='>';//match end of word
#endif
	}
	fm=new file_match(buf.c_str(),REG_EXTENDED|REG_ICASE|REG_NOSUB);
	if (stat((path+'/'+filename).c_str(), &stbuf)==0)
		fm->size=stbuf.st_size;
	else
		fm->size=0;
	l.push_front(fm);
}

void buildflist(const string &path, filematchlist **l){
	DIR *dir=opendir(path.c_str());
	struct dirent *de;
	if (!dir)
		throw PathExFatal(Ex_INIT,"opendir: %s(%i)",strerror(errno),errno);
	*l=new filematchlist;
	while ((de=readdir(dir))) {
		if (strcmp(de->d_name,"..")==0) continue;
		if (strcmp(de->d_name,".")==0) continue;
		flist_addfile(**l, path, de->d_name);
	}
	closedir(dir);
}

int flist_checkhavefile(filematchlist *fl,const char *f,string messageid,ulong bytes){
	filematchlist::iterator curl;
	if (fl){
		file_match *fm;
		for (curl=fl->begin();curl!=fl->end();++curl){
			fm=*curl;
			if ((fm->reg.match(f)==0/* || fm->reg.match((messageid+".txt").c_str())==0*/) && fm->size*2>bytes && fm->size<bytes){//TODO: handle text files saved.
				//			printf("0\n");
				printf("already have %s\n",f);
				return 1;
			}
			//		printf("1\n");
		}
	}
	return 0;
}



