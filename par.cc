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
#include "par.h"
#include <map>
#include <set>
#include <dirent.h>
#include <sys/stat.h>
#include "path.h"
#include "log.h"
#include "misc.h"
#include "nget.h"
#include "par2/par2cmdline.h"
#include "knapsack.h"


static void add_to_nocase_map(t_nocase_map *nocase_map, const char *key, const char *name){
	string lowername;
	for (const char *cp=key; *cp; ++cp)
		lowername.push_back(tolower(*cp));
	nocase_map->insert(t_nocase_map::value_type(lowername, name));
}
void LocalParFiles::addfrompath_par1(const string &path, t_nocase_map *nocase_map){
	c_regex_r parfile_re("^(.+)\\.p(ar|[0-9]{2})(\\.[0-9]+\\.[0-9]+)?$", REG_EXTENDED|REG_ICASE);
	c_regex_r dupefile_re("^(.+)\\.[0-9]+\\.[0-9]+$");
	c_regex_subs rsubs;
	DIR *dir=opendir(path.c_str());
	struct dirent *de;
	if (!dir)
		throw PathExFatal(Ex_INIT,"opendir: %s(%i)",strerror(errno),errno);
	while ((de=readdir(dir))) {
		if (!parfile_re.match(de->d_name, &rsubs)) {
			string sethash;
			string fullname = path_join(path, de->d_name);
			string basename = rsubs.sub(1);
			for (string::iterator i=basename.begin(); i!=basename.end(); ++i)
				*i=tolower(*i);
			if (parfile_get_sethash(fullname, sethash)) {
				basefilenames[sethash].push_back(de->d_name);
				addsubjmatch_par1(sethash, basename);
			}
			else
				badbasenames.insert(basename);
		}
		if (nocase_map) {
			if (strcmp(de->d_name,"..")!=0 && strcmp(de->d_name,".")!=0){
				if (!dupefile_re.match(de->d_name, &rsubs)) //check for downloaded dupe files, and add them under their original name.
					add_to_nocase_map(nocase_map, rsubs.subs(1), de->d_name);
				add_to_nocase_map(nocase_map, de->d_name, de->d_name);
			}
		}
	}
	closedir(dir);
}

void LocalParFiles::addfrompath_par2(const string &path, t_nocase_map *nocase_map){
	c_regex_r parfile_re("^(.+)\\.par2(\\.[0-9]+\\.[0-9]+)?$", REG_EXTENDED|REG_ICASE);
	c_regex_r dupefile_re("^(.+)\\.[0-9]+\\.[0-9]+$");
	static c_regex_r par2pxxre("^(.*).vol[0-9]+\\+[0-9]+$", REG_EXTENDED|REG_ICASE);
	c_regex_subs rsubs;
	DIR *dir=opendir(path.c_str());
	struct dirent *de;
	if (!dir)
		throw PathExFatal(Ex_INIT,"opendir: %s(%i)",strerror(errno),errno);
	while ((de=readdir(dir))) {
		if (!parfile_re.match(de->d_name, &rsubs)) {
			string sethash;
			string fullname = path_join(path, de->d_name);
			string basename = rsubs.sub(1);
			if (!par2pxxre.match(basename.c_str(), &rsubs))
				basename = rsubs.sub(1);
			for (string::iterator i=basename.begin(); i!=basename.end(); ++i)
				*i=tolower(*i);
			if (par2file_get_sethash(fullname, sethash)) {
				basefilenames[sethash].push_back(de->d_name);
				addsubjmatch_par2(sethash, basename);
			}
			else
				badbasenames.insert(basename);
		}
		if (nocase_map) {
			if (strcmp(de->d_name,"..")!=0 && strcmp(de->d_name,".")!=0){
				if (!dupefile_re.match(de->d_name, &rsubs)) //check for downloaded dupe files, and add them under their original name.
					add_to_nocase_map(nocase_map, rsubs.subs(1), de->d_name);
				add_to_nocase_map(nocase_map, de->d_name, de->d_name);
			}
		}
	}
	closedir(dir);
}

