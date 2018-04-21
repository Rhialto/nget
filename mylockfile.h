/*
    mylockfile.* - c++ wrapper for file locks (for easy unlocking due to destructors, and abstraction from lowlevel)
    Copyright (C) 1999-2003  Matthew Mueller <donut AT dakotacom.net>

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
#ifndef _MYLOCKFILE_H_
#define _MYLOCKFILE_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string>

#include <errno.h>
#include "log.h"

//liblockfile doesn't support anything but exclusive locks, so these are only "wants"
#define WANT_SH_LOCK 1
#define WANT_EX_LOCK 0
#define FLOCK_DEBUG_LEV DEBUG_MED

#ifdef HAVE_LIBLOCKFILE
#include <lockfile.h>
class c_lockfile{
	public:
		string file;

		c_lockfile(string filename,int flag){
			file=filename;
			file.append(".lock");
			PDEBUG(FLOCK_DEBUG_LEV,"attempting to lock %s",file.c_str());
			int ret=lockfile_create(file.c_str(),10,0);
			if (ret){
				file = "";
				throw ApplicationExFatal(Ex_INIT,"lockfile_create %s: %i (%s)",filename.c_str(),ret,strerror(errno));
			}
			PDEBUG(FLOCK_DEBUG_LEV,"locked %s",file.c_str());
		}
		~c_lockfile(){
			if (!file.empty()) {
				lockfile_remove(file.c_str());
				PDEBUG(FLOCK_DEBUG_LEV,"unlocked %s",file.c_str());
			}
		}
};
#elif HAVE_LOCKFILE
#define WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#define NOMINMAX 1
#include <windows.h>
class c_lockfile{
	public:
		HANDLE hFile;
		string filename;

		c_lockfile(string infilename,int flag): hFile(INVALID_HANDLE_VALUE){
			filename = infilename + ".lock";//Windows does not allow removing the locked file, (when we rename the .tmp to the real name), so create a seperate lock file instead.
			PDEBUG(FLOCK_DEBUG_LEV,"attempting to LockFile %s",filename.c_str());
			//if (!fexists(filename.c_str()))
			//	return;
			//hFile = CreateFile(filename.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
			hFile = CreateFile(filename.c_str(), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile==INVALID_HANDLE_VALUE){
				throw ApplicationExFatal(Ex_INIT,"c_lockfile: open %s (%i)",filename.c_str(),GetLastError());
			}
			int tries=1;
			while (!LockFile(hFile,0,0,0xFFFFFFFF,0)) {
				if (tries++ >= 10) {
					CloseHandle(hFile); hFile = INVALID_HANDLE_VALUE;
					throw ApplicationExFatal(Ex_INIT,"c_lockfile: LockFile %s: giving up after %i tries",filename.c_str(), tries);
				}
				sleep(1);
				PDEBUG(FLOCK_DEBUG_LEV,"try %i LockFile %s",tries,filename.c_str());
			}
			PDEBUG(FLOCK_DEBUG_LEV,"locked %s (%p)",filename.c_str(),hFile);
		}
		~c_lockfile(){
			if (hFile!=INVALID_HANDLE_VALUE) {
				UnlockFile(hFile,0,0,0xFFFFFFFF,0);
				CloseHandle(hFile);
				DeleteFile(filename.c_str());
				PDEBUG(FLOCK_DEBUG_LEV,"unlocked %s(%p)",filename.c_str(),hFile);
			}
		}
};
#elif HAVE_FCNTL
#include <unistd.h>
#include <fcntl.h>
// Generic POSIX fcntl locking.
//
// All our fcntl() locks are applied to a ".locks" file, so that the lock
// holder can freely unlink or rename the locked file without concern for
// impacting the locks themselves.  Addition of locks to the locks file,
// as well as creation or removal of the locks file, is locked by another
// file which has open() O_EXCL semantics. (Note, that we don't ever lock
// the lock to the locks file to *remove* locks from the locks file, as
// that would deadlock.)
//
class c_lockfile{
	public:
		int fd;
		long mypid;
		string file;

		c_lockfile(string filename,int flag) {
			mypid=getpid();
			file=filename;
			PDEBUG(FLOCK_DEBUG_LEV,"c_lockfile %li: Lock requested: %s (%s)",mypid,filename.c_str(),(flag&WANT_SH_LOCK)?"shared":"exclusive");
			int locklocksfd;
			string locksfile=filename;
			locksfile.append(".locks");
			string locklocksfile=locksfile;
			locklocksfile.append(".locked");

			PDEBUG(FLOCK_DEBUG_LEV,"c_lockfile %li: creating %s",mypid,locklocksfile.c_str());
			while ((locklocksfd=open(locklocksfile.c_str(),O_RDWR|O_CREAT|O_EXCL,0777)) == -1) {
				if (errno==EEXIST)
					sleep(1);
				else
					throw ApplicationExFatal(Ex_INIT,"c_lockfile: open %s (%s)",locklocksfile.c_str(),strerror(errno));
			}
			close(locklocksfd);
			PDEBUG(FLOCK_DEBUG_LEV,"c_lockfile %li: created %s",mypid,locklocksfile.c_str());

			if ((fd=open(locksfile.c_str(),O_RDWR|O_CREAT,0777)) == -1) {
				int savederrno=errno;
				if (unlink(locklocksfile.c_str()) == -1)
					PERROR("c_lockfile %li: unlink %s: %s",mypid,locklocksfile.c_str(),strerror(errno));
				throw ApplicationExFatal(Ex_INIT,"c_lockfile: open %s (%s)",locksfile.c_str(),strerror(savederrno));
			}
			struct flock mylock;
			memset(&mylock,0,sizeof(struct flock));
			mylock.l_type=(flag&WANT_SH_LOCK)?F_RDLCK:F_WRLCK;
			mylock.l_whence=SEEK_SET;//Lock the whole file.
			int ret;
			PDEBUG(FLOCK_DEBUG_LEV,"c_lockfile %li: locking %s (%i)",mypid,locksfile.c_str(),fd);
			while ((ret=fcntl(fd,F_SETLKW,&mylock)) == -1) {
				if (errno!=EINTR) {
					int savederrno=errno;
					if (unlink(locklocksfile.c_str()) == -1)
						PERROR("c_lockfile %li: unlink %s: %s",mypid,locklocksfile.c_str(),strerror(errno));
					throw ApplicationExFatal(Ex_INIT,"c_lockfile: fcntl %s: (%s)",locksfile.c_str(),strerror(savederrno));
				}
			}
			PDEBUG(FLOCK_DEBUG_LEV,"c_lockfile %li: locked %s (%i)",mypid,locksfile.c_str(), fd);

			if (unlink(locklocksfile.c_str()) == -1)
				PERROR("c_lockfile %li: unlink %s: %s",mypid,locklocksfile.c_str(),strerror(errno));
			else
				PDEBUG(FLOCK_DEBUG_LEV,"c_lockfile %li: unlinked %s",mypid,locklocksfile.c_str());
		}
		~c_lockfile(){
			if (fd>=0)
				close(fd);
			PDEBUG(FLOCK_DEBUG_LEV,"~c_lockfile %li: unlocked (%i)",mypid,fd);

			// At this point in time, another process may remove
			// the "emptied of locks" locks file!  Therefore, we
			// absolutely must lock, then reopen, the locks file
			// before checking it, so we don't end up checking a
			// file that had already been unlinked, then unlinking
			// a live locks file.

			int locklocksfd;
			string locksfile=file;
			locksfile.append(".locks");
			string locklocksfile=locksfile;
			locklocksfile.append(".locked");
			PDEBUG(FLOCK_DEBUG_LEV,"~c_lockfile %li: creating %s",mypid,locklocksfile.c_str());
			while ((locklocksfd=open(locklocksfile.c_str(),O_RDWR|O_CREAT|O_EXCL,0777)) == -1) {
				if (errno==EEXIST)
					sleep(1);
				else
					throw ApplicationExFatal(Ex_INIT,"c_lockfile: open %s (%s)",locklocksfile.c_str(),strerror(errno));
			}
			close(locklocksfd);
			PDEBUG(FLOCK_DEBUG_LEV,"~c_lockfile %li: created %s",mypid,locklocksfile.c_str());

			// Note, no O_CREAT. If the locks file has already been
			// emptied of locks and unlinked, we're nearly done.
			if ((fd=open(locksfile.c_str(),O_RDWR)) == -1) {
				if (errno!=ENOENT) {
					int savederrno=errno;
					if (unlink(locklocksfile.c_str()) == -1)
						PERROR("c_lockfile %li: unlink %s: %s",mypid,locklocksfile.c_str(),strerror(errno));
					throw ApplicationExFatal(Ex_INIT,"c_lockfile: open %s (%s)",locksfile.c_str(),strerror(savederrno));
				}
			}
			if (fd>=0) {
				struct flock mylock;
				memset(&mylock,0,sizeof(struct flock));
				mylock.l_type=F_WRLCK;
				mylock.l_whence=SEEK_SET;
				PDEBUG(FLOCK_DEBUG_LEV,"~c_lockfile %li: checking (%i)",mypid,fd);
				if (fcntl(fd,F_GETLK,&mylock) == -1)
					throw ApplicationExFatal(Ex_INIT,"~c_lockfile: fcntl %i: (%s)",fd,strerror(errno));
				close(fd);
				if (mylock.l_type==F_UNLCK) {
					if (unlink(locksfile.c_str()) == -1)
						PERROR("~c_lockfile %li: unlink %s: %s",mypid,locksfile.c_str(),strerror(errno));
					else
						PDEBUG(FLOCK_DEBUG_LEV,"~c_lockfile %li: unlinked %s",mypid,locksfile.c_str());
				} else {
					PDEBUG(FLOCK_DEBUG_LEV,"~c_lockfile %li: %s locked by %li",mypid,file.c_str(),(long)mylock.l_pid);
				}
			}

			if (unlink(locklocksfile.c_str()) == -1)
				PERROR("~c_lockfile %li: unlink %s: %s",mypid,locklocksfile.c_str(),strerror(errno));
			else
				PDEBUG(FLOCK_DEBUG_LEV,"~c_lockfile %li: unlinked %s",mypid,locklocksfile.c_str());
		}
};
#else
#warning building without any sort of locking at all
class c_lockfile{
	public:
		c_lockfile(string filename,int flag){}
		~c_lockfile(){}
};
#endif


#endif
