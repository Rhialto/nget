/*
    log.* - debug/error logging and exception defines
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
#ifndef _LOG_H_
#define _LOG_H_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include "strreps.h"
//#include <string.h>
#define PERROR_nnl(a, args...) fprintf(stderr, a, ## args)
#define PMSG_nnl(a, args...) {if (quiet<2) printf(a, ## args);}
#define PDEBUG_nnl(d, a, args...) {if (debug>=d) printf(a, ## args);}
#define PERROR(a, args...) PERROR_nnl(a "\n" , ## args)
#define PMSG(a, args...) PMSG_nnl(a "\n" , ## args)
#define PDEBUG(d, a, args...) PDEBUG_nnl(d, a "\n" , ## args)
extern int debug;
#define DEBUG_MIN 1
#define DEBUG_MED 2
#define DEBUG_ALL 3
extern int quiet;

class baseEx {
	protected:
		string str;
		const char *mfile;
		int mline;
		void set_params(const char *file, int line, const char * s, va_list ap);
	public:
		const char* getExFile(void)const{return mfile;}
		int getExLine(void)const{return mline;}
		const char* getExStr(void)const{return str.c_str();}
		virtual bool isfatal(void)const{return false;}
		virtual const char* getExType(void)const=0;
		virtual ~baseEx() { }
};

#define DEFINE_EX_SUBCLASS(name,base,fatalv) class name : public base {\
	protected:\
		name(void) { } /* to allow subclasses */ \
	public:\
		virtual bool isfatal(void)const{return fatalv;}\
		virtual const char* getExType(void)const{return #name ;}\
		name(const char *file, int line, const char * s, ...) {\
			va_list ap;\
			va_start(ap,s);\
			set_params(file, line, s, ap);\
			va_end(ap);\
			/*PDEBUG(DEBUG_MIN,"%s:%i:Created exception %s with %s(%p)",mfile,mline,getExType(), getExStr(), getExStr());*/\
		}\
};

#define DEFINE_EX_SUBCLASS_SUB(name, sub, fatalv) DEFINE_EX_SUBCLASS(name ## Ex ## sub, name ## Ex, fatalv)

#define DEFINE_EX_CLASSES(name,base) class name ## Ex : public base {};\
DEFINE_EX_SUBCLASS_SUB(name, Fatal, true)\
DEFINE_EX_SUBCLASS_SUB(name, Error, false)


class baseCommEx: public baseEx {};
	
DEFINE_EX_CLASSES(Transport, baseCommEx);
DEFINE_EX_CLASSES(Protocol, baseCommEx);
DEFINE_EX_CLASSES(Path, baseEx);
DEFINE_EX_CLASSES(User, baseEx);
DEFINE_EX_CLASSES(Config, baseEx);
//DEFINE_EX_CLASSES(Application, baseEx);
DEFINE_EX_SUBCLASS(ApplicationExFatal, baseEx, true);
DEFINE_EX_SUBCLASS(CacheEx, baseEx, false);

#define Ex_INIT __FILE__,__LINE__

#define printCaughtEx_nnl(e) PERROR_nnl("%s:%i:caught exception %s:%i:%s: %s",__FILE__,__LINE__,e.getExFile(),e.getExLine(),e.getExType(),e.getExStr())
#define printCaughtEx(e) {printCaughtEx_nnl(e);PERROR_nnl("\n");}


#endif