void LocalParFiles::check_badbasenames(void) {
	for (t_basenames_map::const_iterator bni=basenames.begin(); bni!=basenames.end(); ++bni) {
		for (set<string>::const_iterator sbni=bni->second.begin(); sbni!=bni->second.end(); ++sbni) {
			badbasenames.erase(*sbni);
		}
	}
	for (set<string>::const_iterator i=badbasenames.begin(); i!=badbasenames.end(); ++i){
		PDEBUG(DEBUG_MIN, "no parsable par files with basename %s", (*i).c_str());
		set_autopar_error_status();
	}
	
}


bool Par1Info::maybe_add_parfile(const c_nntp_file::ptr &f, bool want_incomplete) {
	if (f->maybe_a_textpost())
		return false;//try to avoid mistaking text posts that have .pxx stuff in the title for binary par posts.

	if (!want_incomplete && !f->iscomplete())
		return false;

	c_regex_subs rsubs;
	for (t_subjmatches_map::iterator smi=localpars.subjmatches.begin(); smi!=localpars.subjmatches.end(); ++smi) {
		if (!smi->second->match(f->subject.c_str(), &rsubs)) {
			PMSG("autopar: %s matches local parset %s",f->subject.c_str(),hexstr(smi->first).c_str());
			if (isalpha(rsubs.subs(1)[0]))
				serverpars.insert(server_file_list_value(f));
			else
				serverpxxs.insert(server_file_list_value(f));
			return true;
		}
	}

	static c_regex_r parsubj_re((string("([^ ]*)\\.p(ar|[0-9]{2})")+regex_match_word_end()).c_str(), REG_EXTENDED|REG_ICASE);
	if (!parsubj_re.match(f->subject.c_str(), &rsubs)){
		string basename(rsubs.sub(1));
		if (!basename.empty() && basename[0]=='"')
			basename.erase(basename.begin());
		PMSG("autopar: %s seems like a par (%s)",f->subject.c_str(),basename.c_str());
		if (isalpha(rsubs.subs(2)[0]))
			parset(basename)->addserverpar(f);
		else
			parset(basename)->addserverpxx(f);
		return true;
	}

	return false;
}

int ParXInfoBase::get_initial_pars(c_nntp_files_u &fc) {
	int count=0;
	for (t_server_file_list::const_iterator spi=serverpars.begin(); spi!=serverpars.end(); ++spi){
		fc.addfile(spi->second, path, temppath, false);
		count++;
	}
	serverpars.clear();
	for (t_parset_map::iterator psi=parsets.begin(); psi!=parsets.end(); ++psi){
		count+=psi->second.get_initial_pars(fc, path, temppath);
	}
	return count;
}

int Par1Info::get_pxxs(int num, set<uint32_t> &havevols, const string &key, c_nntp_files_u &fc) {
	assert(localpars.subjmatches.find(key)!=localpars.subjmatches.end());
	pair<t_subjmatches_map::iterator,t_subjmatches_map::iterator> matches=localpars.subjmatches.equal_range(key);
	c_regex_subs rsubs;
	if (nconfig.autopar_optimistic) {
		set<uint32_t> availvols;
		for (t_server_file_list::iterator sfi=serverpxxs.begin(); sfi!=serverpxxs.end() && (signed)availvols.size()<num; ++sfi){
			for (t_subjmatches_map::iterator smi=matches.first; smi!=matches.second; ++smi) {
				if (!smi->second->match(sfi->second->subject.c_str(), &rsubs)) {
					int vol = atoi(rsubs.subs(1));
					if (havevols.find(vol)==havevols.end())
						availvols.insert(vol);
				}
			}
		}
		if ((signed)availvols.size()<num){
			PERROR("parset %s: Only %i/%i needed volumes available, giving up", hexstr(key).c_str(), (int)availvols.size(), num);
			return 0;
		}
	}
	int cur=0;
	t_server_file_list::iterator sfi=serverpxxs.begin();
	t_server_file_list::iterator last_sfi=serverpxxs.end();
	while (sfi!=serverpxxs.end() && cur<num){
		last_sfi=sfi;
		++sfi;
		for (t_subjmatches_map::iterator smi=matches.first; smi!=matches.second; ++smi) {
			if (!smi->second->match(last_sfi->second->subject.c_str(), &rsubs)) {
				int vol = atoi(rsubs.subs(1));
				if (havevols.find(vol)!=havevols.end()){
					PDEBUG(DEBUG_MIN, "get_pxxs: %i, %s, already have or getting vol %i, not adding %s", cur, hexstr(key).c_str(), vol, last_sfi->second->subject.c_str());
					continue;
				}
				havevols.insert(vol);//don't try to retrieve multiple of the same volume in one run
				PDEBUG(DEBUG_MIN, "get_pxxs: %i, %s, adding %s", cur, hexstr(key).c_str(), last_sfi->second->subject.c_str());
				fc.addfile(last_sfi->second, path, temppath, false);
				serverpxxs.erase(last_sfi);
				++cur;
				break;
			}
		}
	}
	return cur;
}

