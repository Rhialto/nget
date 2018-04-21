/*
    nget - command line nntp client
    Copyright (C) 1999-2003  Matthew Mueller <donut AT dakotacom.net>

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
#ifndef _NGET_H_
#define _NGET_H_
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>
#include "server.h"
//#include "datfile.h"

void nget_cleanup(void);

//extern c_data_file cfg;
extern string nghome;
extern string ngcachehome;
/*extern c_server_list servers;
extern c_group_info_list groups;
extern c_server_priority_grouping_list priogroups;*/


#define GETFILES_CASESENSITIVE	1
#define GETFILES_GETINCOMPLETE	2
#define GETFILES_NODUPEIDCHECK	4
#define GETFILES_NODUPEFILECHECK	128
#define GETFILES_DUPEFILEMARK	256
#define GETFILES_NOAUTOPAR		4096

#define GETFILES_NODECODE		8
#define GETFILES_KEEPTEMP		16
#define GETFILES_TEMPSHORTNAMES 32
#define GETFILES_NOCONNECT		64
#define GETFILES_TESTMODE		512
#define GETFILES_MARK			1024
#define GETFILES_UNMARK			2048

#define GETFILES_AUTOPAR_DISABLING_FLAGS (GETFILES_NOAUTOPAR|GETFILES_TESTMODE|GETFILES_MARK|GETFILES_UNMARK|GETFILES_NODECODE)

enum t_show_multiserver {
	NO_SHOW_MULTI,
	SHOW_MULTI_LONG,
	SHOW_MULTI_SHORT
};
enum t_cmd_mode {
	RETRIEVE_MODE,
	GROUPLIST_MODE,
	NOCACHE_RETRIEVE_MODE,
	NOCACHE_GROUPLIST_MODE,
};
enum t_text_handling {
	TEXT_FILES,
	TEXT_MBOX,
	TEXT_IGNORE
};
struct nget_options {
	int maxretry,retrydelay;
	ulong linelimit,maxlinelimit;
	int gflags,badskip,qstatus;
	t_show_multiserver test_multi;
	t_show_multiserver retr_show_multi; 
	int makedirs;
	t_cmd_mode cmdmode;
	//c_group_info::ptr group;//,*host;
	vector<c_group_info::ptr> groups;
//	c_data_section *host;
	c_server::ptr host;
	string bindaddr;
//	char *user,*pass;//,*path;
	string path;
	string startpath;
	string temppath;
	vector<string> dupepaths;
	string writelite;
	t_text_handling texthandling;
	bool save_text_for_binaries;
	string mboxfname;
	int fullxover;

	void do_get_path(string &s);
	nget_options(void);
	void get_path(void);
	void get_temppath(void);
	void parse_dupe_flags(const char *opt);
	int set_text_handling(const char *s);
	int set_save_text_for_binaries(const char *s);
	int set_test_multi(const char *s);
	int set_makedirs(const char *s);
};
#endif
