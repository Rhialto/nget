/*
    cfgfile.h - hierarchical config file handling
    Copyright (C) 1999-2002  Matthew Mueller <donut AT dakotacom.net>

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
#ifdef HAVE_LIMITS
#include <limits>
#endif
#include "_sstream.h"
#include "file.h"
#include "misc.h" // for tostr

class CfgBase {
	public:
		string loc;
		string key;
		
		CfgBase(string l, string k): loc(l), key(k) { }
		string name(void) const {
			return loc+'/'+key;
		}
};

class CfgItem : public CfgBase{
	protected:
		string val;
		
		bool issok(istringstream &iss) const;

		template <class T>
		bool doparse(T &v) const {
			istringstream iss(val);
			iss >> v;
			return issok(iss);
		}

		template <class T>
		bool doget(T &v) const {
			if (val.empty()) {
				PERROR("%s: empty val", name().c_str());
				set_user_error_status();
				return false;
			}
			return doparse(v);
		}
	public:
		
		CfgItem(string l, string k, string v): CfgBase(l,k), val(v) { }
		
		const string &gets(void) const { return val; }

		template <class T>
		void get(T &dest) const {
			T v;
			if (doget(v))
				dest = v;
		}

		template <class T>
		bool get(T &dest, T min, T max) const {
#ifdef HAVE_LIMITS
			if (!numeric_limits<T>::is_signed && val.find('-')!=string::npos) {
				PERROR("%s: invalid unsigned value", name().c_str());
				set_user_error_status();
				return false;
			}
#endif
			T v;
			if (!doget(v))
				return false;
			if (v<min || v>max) {
				PERROR("%s: not in range %s..%s", name().c_str(), tostr(min).c_str(), tostr(max).c_str());
				set_user_error_status();
				return false;
			}
			dest = v;
			return true;
		}
};

typedef map<string, CfgItem*> CfgItem_map ;
class CfgSection;
typedef map<string, CfgSection*> CfgSection_map ;

class CfgSection : public CfgBase{
	protected:
		CfgItem_map items;
		CfgSection_map sections;

		void load(c_file *f);
	public:

		CfgSection(const string &filename) : CfgBase(filename,"") {
			c_file_fd f(filename.c_str(),O_RDONLY);
			f.initrbuf();
			load(&f);
			f.close();
		}
		CfgSection(string l, string k, c_file *f) : CfgBase(l,k) {
			load(f);
		}
		~CfgSection();

		CfgSection_map::const_iterator sections_begin(void) const { return sections.begin(); }
		CfgSection_map::const_iterator sections_end(void) const { return sections.end(); }
		CfgItem_map::const_iterator items_begin(void) const { return items.begin(); }
		CfgItem_map::const_iterator items_end(void) const { return items.end(); }

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
		void get(const char *name, T &dest) const {
			const CfgItem *i = getitem(name);
			if (!i) return;
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
