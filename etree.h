/*
    etree.* - handles expression trees.. Allows to create a predicate
        that can then be tested against many objects (of the same ClassType)
        to see if they match the expression.
    Copyright (C) 1999,2002  Matthew Mueller <donut@azstarnet.com>

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
#include "argparser.h"

class c_nntp_file;
template <class ClassType>
struct pred {
	virtual bool operator()(const ClassType*) const=0;
	virtual ~pred(){}
};
typedef pred<const c_nntp_file> nntp_file_pred;
nntp_file_pred * make_pred(const char *optarg, int gflags);
nntp_file_pred * make_pred(const arglist_t &e_parts, int gflags);

#endif
