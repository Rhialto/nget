    Copyright (C) 1999-2001  Matthew Mueller <donut@azstarnet.com>
#include <memory>
			if (strchr("{}()|[]\\.+*^$",*cp))
				PDEBUG(DEBUG_MED,"%lu->%s was gonna add, but author is different?\n",h->articlenum,f->bamid().c_str());
				PDEBUG(DEBUG_MED,"%lu->%s was gonna add, but subject is different?\n",h->articlenum,f->bamid().c_str());
void c_nntp_cache::getxrange(c_nntp_server_info *servinfo,ulong newlow,ulong newhigh, c_nrange *range){
	range->insert(newlow<servinfo->low?newlow:servinfo->low,newhigh);
	if (quiet<2) {printf("Flushing headers %lu-%lu(%lu):",servinfo->low,newlow-1,newlow-servinfo->low);fflush(stdout);}
	if (quiet<2){printf(" %lu (%lu,%lu)\n",count,countp,countf);}
void setfilenamegz(string &file, int gz=-2){
#ifndef HAVE_LIBZ
	gz=0;
	if (gz==-2)
		gz=nconfig.usegz;
	if (gz)
		file.append(".gz");
}
c_file *dofileopen(string file, string mode, int gz=-2){
	c_file *f=NULL;
#ifndef HAVE_LIBZ
	gz=0;
	if (gz==-2)
		gz=nconfig.usegz;
	if (gz){
		if (gz>0){
			char blah[10];
			sprintf(blah,"%i",gz);
			mode.append(blah);
		}
		c_file_gz *gz=new c_file_gz();
		if (gz->open(file.c_str(),mode.c_str())){
			delete gz;
			return NULL;
		}
		f=gz;
	}
	if (!gz){
		c_file_fd *fd=new c_file_fd();
		if (fd->open(file.c_str(),mode.c_str())){
			delete fd;
			return NULL;
		}
		f=fd;
	}
	if (mode[0]=='r' || mode.find('+')>=0)
		f->initrbuf(2048);
	return f;
}

c_nntp_cache::c_nntp_cache(string path,c_group_info::ptr group_):totalnum(0),group(group_){
	c_file *f=NULL;
	saveit=0;
	//file=nid;
	file=path+"/"+group->group + ",cache";
	setfilenamegz(file,group->usegz);
	fileread=0;
	if ((f=dofileopen(file.c_str(),"rb",group->usegz))){
		auto_ptr<c_file> fcloser(f);
		while (f->bgets()>0){
			buf=f->rbufp();
					f->close();fileread=0;
						sa=new c_nntp_server_article(atoul(t[0]),atoul(t[1]),atoul(t[2]),atoul(t[3]));
	c_file *f=NULL;
		string tmpfn;
		tmpfn=file+".tmp";
		if((f=dofileopen(tmpfn,"wb",group->usegz))){
			auto_ptr<c_file> fcloser(f);
			if (quiet<2){printf("saving cache: %lu parts, %i files..",totalnum,files.size());fflush(stdout);}
			f->putf(CACHE_VERSION"\t%lu\n",totalnum);//mode 2
				f->putf("%lu\t%lu\t%lu\t%lu\n",si->serverid,si->high,si->low,si->num);//mode 4
			f->putf(".\n");
				f->putf("%i\t%lu\t%lu\t%s\t%s\t%i\t%i\n",nf->req,nf->flags,nf->fileid,nf->subject.c_str(),nf->author.c_str(),nf->partoff,nf->tailoff);//mode 0
					f->putf("%i\t%lu\t%s\n",np->partnum,np->date,np->messageid.c_str());//mode 1
						f->putf("%lu\t%lu\t%lu\t%lu\n",sa->serverid,sa->articlenum,sa->bytes,sa->lines);//mode 3
					f->putf(".\n");//end mode 3
				f->putf(".\n");//end mode 1
				(*i).second=NULL; //free cache as we go along instead of at the end, so we don't swap more with low-mem.
			f->close();
			if (quiet<2) printf(" done. (%lu sa)\n",counta);
			if (rename(tmpfn.c_str(), file.c_str())){
				printf("error renaming %s > %s: %s(%i)\n",tmpfn.c_str(),file.c_str(),strerror(errno),errno);
			}
			printf("error opening %s: %s(%i)\n",tmpfn.c_str(),strerror(errno),errno);
	if (quiet<2){printf("freeing cache: %lu parts, %i files..\n",totalnum,files.size());}//fflush(stdout);}
int c_mid_info::load(string path,bool merge,bool lock){
	c_file *f=NULL;
		setfilenamegz(path);//ugh, hack.
	//c_lockfile locker(path,WANT_SH_LOCK);
	auto_ptr<c_lockfile> locker;
	if (lock){
		auto_ptr<c_lockfile> l(new c_lockfile(path,WANT_SH_LOCK));
		locker=l;
		//locker=new c_lockfile(path,WANT_SH_LOCK);//why can't we just do this?  sigh.
	}
	if ((f=dofileopen(path.c_str(),"rb"))){
		auto_ptr<c_file> fcloser(f);
		while (f->bgets()>0){
			if (!toker.tok(f->rbufp()) && toker.numtoks==3)
		f->close();
	c_file *f=NULL;
	c_lockfile locker(file,WANT_EX_LOCK);//lock before we read, so that multiple copies trying to save at once don't lose changes.
		load(file,1,0);//merge any changes that might have happened
	string tmpfn=file+".tmp";
	if((f=dofileopen(tmpfn,"wb"))){
		auto_ptr<c_file> fcloser(f);
			f->putf("%s %li %li\n",ms->messageid.c_str(),ms->date_added,ms->date_removed);
		f->close();
		if (rename(tmpfn.c_str(), file.c_str())){
			printf("error renaming %s > %s: %s(%i)\n",tmpfn.c_str(),file.c_str(),strerror(errno),errno);
			return -1;
		}