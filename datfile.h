/*
    datfile.h - easy config/etc file handling
    Copyright (C) 1999-2001  Matthew Mueller <donut@azstarnet.com>

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
#ifndef _DATFILE_H_
#define _DATFILE_H_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string>
#include "_hash_map.h"
#include <stdio.h>
#include "stlhelp.h"
#include "file.h"

class c_data_item
{
	public:
		string key;
		string str;
		long i;
		int ierr;//0=no error;-1=error after converting some;-2=not at all;-3=never been set
		int type;
		c_data_item(const char *k):key(k)
		{
			ierr=-3;i=0;
			type=0;
		};
		c_data_item(const char *k, const char *s):key(k)
		{
			set(s);
			type=0;
		};
		c_data_item(const char *k, long l):key(k)
		{
			set(l);
			type=0;
		};
		void set(long l)
		{
			str="";
			i=l;
			ierr=0;
		};
		void set(const char *val);
/*		bool compareval(c_data_item &it){
			if (it.type==0 && type==0 && it.ierr==ierr){
				switch(ierr){
					case 0:
						if (i==it.i) return 1;
						break;
					case -3:
						return 1;
					default:
						if (str==it.str)return 1;
				}
			}
			return 0;
		}
		bool compareall(c_data_item &it){
			if (compareval(it) && key==it.key)return 1;
			return 0;
		}*/
		virtual ~c_data_item(){}
};

#ifdef USE_HASH_MAP
typedef hash_map<const char*, c_data_item*, hash<const char*>, eqstr> data_list ;
#else
typedef map<const char*, c_data_item*, ltstr> data_list ;
#endif
//typedef map<const char*, T_Data_Item_List, ltstr> T_Data_List ;

class c_data_section: public c_data_item
{
  public:
	data_list data;
/*	int getiteml(const char *name,long *l){
		c_data_item *i=rgetitem(name);
		if (i && i->type==0 && i->ierr==0) {*l=i->i;return 0;}
		return -1;
	};*/
	int getitemi(const char *name,int *l){
		c_data_item *i=rgetitem(name);
		if (i && i->type==0 && i->ierr==0) {*l=i->i;return 0;}
		return -1;
	};
	int geti(const char *name,int err=-1){
		c_data_item *i=rgetitem(name);
		if (i && i->type==0 && i->ierr==0) {return i->i;}
		return err;
	};
	int getitemul(const char *name,unsigned long *l){
		c_data_item *i=rgetitem(name);
		if (i && i->type==0 && i->ierr==0) {*l=i->i;return 0;}
		return -1;
	};
	int getitems(string name, string *dest){
		c_data_item *i=rgetitem(name.c_str());
		if (i && i->type==0) {*dest = i->str; return 0;}
		return -1;
	};
	string getitems(string name){
		c_data_item *i=rgetitem(name.c_str());
		if (i && i->type==0) return i->str;
		return "";
	};
	const char *getitema(const char *name){
		c_data_item *i=rgetitem(name);
		if (i && i->type==0) return i->str.c_str();
		return NULL;
	};
	c_data_item *getitem(const char *name){
		c_data_item *i=rgetitem(name);
		if (i && i->type==0) return i;
		return NULL;
	};
	c_data_section *getsection(const char *name){
		c_data_item *i=rgetitem(name);
		if (i && i->type==1) return (c_data_section*) i;
		return NULL;
	};
	c_data_item *rgetitem(const char *name);
	c_data_item *additem(c_data_item *item);
	int delitem(data_list::iterator i);
	int delitem(const char *name){
		return delitem(data.find(name));
	}

	void read_list(c_file *f);
	void save_list(int &r,FILE *f,const char *cname,const char*termin="\n");
	void cleanup(void);
	c_data_section(const char *k):c_data_item(k)
	{
//	    data=new data_list;
//	    data["default"]=new c_data_item("test");
		//ierr=-3;
		type=1;
	};
	virtual ~c_data_section()
	{
		cleanup();
	};
};
//typedef hash_map<const char*, data_section*, hash<const char*>, eqstr> t_data_list ;

//typedef map<const char*, T_Data_Item_List, ltstr> T_Data_File ;
class c_data_file
{
//    FILE *f;
//    t_data_file data;
	int changed;
  public:
	string filename;
	c_data_section data;

	void setfilename(const char * f);

	int read(void);
	void save(void);


//    c_data_section* find (char*section);
//    c_data_section* find (char*format,...);

	c_data_file(void):data(""){}
//    c_data_section* findif(Predicate Pred)
//	{
//	    data_list::iterator i=find_if(data.begin(),data.end(),Pred);
//	    if (i==data.end())return NULL;
//	    else return (*i).second;	    
//	};

//    ~c_data_file()
//	{
//	    if (changed) save();
//	    data.cleanup();
//	};

//    data_section* findif(char *name, char*val);
//    data_section* findifa(char *name, char*val,char *name2, char*val2);
};
#endif
