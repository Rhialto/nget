/*
    cfgfile.cc - hierarchical config file handling
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
#include "cfgfile.h"
#include "status.h"

bool CfgItem::issok(istringstream &iss) const {
	if (iss.fail() || iss.bad()) {
		PERROR("%s: invalid value", name().c_str());
		set_user_error_status();
		return false;
	}
	if (!iss.eof() && iss.peek()!=EOF) {
		PERROR("%s: trailing junk", name().c_str());
		set_user_error_status();
		return false;
	}
	return true;
}

void CfgSection::load(c_file *f, int &level) {
	char *v,*buf;
	int slen;
	while ((slen=f->bgets())>0){
		buf=f->rbufp();
		if (slen<1)
			continue;
		buf+=strspn(buf," \t");
		if (!*buf)
			continue;
		if (*buf=='}') {
			//			printf("end section\n");
			level--;
			break;
		}
		if (*buf=='{')
		{
			level++;
			CfgSection *s=new CfgSection(name(),buf+1,f,level);
			CfgSection_map::iterator si;
			if ((si=sections.find(s->key))!=sections.end()) {
				delete si->second;
				si->second=s;
				PERROR("%s: duplicate section", s->name().c_str());
				set_user_error_status();
			}
			else
				sections.insert(CfgSection_map::value_type(s->key,s));
			//			printf("new section: %s\n",buf+r+1);
		}
		else if (*buf=='#')
			;//ignore
		else if (buf[0]=='/' && buf[1]=='/')
			;//ignore
		else if((v=strchr(buf,'='))) {
			*v=0;
			v++;
			CfgItem *i=new CfgItem(name(),buf,v);
			CfgItem_map::iterator ii;
			if ((ii=items.find(i->key))!=items.end()) {
				delete ii->second;
				ii->second=i;
				PERROR("%s: duplicate item", i->name().c_str());
				set_user_error_status();
			}
			else
				items.insert(CfgItem_map::value_type(i->key,i));
			//				printf("new item: %s=%s\n",n,v);
		}
		else {
			PERROR("%s: invalid line '%s'",f->name(),buf);
			set_user_error_status();
		}
	}
}
CfgSection::CfgSection(const string &filename) : CfgBase(filename,"") {
	c_file_fd f(filename.c_str(),O_RDONLY);
	f.initrbuf();
	int level=0;
	load(&f, level);
	if (level!=0) {
		PERROR("%s: unbalanced {}'s: level=%+i", filename.c_str(), level);
		set_user_error_status();
	}
	f.close();
	used=true;
}
CfgSection::~CfgSection() {
	for (CfgItem_map::iterator i = items.begin(); i != items.end(); ++i)
		delete (*i).second;
	for (CfgSection_map::iterator s = sections.begin(); s != sections.end(); ++s)
		delete (*s).second;
}

int CfgSection::check_unused(void) const {
	int count=0;
	for (CfgItem_map::const_iterator i = items.begin(); i != items.end(); ++i)
		if (!i->second->isused()) {
			count++;
			PERROR("%s: unused item (possible typo)", i->second->name().c_str());
			set_user_error_status();
		}
	for (CfgSection_map::const_iterator s = sections.begin(); s != sections.end(); ++s){
		if (s->second->isused()) {
			count+=s->second->check_unused();
		}
		else {
			count++;
			PERROR("%s: unused section (possible typo)", s->second->name().c_str());
			set_user_error_status();
		}
	}
	return count;
}

const CfgItem *CfgSection::getitem(const char *name) const {
	used=true;
	CfgItem_map::const_iterator i=items.find(name);
	return (i!=items.end()) ? (*i).second : NULL;
}
const CfgSection *CfgSection::getsection(const char *name) const {
	used=true;
	CfgSection_map::const_iterator i=sections.find(name);
	return (i!=sections.end()) ? (*i).second : NULL;
}

