/*
    etree.* - handles expression trees.. idea for these object types came
	from the Standard Template Library, but unfortunatly its implementation
	just couldn't handle what I need to do. (or at least I couldn't figure
	out how to make it)

    Copyright (C) 1999  Matthew Mueller <donut@azstarnet.com>

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
#ifndef _ETREE_H_
#define _ETREE_H_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
typedef unsigned char ubyte;

//typedef c_btree<c_exp> etree;

template <class Arg, class Ret>
struct e_unary_function {
	typedef Arg arg_type;
	typedef Ret ret_type;

	virtual Ret operator()(const Arg) const=0;
	virtual ~e_unary_function(){}
};
template <class Arg1, class Arg2, class Ret>
struct e_binary_function {
	typedef Arg1 arg1_type;
	typedef Arg2 arg2_type;
	typedef Ret ret_type;

	virtual Ret operator()(const Arg1,const Arg2) const=0;
};

//template <class T>
//struct e_land : public e_binary_function<T, T, bool> {
//	bool operator()(const T v1, const T v2) const {return v1 && v2;}
//};
template <class T>
struct e_lnot : public e_unary_function<T, T> {
	T operator()(const T v) const {return !v;}
};
#define MAKE_BINARY_FUNC(name,op) template <class T,class T2=T>				\
struct e_ ## name : public e_binary_function<T, T2, bool> {			\
	bool operator()(const T v1,const T2 v2) const {return v1 op v2;}	\
};
MAKE_BINARY_FUNC(gt,>);
MAKE_BINARY_FUNC(ge,>=);
MAKE_BINARY_FUNC(lt,<);
MAKE_BINARY_FUNC(le,<=);
MAKE_BINARY_FUNC(eq,==);
MAKE_BINARY_FUNC(ne,!=);
MAKE_BINARY_FUNC(land,&&);
MAKE_BINARY_FUNC(lor,||);
//template <class T>
//struct e_ge : public e_binary_function<T, T, bool> {
//	bool operator()(const T v1,const T v2) const {return v1>=v2;}
//};

//turn Op(a,b) into unary func this(a)
template <class Op>
struct e_binder2nd : public e_unary_function<typename Op::arg1_type, typename Op::ret_type> {
	typedef typename Op::arg2_type arg2_type;
	arg2_type val2;
	Op O;

	e_binder2nd(const arg2_type v):val2(v){}
	virtual ~e_binder2nd(){}
	ret_type operator()(const arg_type v) const {return O(v,val2);}
};
template <class Op>
struct e_binder2nd_p : public e_binder2nd<Op> {
	inline e_binder2nd_p(const arg2_type v):e_binder2nd<Op>(v){}
	virtual ~e_binder2nd_p(){delete val2;}
};

//compose op1,op2,op3 into op1(op2(v),op3(v))
template <class T,class UR,class BR>
struct e_binary_compose : public e_unary_function<T, BR> {
	typedef e_binary_function<UR,UR,BR> Op1;
	typedef e_unary_function<T,UR> Op2;
	const Op1 *O1;
	const Op2 *O2,*O3;

	e_binary_compose(const Op1 *a,const Op2 *b, const Op2 *c):O1(a),O2(b),O3(c){};
	virtual ~e_binary_compose(){delete O1; delete O2; delete O3;}
	ret_type operator()(const arg_type v) const {return (*O1)((*O2)(v),(*O3)(v));}

};

//convenience function, puts the template stuff for you
template <class Operation1, class Operation2, class Operation3>
inline e_binary_compose<typename Operation2::arg_type, typename Operation2::ret_type, typename Operation3::ret_type> *
ecompose2(const Operation1 *op1, const Operation2 *op2, const Operation3 *op3) {
	return new e_binary_compose<typename Operation2::arg_type, typename Operation2::ret_type, typename Operation3::ret_type>(op1, op2, op3);
};

class c_nntp_file;
typedef e_unary_function<const c_nntp_file*,bool> generic_pred;
generic_pred * make_pred(const char *optarg, int gflags);

#endif