int Par1Info::maybe_get_pxxs(c_nntp_files_u &fc) {
	t_nocase_map nocase_map;
	localpars.clear();
	localpars.addfrompath_par1(path, &nocase_map);

	vector<c_nntp_file::ptr> unclaimedfiles;
	for (t_parset_map::iterator psi=parsets.begin(); psi!=parsets.end(); ++psi){
		psi->second.release_unclaimed_pxxs(unclaimedfiles);
	}
	for (vector<c_nntp_file::ptr>::iterator ufi=unclaimedfiles.begin(); ufi!=unclaimedfiles.end(); ++ufi) {
#ifndef NDEBUG
		bool r=
#endif
			maybe_add_parfile(*ufi, true);
		assert(r);
	}

	int total_added = get_initial_pars(fc);

	for (t_basefilenames_map::const_iterator bfni=localpars.basefilenames.begin(); bfni!=localpars.basefilenames.end(); ++bfni) {
		if (finished_parsets.find(bfni->first)!=finished_parsets.end())
			continue;//we are already done with this parset, don't waste time retesting stuff.

		string goodpar;
		uint32_t volnumber;
		set<uint32_t> goodvols;
		int badcount=0;
		for (vector<string>::const_iterator fni=bfni->second.begin(); fni!=bfni->second.end(); ++fni) {
			string fn = path_join(path, *fni);
			if (parfile_ok(fn, volnumber)) {
				PMSG("parfile %s: good (vol %i parset %s)",fn.c_str(), volnumber, hexstr(bfni->first).c_str());
				goodpar=fn;
				if (volnumber>0)
					goodvols.insert(volnumber);
			}else {
				PMSG("parfile %s: bad",fn.c_str());
				badcount++;
			}

		}
		int needed=0;
		if (goodpar.empty()) {
			needed=1;
			PMSG("parset %s in %s: no goodpar found, trying to get one", hexstr(bfni->first).c_str(), path.c_str());
		} else {
			int bad = parfile_check(goodpar, path, nocase_map);
			needed = max(0, bad - (signed)goodvols.size());
			PMSG("parset %s in %s: %i goodpxxs, %i badp??s, %i bad/missing files, trying to get %i more", hexstr(bfni->first).c_str(), path.c_str(), (int)goodvols.size(), badcount, bad, needed);
		}
		if (needed) {
			int parset_added = get_pxxs(needed, goodvols, bfni->first, fc);//modifies goodvols, but we don't care.
			if (!parset_added) {
				set_autopar_error_status();
				finished_parsets.insert(bfni->first);
			}
			total_added += parset_added;
		}else{
			set_autopar_ok_status();
			finished_parsets.insert(bfni->first);
			finished_okcount++;
		}
	}

	if (total_added==0)
		localpars.check_badbasenames();

	return total_added;
	
}


