/*
    auto_vector.h - vector that manages its contents
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
#ifndef AUTO_VECTOR_H__
#define AUTO_VECTOR_H__

#include "rcount.h"
#include <vector>

template <class T>
class auto_vector : public std::vector<restricted_ptr<T> > {
	private:
		typedef std::vector<restricted_ptr<T> > super;
	public:
		typedef typename super::iterator iterator;
		
		void push_back(T* v) {super::push_back(restricted_ptr<T>(v));}

		void clear(void){
			delete_all();
			super::clear();
		}
		void pop_back(void) {
			delete this->back().gimmethepointer();
			super::pop_back();
		}
		iterator erase(iterator i){
			delete (*i).gimmethepointer();
			return super::erase(i);
		}
		auto_vector(void){}
		~auto_vector() {
			delete_all();
		}
	private:
		void delete_all(void) {
			for (iterator i=this->begin(); i!=this->end(); ++i)
				delete (*i).gimmethepointer();
		}
		void insert(void); //private insert func to hide std::vector's insert members
		auto_vector(const auto_vector &v); //private copy constructor to disallow copying
		auto_vector& operator= (const auto_vector &v); //private operator= to disallow assignment
};

#endif
