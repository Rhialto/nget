/*
    par.* - parity file handling
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
#ifndef NGET__PAR_H__
#define NGET__PAR_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <ctype.h>
#include <set>
#include "myregex.h"
#include "auto_map.h"
#include "cache.h"

typedef multimap<string, string> t_nocase_map;
bool parfile_get_sethash(const string &filename, string &sethash);
bool parfile_ok(const string &filename, uint32_t &vol_number);
int parfile_check(const string &filename, const string &path, const t_nocase_map &nocase_map);


typedef multimap<time_t, c_nntp_file::ptr> t_server_file_list;
class ParSetInfo {
	protected:
		t_server_file_list serverpars;
		t_server_file_list serverpxxs;
	public:
		void addserverpar(const c_nntp_file::ptr &f) {
			serverpars.insert(t_server_file_list::value_type(f->badate(), f));
		}
		void addserverpxx(const c_nntp_file::ptr &f) {
			serverpxxs.insert(t_server_file_list::value_type(f->badate(), f));
		}
		void get_initial_pars(const string &path, c_nntp_files_u &fc) {
			for (t_server_file_list::const_iterator spi=serverpars.begin(); spi!=serverpars.end(); ++spi){
				fc.addfile(spi->second, path, path);//#### should honor -P
			}
			if (serverpars.empty() && !serverpxxs.empty()){
				fc.addfile(serverpxxs.begin()->second, path, path);//#### should honor -P
				serverpxxs.erase(serverpxxs.begin());
			}
			serverpars.clear();
		}
		void release_unclaimed_pxxs(vector<c_nntp_file::ptr> &unclaimedfiles){
			for (t_server_file_list::const_iterator spi=serverpxxs.begin(); spi!=serverpxxs.end(); ++spi){
				unclaimedfiles.push_back(spi->second);
			}
			serverpxxs.clear();
		}
};


typedef auto_multimap<string, c_regex_r> t_subjmatches_map;
typedef map<string, set<string> > t_basenames_map;
typedef map<string, vector<string> > t_basefilenames_map;
class LocalParFiles {
	public:
		t_subjmatches_map subjmatches; // maps sethash -> subject matches for known basenames of that set
		t_basenames_map basenames; // maps sethash -> known basenames for that set
		t_basefilenames_map basefilenames; // maps sethash -> all known filenames for that set

		void addsubjmatch(const string &key, const string &basename){
			if (basenames[key].find(basename)!=basenames[key].end())
				return;//only insert one subject match for each basename
			basenames[key].insert(basename);

			string matchstr;
			if (isalnum(basename[0]))
				matchstr+=regex_match_word_beginning();
			regex_escape_string(basename, matchstr);
			matchstr+="\\.p(ar|[0-9]{2})";
			matchstr+=regex_match_word_end();
			subjmatches.insert_value(key, new c_regex_r(matchstr.c_str(), REG_EXTENDED|REG_ICASE));
		}
		void addfrompath(const string &path, t_nocase_map *nocase_map=NULL);
		void clear(void){
			basefilenames.clear();
			basenames.clear();
			subjmatches.clear();
		}
};


class ParInfo {
	protected:
		t_server_file_list serverpars;
		t_server_file_list serverpxxs;
		typedef map<string, ParSetInfo> t_parset_map;
		t_parset_map parsets;
		LocalParFiles localpars;
		const string &path;
	public:
		ParInfo(const string &p):path(p){
			localpars.addfrompath(path);
		}
		ParSetInfo *parset(const string &basename) { 
			return &parsets[basename]; // will insert a new ParSetInfo into parsets if needed.
		}
		bool maybe_add_parfile(const c_nntp_file::ptr &f);
		void get_initial_pars(c_nntp_files_u &fc);
		void get_pxxs(int num, set<uint32_t> &havevols, const string &key, c_nntp_files_u &fc);
		void maybe_get_pxxs(c_nntp_files_u &fc);
};


class ParInfo;
class ParHandler {
	protected:
		typedef auto_map<string, ParInfo> t_parinfo_map;
		t_parinfo_map parinfos;
	public:
		t_parinfo_map::data_type parinfo(const string &path);
		bool maybe_add_parfile(const string &path, const c_nntp_file::ptr &f);
		void get_initial_pars(c_nntp_files_u &fc);
		void maybe_get_pxxs(const string &path, c_nntp_files_u &fc);
};

#endif
