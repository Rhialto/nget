#include "cfgfile.h"
#include "nget.h"

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

void CfgSection::load(c_file *f) {
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
			break;
		}
		if (*buf=='{')
		{
			CfgSection *s=new CfgSection(name(),buf+1,f);
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
			items.insert(CfgItem_map::value_type(i->key,i));
			//				printf("new item: %s=%s\n",n,v);
		}
		else {
			PERROR("%s: invalid line '%s'",f->name(),buf);
			set_user_error_status();
		}
	}
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

