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
				throw new c_error(EX_A_FATAL,"lockfile_create: %i (%i)",ret,errno);
			}
			PDEBUG(FLOCK_DEBUG_LEV,"locked %s",file.c_str());
		}
		~c_lockfile(){
			lockfile_remove(file.c_str());
			PDEBUG(FLOCK_DEBUG_LEV,"unlocked %s",file.c_str());
		}
};
#else //!HAVE_LIBLOCKFILE
#ifdef HAVE_FLOCK
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
					throw ApplicationExFatal(Ex_INIT,"c_lockfile: open: %i (%i)",fd,errno);
			}
			int ret=flock(fd,(flag&WANT_SH_LOCK)?LOCK_SH:LOCK_EX);
			if (ret)
				throw ApplicationExFatal(Ex_INIT,"c_lockfile: flock: %i (%i)",ret,errno);
			PDEBUG(FLOCK_DEBUG_LEV,"flocked %s (%i)",filename.c_str(),fd);
//			sleep(10);
		}
		~c_lockfile(){
			flock(fd,LOCK_UN);
			close(fd);
			PDEBUG(FLOCK_DEBUG_LEV,"unflocked %i",fd);
		}
};
#else //!HAVE_FLOCK
#warning building without any sort of locking at all
class c_lockfile{
	public:
		c_lockfile(string filename,int flag){}
		~c_lockfile(){}
};
#endif //!HAVE_FLOCK
#endif //!HAVE_LIBLOCKFILE


#endif
