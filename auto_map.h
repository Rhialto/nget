/*
    auto_map.h - map that manages its contents
    Copyright (C) 2003  Matthew Mueller <donut@azstarnet.com>

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
#ifndef AUTO_MAP_H__
#define AUTO_MAP_H__

#include "rcount.h"
#include <map>

template <class K, class T>
class auto_map : public std::map<K, restricted_ptr<T> > {
	private:
		typedef std::map<K, restricted_ptr<T> > super;
	public:
		typedef typename super::iterator iterator;

		/*super::value_type value_type(const K &k, T*p) {
			return super::value_type(k, restricted_ptr<T>(p));
		}
		pair<iterator, bool> insert(const super::value_type &v) {
			assert(find(v.first)==end());
			return super::insert(v);
		}*/
		pair<iterator, bool> insert_value(const K &k, T* p) { //we can't really use the normal insert funcs, but we don't want to just name it insert since it would be easy to confuse with all the normal map insert funcs
			assert(find(k)==end());
			return super::insert(super::value_type(k, restricted_ptr<T>(p)));
		}

		void clear(void){
			while (!empty())
				erase(begin());
		}
		void erase(iterator i){
			T* p=(*i).second.gimmethepointer();
			super::erase(i);
			delete p;
		}
		auto_map(void){}
		~auto_map() {
			clear();
		}
	private:
		void insert(void); //private insert func to hide std::map's insert members
		void delete_all(void) {
			for (iterator i=begin(); i!=end(); ++i)
				delete (*i).gimmethepointer();
		}
		auto_map(const auto_map &v); //private copy constructor to disallow copying
		auto_map& operator= (const auto_map &m); //private operator= to disallow assignment
};

#endif
