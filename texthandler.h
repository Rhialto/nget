/*
    texthandler.* - text post handler
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
#ifndef NGET_TEXTHANDLER_H__
#define NGET_TEXTHANDLER_H__
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <list>
#include <string>

#include "nget.h"
#include "cache.h"

class TextHandler {
	protected:
		t_text_handling texthandling;
		bool save_text_for_binaries;
		string mboxname;
		c_nntp_file_retr::ptr fr;
		list<string> info;
		string firsttempfn;
		int infocount;
		int decodeinfocount;

		void writeinfo(c_file *f, bool escape_From);
	public:
		void addinfo(const string &str);
		void adddecodeinfo(const string &str);
		TextHandler(t_text_handling texthandlin, bool save_text_for_binarie, const string &mboxname, c_nntp_file_retr::ptr frp, const char *firsttempfn);
		~TextHandler();
};
#endif