bool Par2Info::maybe_add_parfile(const c_nntp_file::ptr &f, bool want_incomplete) {
	if (f->maybe_a_textpost())
		return false;//try to avoid mistaking text posts that have .pxx stuff in the title for binary par posts.

	c_regex_subs rsubs;
	for (t_subjmatches_map::iterator smi=localpars.subjmatches.begin(); smi!=localpars.subjmatches.end(); ++smi) {
		if (!smi->second->match(f->subject.c_str(), &rsubs)) {
			PMSG("autopar: %s matches local par2set (%s)",f->subject.c_str(),hexstr(smi->first).c_str());
			if (rsubs.sublen(1) == 0)
				serverpars.insert(server_file_list_value(f));
			else
				serverpxxs.insert(server_file_list_value(f));
			return true;
		}
	}

	static c_regex_r par2subj_re((string("([^ ]*)\\.par2")+regex_match_word_end()).c_str(), REG_EXTENDED|REG_ICASE);
	static c_regex_r par2pxxre("^(.*).vol[0-9]+\\+[0-9]+$", REG_EXTENDED|REG_ICASE);
	if (!par2subj_re.match(f->subject.c_str(), &rsubs)){
		string basename(rsubs.sub(1));
		bool ispxx = !par2pxxre.match(basename.c_str(), &rsubs);
		if (ispxx)
			basename = rsubs.sub(1);
		if (!basename.empty() && basename[0]=='"')
			basename.erase(basename.begin());
		PMSG("autopar: %s seems like a par2 (%s)",f->subject.c_str(),basename.c_str());
		if (!ispxx)
			parset(basename)->addserverpar(f);
		else
			parset(basename)->addserverpxx(f);
		return true;
	}

	if (!want_incomplete && !f->iscomplete()) {
		serverextradata.insert(server_file_list_value(f));
		return true;
	}

	return false;
}

int Par2Info::get_extradata(c_nntp_files_u &fc) {
	if (!serverextradata.empty()) {
		PMSG("autopar: not enough par2 recovery packets, trying to get some incomplete data");
		//####TODO: should be smart and get only posts that pertain to files that are missing or damaged.
		fc.addfile(serverextradata.begin()->second, path, temppath, false);
		return 1;
	}
	return 0;
}
		
int Par2Info::get_recoverypackets(int num, set<uint32_t> &havepackets, const string &key, c_nntp_files_u &fc) {
	assert(localpars.subjmatches.find(key)!=localpars.subjmatches.end());
	pair<t_subjmatches_map::iterator,t_subjmatches_map::iterator> matches=localpars.subjmatches.equal_range(key);
	c_regex_subs rsubs;
	
	vector<int> sizes, values;
	vector<t_server_file_list::iterator> items;
	set<int> results;
	set<uint32_t> availpackets;
	for (t_server_file_list::iterator sfi=serverpxxs.begin(); sfi!=serverpxxs.end(); ++sfi){
		for (t_subjmatches_map::iterator smi=matches.first; smi!=matches.second; ++smi) {
			if (!smi->second->match(sfi->second->subject.c_str(), &rsubs)) {
				uint32_t beginpacket = atoi(rsubs.subs(2)), endpacket = atoi(rsubs.subs(3)) + beginpacket;
				int value = 0;
				for (uint32_t packet = beginpacket; packet < endpacket; ++packet)
					if (havepackets.find(packet)==havepackets.end() && //don't try to retrive packets we already have
							availpackets.find(packet)==availpackets.end()) //don't try to retrieve multiple of the same packet in one run
					{
						availpackets.insert(packet);
						value++;
					}
				if (value>0) {
					if (sfi->second->req >= 1)
						value = max(1, value*sfi->second->have/sfi->second->req); // give incomplete files less weight (but no lower than 1, since they would never be gotten then)
					sizes.push_back(endpacket-beginpacket);
					values.push_back(value);
					items.push_back(sfi);
				}
				else PDEBUG(DEBUG_MIN, "get_recoverypackets: %s, already have or getting packets %u+%u, skipping %s", hexstr(key).c_str(), beginpacket, endpacket - beginpacket, sfi->second->subject.c_str());
			}
		}
	}
	if ((signed)availpackets.size()<num){
		int r = get_extradata(fc);
		if (r) return r;
	}
	if (nconfig.autopar_optimistic && (signed)availpackets.size()<num){
		PERROR("par2set %s: Only %i/%i needed packets available, giving up", hexstr(key).c_str(), (int)availpackets.size(), num);
		return 0;
	}
#ifndef NDEBUG
	int result_size = 
#endif
		knapsack_minsize(values, sizes, num, results);
	
	int cur=0;
	set<int>::iterator ri=results.begin(), re=results.end();
	while (ri!=re){
		assert(cur<num);

		cur += values[*ri];
		fc.addfile(items[*ri]->second, path, temppath, false);
		PDEBUG(DEBUG_MIN, "get_recoverypackets: %i, %s, adding %s", cur, hexstr(key).c_str(), items[*ri]->second->subject.c_str());
		serverpxxs.erase(items[*ri]);
#ifndef NDEBUG
		result_size -= sizes[*ri];
#endif
		++ri;
	}
	assert(result_size==0);
	return cur;
}

