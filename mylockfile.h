/*
    mylockfile.* - c++ wrapper for file locks (for easy unlocking due to destructors, and abstraction from lowlevel)
    Copyright (C) 1999-2000  Matthew Mueller <donut@azstarnet.com>

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
				throw ApplicationExFatal(Ex_INIT,"lockfile_create %s: %i (%s)",filename.c_str(),ret,strerror(errno));
			}
			PDEBUG(FLOCK_DEBUG_LEV,"locked %s",file.c_str());
		}
		~c_lockfile(){
			lockfile_remove(file.c_str());
			PDEBUG(FLOCK_DEBUG_LEV,"unlocked %s",file.c_str());
		}
};
#elif HAVE_FLOCK
#include <unistd.h>
#include <sys/file.h>
class c_lockfile{
	public:
		int fd;

		c_lockfile(string filename,int flag){
			PDEBUG(FLOCK_DEBUG_LEV,"attempting to flock %s",filename.c_str());
			fd=open(filename.c_str(),O_RDONLY);
			if (fd<0){
				if (errno==ENOENT){
//					if (flag&WANT_SH_LOCK)
					return;
				} else
					throw ApplicationExFatal(Ex_INIT,"c_lockfile: open %s (%s)",filename.c_str(),strerror(errno));
			}
			int ret=flock(fd,(flag&WANT_SH_LOCK)?LOCK_SH:LOCK_EX);
			if (ret)
				throw ApplicationExFatal(Ex_INIT,"c_lockfile: flock %s: %i (%s)",filename.c_str(),ret,strerror(errno));
			PDEBUG(FLOCK_DEBUG_LEV,"flocked %s (%i)",filename.c_str(),fd);
//			sleep(10);
		}
		~c_lockfile(){
			if (fd>=0) {
				flock(fd,LOCK_UN);
				close(fd);
				PDEBUG(FLOCK_DEBUG_LEV,"unflocked %i",fd);
			}
		}
};
#elif HAVE_LOCKFILE
#include <windows.h>
#include "path.h"
class c_lockfile{
	public:
		HANDLE hFile;

		c_lockfile(string filename,int flag): hFile(INVALID_HANDLE_VALUE){
			PDEBUG(FLOCK_DEBUG_LEV,"attempting to LockFile %s",filename.c_str());
			if (!fexists(filename.c_str()))
				return;
			hFile = CreateFile(filename.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
			if (hFile==INVALID_HANDLE_VALUE){
				throw ApplicationExFatal(Ex_INIT,"c_lockfile: open %s (%i)",filename.c_str(),GetLastError());
			}
			int tries=1;
			while (!LockFile(hFile,0,0,0xFFFFFFFF,0)) {
				if (tries++ >= 10)
					throw ApplicationExFatal(Ex_INIT,"c_lockfile: LockFile %s: giving up after %i tries",filename.c_str(), tries);
				sleep(1);
				PDEBUG(FLOCK_DEBUG_LEV,"try %i LockFile %s",tries,filename.c_str());
			}
			PDEBUG(FLOCK_DEBUG_LEV,"locked %s (%p)",filename.c_str(),hFile);
		}
		~c_lockfile(){
			if (hFile!=INVALID_HANDLE_VALUE) {
				UnlockFile(hFile,0,0,0xFFFFFFFF,0);
				CloseHandle(hFile);
				PDEBUG(FLOCK_DEBUG_LEV,"unlocked %p",hFile);
			}
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
