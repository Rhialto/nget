/*
    nrange.* - stores a set of numbers in a non-memory-hogging way (and speedy too?)
    Copyright (C) 1999-2000,2002  Matthew Mueller <donut@azstarnet.com>

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
#include "nrange.h"
#include "strreps.h"
#include "log.h"
#include "mylockfile.h"

void c_nrange::insert(ulong n){
	t_rlist::iterator i=rlist.lower_bound(n),j;
	if (i!=rlist.end()){
		if ((*i).second<=n)
			return;//already in a range
		if ((*i).second==n+1){
			(*i).second=n;
			//					return;//1 below a range, don't return because we need to check for being 1 above another range
		}else
			i=rlist.end();//no good.
	}
	//changed=1;//if we've gotten here, something will change.
	if (n>varmin){//don't wanna wrap around, since we are using unsigned
		j=rlist.find(n-1);
		if (j!=rlist.end()){
			if (i!=rlist.end())
				(*i).second=(*j).second;//1 above and 1 below two different ranges
			else
				rlist[n]=(*j).second;//only 1 above a range
			rlist.erase(j);
			return;
		}
	}
	if (i==rlist.end())
		rlist[n]=n;//need a new range
}
void c_nrange::insert(ulong l,ulong h){
	if (rlist.empty()){
		rlist[h]=l;
		//changed=1;
		return;
	}
	ulong l1=(l>varmin)?l-1:l;
	ulong h1=(h<varmax)?h+1:h;
	t_rlist::iterator j=rlist.lower_bound(l1);
	if (j==rlist.end()){
		rlist[h]=l;//changed=1;
		return;
	}
	t_rlist::iterator i=rlist.lower_bound(h);
	if (i==j){//if high and low matches find the same range
		if (l<(*j).second){//if low is outside the range:
			if ( (*j).second > h1 )
				rlist[h]=l;//and high is outside too, make a new range
			else
				(*j).second=l;//else extend it
			//changed=1;
		}
		return;
	}
	if ((*j).second<l)
		l=(*j).second;
	rlist.erase(j,i);//erase all the entries that will be encompassed by the new range
	if ((i==rlist.end()) || ( (*i).second > h1 ) ){
		rlist[h]=l;//if we are at the end, or below the one above us, make a new entry
	}else
		(*i).second=l;//else extend the one above us
	//changed=1;
}

void c_nrange::remove(ulong l, ulong h){
	const int debug=0;//hehe.
	if (debug){printf("remove (%lu-%lu) nrange...",l,h);fflush(stdout);}
	if (rlist.empty()){
		if (debug)printf("empty\n");
		return;
	}
	ulong ssize=0,rtot=0,rpart=0;
	int tmp=0,doneflag=0;
	if (debug)ssize=rlist.size();
	t_rlist::iterator i=rlist.lower_bound(h),j;
	if (i==rlist.end())
		i=rlist.lower_bound(l);
//		--i;
	//while (i!=rlist.end()){
	if (i==rlist.end())
		doneflag=1;
	else
		j=i;
	//while (j!=rlist.begin() && i!=rlist.end()){
	while (!doneflag){
		if (j==rlist.begin())
			doneflag=1;
		--i;
		if (h >= (*j).first){
			if (l <= (*j).first) {//remove range cuts off (some or all) of this range
				if (l > (*j).second){
					rlist[l-1]=(*j).second;//remove range cuts off only the top part
					rpart++;
				}else
					rtot++;
				rlist.erase(j);
				//changed=1;
			}else{
//				if (debug)printf("a(%i)\n",ssize-rlist.size());
//				return;//remove range is completely above this range, so we are done
				tmp=2;break;
			}
		}else{
			if (h >= (*j).second) {//remove range cuts off (some) of the lower of this range
				if (l > (*j).second) {
					tmp=1;
					rlist[l-1]=(*j).second;//remove range cuts through the middle of a current range
				}
				(*j).second=h+1;
				//changed=1;
				rpart++;
				if (tmp) break;//its completely within this range, so we are done.
			}
			//					delete j;
		}
		j=i;
	}
	if (debug)printf("%i(%lu tot:%lu part:%lu (%lu->%lu))\n",tmp,ssize-(ulong)rlist.size(),rtot,rpart,ssize,(ulong)rlist.size());
}
