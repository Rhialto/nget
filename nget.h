/*
    nget - command line nntp client
    Copyright (C) 1999-2002  Matthew Mueller <donut@azstarnet.com>

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

#include "server.h"
//#include "datfile.h"

void nget_cleanup(void);

//extern c_data_file cfg;
extern string nghome;
extern string ngcachehome;
/*extern c_server_list servers;
extern c_group_info_list groups;
extern c_server_priority_grouping_list priogroups;*/

#define PRIVMODE (S_IRUSR | S_IWUSR)
#define PUBMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
//#define PUBXMODE (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define PUBXMODE (S_IRWXU|S_IRWXG|S_IRWXO)


void set_decode_error_status(int incr=1);
void set_retrieve_error_status(int incr=1);
void set_group_error_status(int incr=1);
void set_path_error_status(int incr=1);
void set_user_error_status(int incr=1);
void set_other_error_status(int incr=1);
void set_fatal_error_status(int incr=1);

void fatal_exit(void);

void set_group_warn_status(int incr=1);
void set_retrieve_warn_status(int incr=1);
void set_undecoded_warn_status(int incr=1);
void set_unequal_line_count_warn_status(int incr=1);
void set_dupe_warn_status(int incr=1);
void set_cache_warn_status(int incr=1);

void set_total_ok_status(int incr=1);
void set_yenc_ok_status(int incr=1);
void set_uu_ok_status(int incr=1);
void set_base64_ok_status(int incr=1);
void set_xx_ok_status(int incr=1);
void set_binhex_ok_status(int incr=1);
void set_plaintext_ok_status(int incr=1);
void set_qp_ok_status(int incr=1);
void set_unknown_ok_status(int incr=1);
void set_group_ok_status(int incr=1);
void set_dupe_ok_status(int incr=1);
void set_skipped_ok_status(int incr=1);

enum t_show_multiserver {
	NO_SHOW_MULTI,
	SHOW_MULTI_LONG,
	SHOW_MULTI_SHORT
};
struct nget_options {
	int maxretry,retrydelay;
	ulong linelimit,maxlinelimit;
	int gflags,badskip,qstatus;
	t_show_multiserver test_multi;
	t_show_multiserver retr_show_multi; 
	char makedirs;
	c_group_info::ptr group;//,*host;
//	c_data_section *host;
	c_server::ptr host;
//	char *user,*pass;//,*path;
	string path;
	string startpath;
	string temppath;
	string writelite;

	void do_get_path(string &s);
	nget_options(void);
	nget_options(nget_options &o);
	void get_path(void);
	void get_temppath(void);
	void parse_dupe_flags(const char *opt);
	int set_test_multi(const char *s);
	int set_makedirs(const char *s);
};
#endif
