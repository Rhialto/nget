/*
    _hash_map.h - handle the different types of hash_map extensions
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
#ifndef __HASH_MAP_H_
#define __HASH_MAP_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define USE_HASH_MAP 1
#ifdef HAVE_HASH_MAP
#include <hash_map>
#elif HAVE_EXT_HASH_MAP
#include <ext/hash_map>
using namespace __gnu_cxx;
#elif HAVE_HASH_MAP_H
#include <hash_map.h>
#else
#undef USE_HASH_MAP
#include <map>
#endif

#endif
