/*
    _sstream.h - wrapper around old ostrstream class to emulate ostringstream
    Copyright (C) 2001  Matthew Mueller <donut AT dakotacom.net>

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
#ifndef __SSTREAM_H_
#define __SSTREAM_H_

#ifdef HAVE_SSTREAM
//If we have the new sstream, we don't need to do anything.
#include <sstream>
#else
//Otherwise, create a wrapper class.
#include <strstream>
#include <string>

class ostringstream : public ostrstream {
	public:
		string str(void) {return string(ostrstream::str(), pcount());} //ostrstream::str() is not null terminated.
};
#endif

#endif
