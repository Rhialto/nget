/*
    getter.h - classes to get a piece of data out of objects
    Copyright (C) 2002  Matthew Mueller <donut@azstarnet.com>

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
#ifndef __GETTER_H__
#define __GETTER_H__

template <class RetType, class ClassType>
class MemGetter {
	private:
		typedef RetType ClassType::*member_t;
		member_t member;		
	public:
		//typedef const RetType &T; //may be faster for some types? maybe should be a seperate MemrefGetter class.
		typedef RetType T;
		typedef member_t init_t;
		MemGetter(init_t i): member(i) {}
		T operator()(ClassType *o) const {return o->*member;}
};

template <class RetType, class ClassType>
class MemfuncGetter {
	private:
		typedef RetType (ClassType::*member_t)(void);
		member_t member;		
	public:
		typedef RetType T;
		typedef member_t init_t;
		MemfuncGetter(init_t i): member(i) {}
		T operator()(ClassType *o) const {return (o->*member)();}
};

template <class RetType, class ClassType>
class FuncGetter {
	private:
		typedef RetType (*func_t)(ClassType *o);
		func_t func;		
	public:
		typedef RetType T;
		typedef func_t init_t;
		FuncGetter(init_t i): func(i) {}
		T operator()(ClassType *o) const {return func(o);}
};

/*
template <class RetType, class ClassType, class getter_t>
class Getter {
	private:
		getter_t getter;
	public:
		typedef getter_t init_t;
		Getter(init_t i): getter(i) {}
		RetType operator()(ClassType *o);
};

//func getter
template <class RetType, class ClassType>
class Getter<RetType, ClassType, RetType (*)(ClassType *)> {
	public:
		RetType operator()(ClassType *o) {return getter(o);}
};
//member func getter
template <class RetType, class ClassType>
class Getter<RetType, ClassType, RetType (ClassType::*)(void)> {
	public:
		RetType operator()(ClassType *o) {return o->*getter();}
};
//member variable getter
template <class RetType, class ClassType>
class Getter<RetType, ClassType, RetType (ClassType::*)> {
	public:
		RetType operator()(ClassType *o) {return o->*getter;}
};
*/

#endif
