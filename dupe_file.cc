#include "dupe_file.h"
#include "log.h"

#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

void dupe_file_checker::addfile(const string &path, const char *filename) {
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
	flist.push_front(fm);
}

void dupe_file_checker::addfrompath(const string &path){
	DIR *dir=opendir(path.c_str());
	struct dirent *de;
	if (!dir)
		throw PathExFatal(Ex_INIT,"opendir: %s(%i)",strerror(errno),errno);
	while ((de=readdir(dir))) {
		if (strcmp(de->d_name,"..")==0) continue;
		if (strcmp(de->d_name,".")==0) continue;
		addfile(path, de->d_name);
	}
	closedir(dir);
}

int dupe_file_checker::checkhavefile(const char *f, const string &messageid, ulong bytes){
	filematchlist::iterator curl;
	file_match *fm;
	for (curl=flist.begin();curl!=flist.end();++curl){
		fm=*curl;
		if (fm->size*2>bytes && fm->size<bytes && (fm->reg.match(f)==0/* || fm->reg.match((messageid+".txt").c_str())==0*/)){//TODO: handle text files saved.
			//			printf("0\n");
			printf("already have %s\n",f);
			return 1;
		}
		//		printf("1\n");
	}

	return 0;
}

void dupe_file_checker::clear(void){
	filematchlist::iterator curl;
	for (curl=flist.begin();curl!=flist.end();++curl)
		delete *curl;
	flist.clear();
}



