/*
    knapsack.* - 0-1 knapsack algorithm, and variants
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
#include "knapsack.h"
#include <numeric>
#include <limits.h>

int knapsack(const vector<int> &values, const vector<int> &sizes, int target_size, set<int> &result) {
	assert(result.empty());
	assert(values.size() == sizes.size());
	int *V = new int[target_size+1];
	vector<int> *best = new vector<int>[target_size+1];
	int i,j;
	const int n = values.size();
	for (i=0; i<=target_size; ++i){
		V[i]=0;
	}
	
	for (i=0; i<n; ++i) {
		for (j=target_size; j>=sizes[i]; --j) {
			int othersol = V[j - sizes[i]] + values[i];
			if (othersol > V[j]) {
				V[j] = othersol;
				best[j].push_back(i);
			}
		}
	}
	const int result_value = V[target_size];
	
#ifndef NDEBUG
	i=0;
#endif
	j = target_size;
	int last = INT_MAX;
	while (j>0) {
		vector<int>::reverse_iterator bi, bend=best[j].rend();
		for (bi=best[j].rbegin(); bi!=bend; ++bi) {
			// At the point in time when a value is entered into the best array,
			// it is valid only for it and the preceeding values that have been
			// calculated.  So if we have used the result of a value X, then any
			// further results we use can only be from values before X.
			if (*bi<last) {
				last=*bi;
#ifndef NDEBUG
				i += values[*bi];
#endif
				result.insert(*bi);
				j -= sizes[*bi];
				break;
			}
		}
		if (bi==bend)
			break;
	}
	assert(j >= 0);//== assert(sum([sizes[i] for i in result]) <= target_size)
	assert(i == result_value);
	
	delete[] V;
	delete[] best;

	return result_value;
}

// In order to find the minimum size with at least target_value, we
// find the max size in the values we _don't_ want, then invert
// the result.
int knapsack_minsize(const vector<int> &values, const vector<int> &sizes, int target_value, set<int> &result) {
	const int sum_values = accumulate(values.begin(), values.end(), 0);
	const int sum_sizes = accumulate(sizes.begin(), sizes.end(), 0);
	const int targetprime = sum_values - target_value;
	set<int> resultprime;
	int result_sizeprime = 0;
	if (targetprime>=0) {
		result_sizeprime = knapsack(sizes, values, targetprime, resultprime);
	}
	for (unsigned int i=0; i<values.size(); ++i) {
		if (!resultprime.count(i))
			result.insert(i);
	}
	return sum_sizes - result_sizeprime;
}
