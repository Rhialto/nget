/*
    decode.* - uu/yenc/etc decoding (wrapper around uulib)
    Copyright (C) 1999-2003  Matthew Mueller <donut AT dakotacom.net>

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
#ifndef __NGET__DECODE_H__
#define __NGET__DECODE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string>
#include <list>
#include "nget.h"
#include "dupe_file.h"
#include "cache.h"

class Decoder {
	protected:
		typedef list<pair<int,char*> > t_fnbuf_list;
		t_fnbuf_list fnbuf;
	public:
		void addpart(int partno, char *fn);//takes ownership of fn.
		int decode(const nget_options &options, const c_nntp_file_retr::ptr &fr, dupe_file_checker &flist);
		void delete_tempfiles(void);
		~Decoder();
};


#endif