int Par2Info::maybe_get_pxxs(c_nntp_files_u &fc) {
	t_nocase_map nocase_map;
	localpars.clear();
	localpars.addfrompath_par2(path, &nocase_map);

	vector<c_nntp_file::ptr> unclaimedfiles;
	for (t_parset_map::iterator psi=parsets.begin(); psi!=parsets.end(); ++psi){
		psi->second.release_unclaimed_pxxs(unclaimedfiles);
	}
	for (vector<c_nntp_file::ptr>::iterator ufi=unclaimedfiles.begin(); ufi!=unclaimedfiles.end(); ++ufi) {
#ifndef NDEBUG
		bool r=
#endif
			maybe_add_parfile(*ufi, true);
		assert(r);
	}

	int total_added = get_initial_pars(fc);

	for (t_basefilenames_map::const_iterator bfni=localpars.basefilenames.begin(); bfni!=localpars.basefilenames.end(); ++bfni) {
		if (finished_parsets.find(bfni->first)!=finished_parsets.end())
			continue;//we are already done with this parset, don't waste time retesting stuff.
		
		set<uint32_t> goodpackets;
		bool goodparfound = false;
		int needed_packets = 0;

		vector<string> fullpaths;
		for (vector<string>::const_iterator fni=bfni->second.begin(); fni!=bfni->second.end(); ++fni)
			fullpaths.push_back(path_join(path, *fni));
	
		for (vector<string>::const_iterator fni=fullpaths.begin(); fni!=fullpaths.end(); ++fni) {
			const string &fn = *fni;
			
			CommandLine par2cmd(fn, fullpaths, nocase_map);
			Par2Repairer par2;
			Result r = par2.Process(par2cmd, false);

			if (r == eSuccess || r == eRepairPossible || r == eRepairNotPossible) {
				goodparfound = true;

				for (map<u32, RecoveryPacket*>::const_iterator rpmi = par2.recoverypacketmap.begin(); rpmi != par2.recoverypacketmap.end(); ++rpmi)
					goodpackets.insert(rpmi->first);
				
				if (r == eRepairNotPossible)
					needed_packets = par2.missingblockcount - par2.recoverypacketmap.size();
				PMSG("par2set %s in %s: %i recovery packets, %i bad/missing source blocks, trying to get %i more", hexstr(bfni->first).c_str(), path.c_str(), (int)goodpackets.size(), par2.missingblockcount, needed_packets);
				break;
			}
		}

		if (!goodparfound) {
			needed_packets = 1;
			PMSG("par2set %s in %s: no goodpar found, trying to get one", hexstr(bfni->first).c_str(), path.c_str());
		}

		if (needed_packets) {
			int parset_added = get_recoverypackets(needed_packets, goodpackets, bfni->first, fc);
			if (!parset_added) {
				set_autopar_error_status();
				finished_parsets.insert(bfni->first);
			}
			total_added += parset_added;
		}else{
			set_autopar_ok_status();
			finished_parsets.insert(bfni->first);
			finished_okcount++;
		}
	}

	if (total_added==0)
		localpars.check_badbasenames();

	return total_added;
}

		
ParHandler::t_parinfo_map::mapped_type ParHandler::parinfo(const string &path, const string &temppath) {
	t_parinfo_map::iterator i = parinfos.find(path);
	if (i != parinfos.end())
		return (*i).second;
	return (*parinfos.insert_value(path, new ParInfo(path, temppath)).first).second;
}
ParHandler::t_parinfo_map::mapped_type ParHandler::parinfo(const string &path) {
	t_parinfo_map::iterator i = parinfos.find(path);
	assert(i != parinfos.end());
	return (*i).second;
}

