/*
    nrange.* - stores a set of numbers in a non-memory-hogging way (and speedy too?)
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
#ifndef _NRANGE_H_
#define _NRANGE_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string>
#include <sys/types.h>
#include <map>
#include <stdio.h>
#include "file.h"


typedef map<ulong, ulong> t_rlist;//we are going to use high as the key, and low as the value
class c_nrange{
	public:
		static const ulong varmin=0;
		static const ulong varmax=ULONG_MAX;
		int changed;
		string file;
		t_rlist rlist;
		ulong get_total(void) const {
			ulong tot=0;
			for (t_rlist::const_iterator i=rlist.begin();i!=rlist.end();++i)
				tot += i->first - i->second + 1;
			return tot;
		}
		t_rlist::size_type num_ranges(void) const {return rlist.size();}
		bool empty(void) const {return rlist.empty();}
		bool check(ulong n) const {
			t_rlist::const_iterator i=rlist.lower_bound(n);
			if (i!=rlist.end() && (*i).second<=n)
				return true;
			return false;
		}
		void insert(ulong n);
		void insert(ulong l,ulong h);
		void remove(ulong n){remove(n,n);}
		void remove(ulong l, ulong h);
		void print(c_file *f) const;
		void clear(void){
			if (!rlist.empty()){
				rlist.erase(rlist.begin(),rlist.end());
				changed=1;
			}
		}
		void load(string f,int merge=0);
		void save(void);
		c_nrange(c_nrange &r):changed(r.changed),rlist(r.rlist){}
		c_nrange(string f="");
		~c_nrange();
		bool operator==(const c_nrange &b) const {return rlist==b.rlist;}
		bool operator!=(const c_nrange &b) const {return rlist!=b.rlist;}
};

#endif
