/*
    rcount.h - classes for easy object refcounting
    Copyright (C) 2000-2003  Matthew Mueller <donut AT dakotacom.net>

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
#ifndef RCOUNT_H_INCD
#define RCOUNT_H_INCD

#include <stdlib.h>

template <class T>
class c_refpointer {
	public:
//		operator T* () { return p_; }
		T* gimmethepointer() {return p_;}
		const T* gimmethepointer() const {return p_;}
		bool isnull(void) const {return p_==NULL;}
		operator bool() const {return p_!=NULL;}
		bool operator< (const c_refpointer& p) const {return p_<p.p_;}
		bool operator> (const c_refpointer& p) const {return p_>p.p_;}
		bool operator<= (const c_refpointer& p) const {return p_<=p.p_;}
		bool operator>= (const c_refpointer& p) const {return p_>=p.p_;}
		bool operator== (const c_refpointer& p) const {return p_==p.p_;}
		bool operator!= (const c_refpointer& p) const {return p_!=p.p_;}
		T* operator-> () { return p_; }
		const T* operator-> () const { return p_; }
		T& operator* ()  { return *p_; }
		const T& operator* () const { return *p_; }
		c_refpointer(T* p)    : p_(p) { if(p_) ++p_->count_; }  // p must not be NULL
		~c_refpointer()           { if (p_) {if (--p_->count_ == 0) delete p_;} }
		c_refpointer(void) : p_(NULL) {}
		c_refpointer(const c_refpointer& p) : p_(p.p_) { if (p_) ++p_->count_; }
		c_refpointer& operator= (const c_refpointer& p)
		{ // DO NOT CHANGE THE ORDER OF THESE STATEMENTS!
			// (This order properly handles self-assignment)
			if (p.p_)
				++p.p_->count_;
			if (p_)
				if (--p_->count_ == 0) delete p_;
			p_ = p.p_;
			return *this;
		}
	private:
		T* p_;
};
template <class T>
class c_refcounted {
	public:
		c_refcounted(void):count_(0){}
		friend class c_refpointer<T>;
		typedef c_refpointer<T> ptr;
	protected:
		unsigned int count_;
};


template <class T>
class restricted_ptr {
	private:
		T *p_;
	public:
		operator bool() const {return p_!=NULL;}
		T* operator-> () { return p_; }
		const T* operator-> () const { return p_; }
		T& operator* ()  { return *p_; }
		const T& operator* () const { return *p_; }

		T* gimmethepointer() {return p_;}
		const T* gimmethepointer() const {return p_;}
		explicit restricted_ptr(T *p):p_(p){} //explicit constructor keeps you from accidentally trying to assign to it, yet allows stl containers to copy around internally with the copy constructor and operator=
};

#endif