bool ParHandler::maybe_add_parfile(const c_nntp_file::ptr &f, const string &path, const string &temppath, bool want_incomplete) {
	return parinfo(path,temppath)->maybe_add_parfile(f, want_incomplete);
}

void ParHandler::get_initial_pars(c_nntp_files_u &fc) {
	set<string> paths_in_fc;
	for (t_nntp_files_u::iterator dfi = fc.files.begin(); dfi!=fc.files.end(); ++dfi)
		paths_in_fc.insert(dfi->second->path);
	for (t_parinfo_map::iterator pi=parinfos.begin(); pi!=parinfos.end(); ++pi) {
		if (paths_in_fc.find(pi->first) != paths_in_fc.end())
			pi->second->get_initial_pars(fc);
		else
			pi->second->maybe_get_pxxs(fc);//if the path isn't in the set of files we are retrieving, then try to get any pxxs now, since we won't be called for that path ever otherwise.
	}
}

void ParHandler::maybe_get_pxxs(const string &path, c_nntp_files_u &fc) {
	parinfo(path)->maybe_get_pxxs(fc);
}


void md5_file(c_file *f, uchar *result) {
	const int BLOCKSIZE = 8192;
	char buffer[BLOCKSIZE];
	MD5Context context;
	size_t n;
	while ((n = f->read(buffer, BLOCKSIZE)))
		context.Update(buffer, n);
	MD5Hash hash;
	context.Final(hash);
	memcpy(result, hash.hash, 16);
}

void md5_file(const char *filename, uchar *result) {
	c_file_fd f(filename, "rb");
	md5_file(&f, result);
}


static int par2file_check_packet(c_file_fd &f, int pos) {
	uint64_t pktlen;
	char pkthash[16];
	f.seek(pos+8, SEEK_SET);
	f.read_le_u64(&pktlen);
	f.readfull(pkthash, 16);

	const int BLOCKSIZE = 8192;
	char buffer[BLOCKSIZE];
	MD5Context context;
	size_t n;
	pktlen -= 8+8+16;
	while (pktlen && (n = f.read(buffer, min((uint64_t)BLOCKSIZE,pktlen)))) {
		context.Update(buffer, n);
		pktlen -= n;
	}
	if (pktlen) return 1;
	MD5Hash hash;
	context.Final(hash);
	return memcmp(pkthash, hash.hash, 16);
}

static int par2file_find_packet(c_file_fd &f) {
	const uchar parheader[] = "PAR2\0PKT";
	const int BLOCKSIZE = 8192;
	uchar buf[BLOCKSIZE];
	int pos = 0, bufc = 0;
	while (1) {
		while (bufc<8) {
			int c = f.read(buf+bufc, BLOCKSIZE-bufc);
			if (c <= 0) return -1;
			bufc += c;
		}
		for (int i=0; i<bufc-7; ++i)
			if (memcmp(parheader, buf+i, 8)==0) {
				if (!par2file_check_packet(f, pos+i))
					return pos+i;
				else
					f.seek(pos+bufc, SEEK_SET);
				
			}
		memmove(buf, buf+bufc-7, 7);
		pos += bufc - 7;
		bufc = 7;
	}
}

bool par2file_get_sethash(const string &filename, string &sethash) {
	try{
		c_file_fd f(filename.c_str(), "rb");
		int pos = par2file_find_packet(f);
		if (pos < 0) {
			PERROR("error reading %s: no valid par2 packets", f.name());
			return false;
		}
		f.seek(pos+8+8+16, SEEK_SET);
		char set_hash[16];
		f.readfull(set_hash, 16);
		sethash.assign(set_hash, 16);
		return true;
	}catch(FileEx &e){
		printCaughtEx(e);
		return false;
	}
}

static int parfile_check_header(c_file &f) {
	const uchar parheader[] = "PAR\0\0\0\0\0";
	uchar buf[8];
	f.readfull(buf, 8);
	if (memcmp(buf, parheader, 8)){
		PERROR("error reading %s: invalid parfile header", f.name());
		return 1;
	}
	f.readfull(buf, 8);//we ignore par generator ver (last 4 bytes)
	if (!(buf[0]==0 && buf[1]==0 && buf[2]==1 && buf[3]==0)) {
		PERROR("error reading %s: unhandled par version %i.%i.%i.%i", f.name(), buf[0], buf[1], buf[2], buf[3]);
		set_autopar_warn_status();
		return 1;
	}
	return 0;
}

