/*
    datfile.cc - easy config/etc file handling
    Copyright (C) 1999-2002  Matthew Mueller <donut AT dakotacom.net>

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
#include "datfile.h"

void c_data_item::set(const char *val){
	char *err;
	str=val;
	i=strtol(val,&err,0);
	if (err==val) ierr=-2;
	else if ((err-val)<(signed)str.size()) ierr=-1;
	else ierr=0;
}



c_data_item *c_data_section::rgetitem(const char *name){
	data_list::iterator i=data.find(name);
	if (i!=data.end()) return (*i).second;
	return NULL;
}
int c_data_section::delitem(data_list::iterator i){
	if (i!=data.end()){
		delete (*i).second;
		data.erase(i);
		return 1;
	}
	return 0;
}
c_data_item *c_data_section::additem(c_data_item *item){
	data_list::iterator i=data.find(item->key.c_str());
	if (i!=data.end()){
		delete (*i).second;
		data.erase(i);
	}
	data.insert(data_list::value_type(item->key.c_str(),item));
	return item;
}
void c_data_section::cleanup(void){
	data_list::iterator i;
	while (!data.empty()) {
		i=data.begin();
		c_data_item *di = (*i).second;
		data.erase(i);
		delete di;
	}
}

void c_data_section::read_list(c_file *f){
	char *v,*buf;
	int slen;
	while ((slen=f->bgets())>0){
		buf=f->rbufp();
//		slen=strlen(buf);
/*		while (slen>0 && (buf[slen-1]=='\n' || buf[slen-1]=='\r')){
			buf[--slen]=0;
		}*/
		if (slen<1)
			continue;
		buf+=strspn(buf," \t");
		if (!*buf)
			continue;
		if (*buf=='}') {
//			printf("end section\n");
			break;
		}
		if (*buf=='{')
		{
			c_data_section *d=new c_data_section(buf+1);
			//		c->data[buf+r+1]=d;
			additem(d);
//			printf("new section: %s\n",buf+r+1);
			d->read_list(f);
		}
		else if (*buf=='#')
			;//ignore
		else if (buf[0]=='/' && buf[1]=='/')
			;//ignore
		else
		{
			if((v=strchr(buf,'='))){
				*v=0;
				v++;
				additem(new c_data_item(buf,v));
//				printf("new item: %s=%s\n",n,v);
			}
		}
	}
}
void c_data_section::save_list(int &r,FILE *f,const char *cname,const char *termin){
	data_list::iterator i;
	c_data_item *item;
	if (cname)
		fprintf(f,"%*s{%s%s",r,"",cname,termin);
	r++;
	i=data.begin();
//	printf("saving %i items\n",c->data.size());
	while (i!=data.end()){
		item=(*i).second;
//		item->feel();
		if (item->type==1)
			((c_data_section*)item)->save_list(r,f,(*i).first);
		else{
			fprintf(f,"%*s%s=%s%s",r,"",(*i).first,item->str.c_str(),termin);
//			 printf("saving %*s%s=%s\n",r,"",(*i).first,item->str.c_str());
		}
		++i;
	}
	r--;
	if (cname)
		fprintf(f,"%*s}%s",r,"",termin);
}



void c_data_file::setfilename(const char * f){
	filename=f;
}


int c_data_file::read(void){
	if (filename.empty())
		return -2;
//	char buf[df_BUFSIZE];
//	FILE *f=fopen(filename.c_str(),"r");
	c_file_fd f(filename.c_str(),O_RDONLY);
	f.initrbuf();
	data.read_list(&f);
	changed=0;
	f.close();
	return 0;
}
void c_data_file::save(void){
	if (filename.empty())
		return;
//	dobackup(filename.str());
	int r=-1;
	FILE *f=fopen(filename.c_str(),"w");
	if (f){
		//save_list(r,f,&data,NULL);
		data.save_list(r,f,NULL);
		fclose(f);
		changed=0;
	}
}

