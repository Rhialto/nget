/*
    knapsack.* - 0-1 knapsack algorithm, and variants
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
#ifndef NGET__KNAPSACK_H__
#define NGET__KNAPSACK_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <set>
#include <vector>

// find the maximum value that can fit in target_size
// runs in O(n*target_size)
int knapsack(const vector<int> &values, const vector<int> &sizes, int target_size, set<int> &result);

// find the minimum size with at least target_value
// runs in O(n*(sum(values)-target_value))
int knapsack_minsize(const vector<int> &values, const vector<int> &sizes, int target_value, set<int> &result);

#endif
