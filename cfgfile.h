/*
    cfgfile.h - hierarchical config file handling
    Copyright (C) 2003  Matthew Mueller <donut AT dakotacom.net>

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
#ifndef _CFGFILE_H_
#define _CFGFILE_H_


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string>
#include <map>
#include "file.h"
#include "misc.h"

class CfgBase {
	protected:
		mutable bool used;
	public:
		string loc;
		string key;
		
		CfgBase(string l, string k): used(false), loc(l), key(k) { }
		string name(void) const {
			return loc+'/'+key;
		}
		bool isused(void) const {
			return used;
		}
};

class CfgItem : public CfgBase{
	protected:
		string val;
		
	public:
		
		CfgItem(string l, string k, string v): CfgBase(l,k), val(v) { }
		
		const string &gets(void) const {
			used=true;
			return val;
		}

		template <class T>
		bool get(T &dest) const {
			used=true;
			try {
				parsestr(val, dest);
				return true;
			} catch (parse_error &e) {
				PERROR("%s: %s", name().c_str(), e.what());
				set_user_error_status();
			}
			return false;
		}

		template <class T>
		bool get(T &dest, T min, T max) const {
			used=true;
			try {
				parsestr(val, dest, min, max);
				return true;
			} catch (parse_error &e) {
				PERROR("%s: %s", name().c_str(), e.what());
				set_user_error_status();
			}
			return false;
		}
};

typedef map<string, CfgItem*> CfgItem_map ;
class CfgSection;
typedef map<string, CfgSection*> CfgSection_map ;

class CfgSection : public CfgBase{
	protected:
		CfgItem_map items;
		CfgSection_map sections;

		void load(c_file *f, int &level);
	public:

		CfgSection(const string &filename);
		CfgSection(string l, string k, c_file *f, int &level) : CfgBase(l,k) {
			load(f, level);
		}
		~CfgSection();

		int check_unused(void) const;

		CfgSection_map::const_iterator sections_begin(void) const {
			used=true;
			return sections.begin();
		}
		CfgSection_map::const_iterator sections_end(void) const {
			return sections.end();
		}
		CfgItem_map::const_iterator items_begin(void) const {
			used=true;
			return items.begin();
		}
		CfgItem_map::const_iterator items_end(void) const {
			return items.end();
		}

		const CfgItem *getitem(const char *name) const;
		const CfgSection *getsection(const char *name) const;

		string gets(const char *name, const string &defval="") const {
			const CfgItem *i = getitem(name);
			return i ? i->gets() : defval;
		}

		const char *geta(const char *name, const char *defval=NULL) const {
			const CfgItem *i = getitem(name);
			return i ? i->gets().c_str() : defval;
		}

		// need a special defination for strings, since >> string only reads one token
		void get(const char *name, string &dest) const {
			const CfgItem *i = getitem(name);
			if (!i) return;
			dest = i->gets();
		}

		template <class T>
		bool get(const char *name, T &dest) const {
			const CfgItem *i = getitem(name);
			if (!i) return false;
			return i->get(dest);
		}
		template <class T>
		bool get(const char *name, T &dest, T min, T max) const {
			const CfgItem *i = getitem(name);
			if (!i) return false;
			return i->get(dest,min,max);
		}
		template <class T>
		bool get(const char *name, T &dest, T min, T max, T defval) const {
			if (get(name,dest,min,max))
				return true;
			dest=defval;
			return false;
		}
};


#endif