bool parfile_get_sethash(const string &filename, string &sethash) {
	try{
		c_file_fd f(filename.c_str(), "rb");
		if (parfile_check_header(f))
			return false;
		f.seek(0x0020, SEEK_SET);
		char set_hash[16];
		f.readfull(set_hash, 16);
		sethash.assign(set_hash, 16);
		return true;
	}catch(FileEx &e){
		printCaughtEx(e);
		return false;
	}
}

bool parfile_ok(const string &filename, uint32_t &vol_number) {
	try{
		c_file_fd f(filename.c_str(), "rb");
		if (parfile_check_header(f))
			return false;
		uchar control_hash[16];
		uchar actual_hash[16];
		f.readfull(control_hash, 16);
		md5_file(&f, actual_hash);
		if (memcmp(control_hash, actual_hash, 16))
			return false;

		f.seek(0x0030, SEEK_SET);
		f.read_le_u32(&vol_number);

		return true;
	}catch(FileEx &e){
		printCaughtEx(e);
		return false;
	}
}

int parfile_check(const string &parfilename, const string &path, const t_nocase_map &nocase_map) {
	int needed=0;
	c_file_fd f(parfilename.c_str(), "rb");
	if (parfile_check_header(f))
		return 1;
	CharBuffer buf(64);
	f.readfull(buf.data(), 16 + 16 + 8);//ignore control hash, set hash, volume number
	uint32_t numfiles;
	uint32_t filelistsize;
	f.read_le_u32(&numfiles);
	f.readfull(buf.data(), 4 + 8); //ignore high dword of numfiles, start offset of the file list
	f.read_le_u32(&filelistsize);
	f.readfull(buf.data(), 4 + 8 + 8); //ignore high dword of filelistsize, start offset of the data, data size

	for (unsigned int i=0;i<numfiles;++i) {
		uint32_t entrysize;
		uchar status;
		uint64_t filesize;
		uchar file_hash[16];

		f.read_le_u32(&entrysize);
		f.readfull(buf.data(), 4); //ignore high dword of entrysize
		f.readfull(&status, 1);
		f.readfull(buf.data(), 7); //rest of status field is unused
		f.read_le_u64(&filesize);
		f.readfull(file_hash, 16);
		f.readfull(buf.data(), 16); //ignore 16k hash
		buf.reserve(entrysize - 0x38);
		f.readfull(buf.data(), entrysize - 0x38);
		
		if (!(status & 1)) //handle status bit0 = 0 - file is not saved in the parity volume set
			continue; //if the file isn't in the parity set, then retrieving more pxxs won't help if it is bad, so don't bother checking it.
		
		string filename;
		for (unsigned int n=0; n<(entrysize-0x38)/2; ++n) {
			if ((uchar)buf[n*2]>127 || buf[n*2+1]) {
				PERROR("in %s: can't handle non-ascii filename for file %u", parfilename.c_str(), i);
				set_autopar_warn_status();
				++needed;
				filename="";
			}else
				filename += tolower(buf[n*2]);
		}
		if (!filename.empty()) {
			pair<t_nocase_map::const_iterator,t_nocase_map::const_iterator> matchingfiles(nocase_map.equal_range(filename));
			for (; matchingfiles.first!=matchingfiles.second; ++matchingfiles.first){
				off_t actual_fsize;
				string matchedfilename = path_join(path, matchingfiles.first->second);
				if (fsize(matchedfilename.c_str(), &actual_fsize) || (uint64_t)actual_fsize!=filesize)
					continue;//doesn't match, try another
				else {
					uchar actual_hash[16];
					md5_file(matchedfilename.c_str(), actual_hash);
					if (memcmp(file_hash, actual_hash, 16)==0)
						break;//found a good match
				}
			}
			if (matchingfiles.first==matchingfiles.second)
				++needed;//no good matches found for this filename
		}
	}

	return needed;
}

