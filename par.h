/*
    par.* - parity file handling
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
bool par2file_get_sethash(const string &filename, string &sethash);
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
		int get_initial_pars(c_nntp_files_u &fc, const string &path, const string &temppath) {
			int count=0;
			for (t_server_file_list::const_iterator spi=serverpars.begin(); spi!=serverpars.end(); ++spi){
				fc.addfile(spi->second, path, temppath, false);
				count++;
			}
			if (serverpars.empty() && !serverpxxs.empty()){
				t_server_file_list::iterator smallest = serverpxxs.begin(), cur = smallest;
				for (++cur; cur!=serverpxxs.end(); ++cur)
					if (cur->second->bytes() < smallest->second->bytes())
						smallest = cur;
				fc.addfile(smallest->second, path, temppath, false);
				count++;
				serverpxxs.erase(smallest);
			}
			serverpars.clear();
			return count;
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
		set<string> badbasenames; // set of all basenames for which corrupt/non-par files were found
		

		void addsubjmatch_par1(const string &key, const string &basename){
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
		void addsubjmatch_par2(const string &key, const string &basename){
			if (basenames[key].find(basename)!=basenames[key].end())
				return;//only insert one subject match for each basename
			basenames[key].insert(basename);

			string matchstr;
			if (isalnum(basename[0]))
				matchstr+=regex_match_word_beginning();
			regex_escape_string(basename, matchstr);
			matchstr+="(\\.vol([0-9]+)\\+([0-9]+))?\\.par2";
			matchstr+=regex_match_word_end();
			subjmatches.insert_value(key, new c_regex_r(matchstr.c_str(), REG_EXTENDED|REG_ICASE));
		}
		void addfrompath_par1(const string &path, t_nocase_map *nocase_map=NULL);
		void addfrompath_par2(const string &path, t_nocase_map *nocase_map=NULL);
		void check_badbasenames(void);
		void clear(void){
			basefilenames.clear();
			basenames.clear();
			subjmatches.clear();
			badbasenames.clear();
		}
};


class ParXInfoBase {
	protected:
		t_server_file_list serverpars;
		t_server_file_list serverpxxs;
		typedef map<string, ParSetInfo> t_parset_map;
		set<string> finished_parsets; // which sethashes we have finished (either tested ok, or exhausted all hope)
		int finished_okcount; // how many of the finished_parsets were ok
		t_parset_map parsets;
		LocalParFiles localpars;
		const string path;
		const string temppath;

		ParSetInfo *parset(const string &basename) { 
			return &parsets[basename]; // will insert a new ParSetInfo into parsets if needed.
		}
	
	public:
		ParXInfoBase(const string &p,const string &t):finished_okcount(0),path(p),temppath(t){//well, saving and using only the first temppath encountered isn't exactly perfect, but I doubt many people really download multiple things to the same path but with different temp paths.  And I'm lazy.
		}
		int get_initial_pars(c_nntp_files_u &fc);
		int getNumFinished(void) const { return finished_parsets.size(); }
		int getNumFinishedOk(void) const { return finished_okcount; }
};

class Par1Info : public ParXInfoBase {
	protected:
		int get_pxxs(int num, set<uint32_t> &havevols, const string &key, c_nntp_files_u &fc);
	public:
		Par1Info(const string &p,const string &t):ParXInfoBase(p,t){
			localpars.addfrompath_par1(path);
		}
		bool maybe_add_parfile(const c_nntp_file::ptr &f);
		int maybe_get_pxxs(c_nntp_files_u &fc);
};

class Par2Info : public ParXInfoBase {
	protected:
		int get_recoverypackets(int num, set<uint32_t> &havepackets, const string &key, c_nntp_files_u &fc);
	public:
		Par2Info(const string &p,const string &t):ParXInfoBase(p,t){
			localpars.addfrompath_par2(path);
		}
		bool maybe_add_parfile(const c_nntp_file::ptr &f);
		int maybe_get_pxxs(c_nntp_files_u &fc);
};

class ParInfo {
	protected:
		const string path;
		Par1Info par1info;
		Par2Info par2info;

	public:
		ParInfo(const string &p,const string &t):path(p), par1info(p,t), par2info(p,t) {
		}
		bool maybe_add_parfile(const c_nntp_file::ptr &f) {
			return par1info.maybe_add_parfile(f) || par2info.maybe_add_parfile(f);
		}
		void get_initial_pars(c_nntp_files_u &fc) {
			par1info.get_initial_pars(fc);
			par2info.get_initial_pars(fc);
		}
		void maybe_get_pxxs(c_nntp_files_u &fc) {
			int total_added=0;
			total_added += par1info.maybe_get_pxxs(fc);
			total_added += par2info.maybe_get_pxxs(fc);
			if (!total_added) {
				PMSG("autopar in %s done (%i/%i parsets ok)", path.c_str(),
						par1info.getNumFinishedOk()+par2info.getNumFinishedOk(),
						par1info.getNumFinished()+par2info.getNumFinished());
			}
		}
};

class ParHandler {
	protected:
		typedef auto_map<string, ParInfo> t_parinfo_map;
		t_parinfo_map parinfos;

		t_parinfo_map::mapped_type parinfo(const string &path, const string &temppath);
		t_parinfo_map::mapped_type parinfo(const string &path);
	public:
		bool maybe_add_parfile(const c_nntp_file::ptr &f, const string &path, const string &temppath);
		void get_initial_pars(c_nntp_files_u &fc);
		void maybe_get_pxxs(const string &path, c_nntp_files_u &fc);
};

#endif
