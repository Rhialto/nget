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
#include "par.h"
#include <map>
#include <set>
#include <dirent.h>
#include <sys/stat.h>
#include "path.h"
#include "log.h"
#include "md5.h"

static void add_to_nocase_map(t_nocase_map *nocase_map, const char *key, const char *name){
	string lowername;
	for (const char *cp=key; *cp; ++cp)
		lowername.push_back(tolower(*cp));
	nocase_map->insert(t_nocase_map::value_type(lowername, name));
}
void LocalParFiles::addfrompath(const string &path, t_nocase_map *nocase_map){
	c_regex_r parfile_re((string("^(.+)\\.p(ar|[0-9]{2})(\\.[0-9]+\\.[0-9]+)?$")).c_str(), REG_EXTENDED|REG_ICASE);
	c_regex_r dupefile_re((string("^(.+)\\.[0-9]+\\.[0-9]+$")).c_str());
	c_regex_subs rsubs;
	DIR *dir=opendir(path.c_str());
	struct dirent *de;
	if (!dir)
		throw PathExFatal(Ex_INIT,"opendir: %s(%i)",strerror(errno),errno);
	while ((de=readdir(dir))) {
		if (!parfile_re.match(de->d_name, &rsubs))
			addfile(rsubs.sub(1), de->d_name);
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


bool ParInfo::maybe_add_parfile(const c_nntp_file::ptr &f) {
	if (f->maybe_a_textpost())
		return false;//try to avoid mistaking text posts that have .pxx stuff in the title for binary par posts.

	c_regex_subs rsubs;
	for (t_subjmatches_map::iterator smi=localpars.subjmatches.begin(); smi!=localpars.subjmatches.end(); ++smi) {
		if (!smi->second->match(f->subject.c_str(), &rsubs)) {
			PDEBUG(DEBUG_MIN, "file %s matches localpars (%s)",f->subject.c_str(),smi->first.c_str());
			if (isalpha(rsubs.subs(1)[0]))
				parset(smi->first)->addserverpar(f);
			else
				serverpxxs.insert(t_server_file_list::value_type(f->badate(), f));
			return true;
		}
	}

	static c_regex_r parsubj_re((string("([^ ]*)\\.p(ar|[0-9]{2})")+regex_match_word_end()).c_str(), REG_EXTENDED|REG_ICASE);
	if (!parsubj_re.match(f->subject.c_str(), &rsubs)){
		string basename(rsubs.sub(1));
		if (!basename.empty() && basename[0]=='"')
			basename.erase(basename.begin());
		PDEBUG(DEBUG_MIN, "file %s seems like a par (%s)",f->subject.c_str(),basename.c_str());
		if (isalpha(rsubs.subs(2)[0]))
			parset(basename)->addserverpar(f);
		else
			parset(basename)->addserverpxx(f);
		return true;
	}

	return false;
}

void ParInfo::get_initial_pars(c_nntp_files_u &fc) {
	for (t_parset_map::iterator psi=parsets.begin(); psi!=parsets.end(); ++psi){
		psi->second.get_initial_pars(path, fc);
	}
}

void ParInfo::get_pxxs(int num, const string &basename, c_nntp_files_u &fc) {
	assert(localpars.subjmatches.find(basename)!=localpars.subjmatches.end());
	t_subjmatches_map::data_type subjmatch = localpars.subjmatches.find(basename)->second;
	t_server_file_list::iterator sfi=serverpxxs.begin();
	t_server_file_list::iterator last_sfi=serverpxxs.end();
	c_regex_subs rsubs;
	while (sfi!=serverpxxs.end() && num>0){
		last_sfi=sfi;
		++sfi;
		if (!subjmatch->match(last_sfi->second->subject.c_str(), &rsubs)) {
			--num;
			PDEBUG(DEBUG_MIN, "get_pxxs: %i, %s, adding %s", num, basename.c_str(), last_sfi->second->subject.c_str());
			fc.addfile(last_sfi->second, path, path);//#### should honor -P
			serverpxxs.erase(last_sfi);
		}
	}
}

void ParInfo::maybe_get_pxxs(c_nntp_files_u &fc) {
	t_nocase_map nocase_map;
	localpars.clear();
	localpars.addfrompath(path, &nocase_map);

	vector<c_nntp_file::ptr> unclaimedfiles;
	for (t_parset_map::iterator psi=parsets.begin(); psi!=parsets.end(); ++psi){
		psi->second.release_unclaimed_pxxs(unclaimedfiles);
	}
	for (vector<c_nntp_file::ptr>::iterator ufi=unclaimedfiles.begin(); ufi!=unclaimedfiles.end(); ++ufi) {
		bool r=maybe_add_parfile(*ufi);
		assert(r);
	}

	get_initial_pars(fc);

	for (t_basefilenames_map::const_iterator bfni=localpars.basefilenames.begin(); bfni!=localpars.basefilenames.end(); ++bfni) {
		string goodpar;
		uint32_t volnumber;
		set<uint32_t> goodvols;
		int badcount=0;
		for (vector<string>::const_iterator fni=bfni->second.begin(); fni!=bfni->second.end(); ++fni) {
			string fn = path_join(path, *fni);
			if (parfile_ok(fn, volnumber)) {
				PDEBUG(DEBUG_MIN, "parfile %s: good (vol %i)",fn.c_str(), volnumber);
				goodpar=fn;
				if (volnumber>0)
					goodvols.insert(volnumber);
			}else {
				PDEBUG(DEBUG_MIN, "parfile %s: bad",fn.c_str());
				badcount++;
			}

		}
		int needed=0;
		if (goodpar.empty()) {
			needed=1;
			PDEBUG(DEBUG_MIN, "in %s/%s.p?? no goodpar found, trying to get one", path.c_str(), bfni->first.c_str());
		} else {
			int bad = parfile_check(goodpar, path, nocase_map);
			needed = max(0, bad - (signed)goodvols.size());
			PDEBUG(DEBUG_MIN, "in %s/%s.p??, %i goodpxxs, %i badp??s found, %i bad/missing files, trying to get %i more", path.c_str(), bfni->first.c_str(), goodvols.size(), badcount, bad, needed);
		}
		get_pxxs(needed, bfni->first, fc);
	}
}

		
ParHandler::t_parinfo_map::data_type ParHandler::parinfo(const string &path) {
	t_parinfo_map::iterator i = parinfos.find(path);
	if (i != parinfos.end())
		return (*i).second;
	return (*parinfos.insert_value(path, new ParInfo(path)).first).second;
}

bool ParHandler::maybe_add_parfile(const string &path, const c_nntp_file::ptr &f) {
	return parinfo(path)->maybe_add_parfile(f);
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
#define BLOCKSIZE 8192
	struct md5_ctx ctx;
	char buffer[BLOCKSIZE + 72];
	size_t sum;

	/* Initialize the computation context.  */
	md5_init_ctx (&ctx);

	/* Iterate over full file contents.  */
	while (1) {

		size_t n;
		sum = 0;

		/* Read block.  Take care for partial reads.  */
		do
		{
			n = f->read(buffer + sum, BLOCKSIZE - sum);
			sum += n;
		}
		while (sum < BLOCKSIZE && n != 0);
		//if (n == 0 && ferror (stream))
		//	return -1;

		/* If end of file is reached, end the loop.  */
		if (n == 0)
			break;

		/* Process buffer with BLOCKSIZE bytes.  Note that
		   BLOCKSIZE % 64 == 0
		   */
		md5_process_block (buffer, BLOCKSIZE, &ctx);
	}

	/* Add the last bytes if necessary.  */
	if (sum > 0)
		md5_process_bytes (buffer, sum, &ctx);

	/* Construct result in desired memory.  */
	md5_finish_ctx (&ctx, result);
}

void md5_file(const char *filename, uchar *result) {
	c_file_fd f(filename, "rb");
	md5_file(&f, result);
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
		return 1;
	}
	return 0;
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
	uchar buf[2048];
	f.readfull(buf, 16 + 16 + 8);//ignore control hash, set hash, volume number
	uint32_t numfiles;
	uint32_t filelistsize;
	f.read_le_u32(&numfiles);
	f.readfull(buf, 4 + 8); //ignore high dword of numfiles, start offset of the file list
	f.read_le_u32(&filelistsize);
	f.readfull(buf, 4 + 8 + 8); //ignore high dword of filelistsize, start offset of the data, data size

	for (unsigned int i=0;i<numfiles;++i) {
		uint32_t entrysize;
		uchar status;
		uint64_t filesize;
		uchar file_hash[16];

		f.read_le_u32(&entrysize);
		f.readfull(buf, 4); //ignore high dword of entrysize
		f.readfull(&status, 1);
		f.readfull(buf, 7); //rest of status field is unused
		f.read_le_u64(&filesize);
		f.readfull(file_hash, 16);
		f.readfull(buf, 16); //ignore 16k hash
		f.readfull(buf, entrysize - 0x38); ///#####don't overflow buf
		
		if (!(status & 1)) //handle status bit0 = 0 - file is not saved in the parity volume set
			continue; //if the file isn't in the parity set, then retrieving more pxxs won't help if it is bad, so don't bother checking it.
		
		string filename;
		for (unsigned int n=0; n<(entrysize-0x38)/2; ++n) {
			if (buf[n*2]>127 || buf[n*2+1]) {
				PERROR("in %s: can't handle non-ascii filename for file %u", parfilename.c_str(), i);
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

