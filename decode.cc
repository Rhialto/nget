/*
    decode.* - uu/yenc/etc decoding (wrapper around uulib)
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "decode.h"
#include "strreps.h"
#include "texthandler.h"
#include "path.h"
#include "misc.h"
#include "myregex.h"
#include "status.h"
#include "_sstream.h"

#include <glob.h>

#ifndef PROTOTYPES
#define PROTOTYPES //required for uudeview.h to prototype function args.
#endif
#ifdef HAVE_UUDEVIEW_H
#include <uudeview.h>
#else
#include "uudeview.h"
#endif

int uu_info_callback(void *v,char *i){
	TextHandler *th = static_cast<TextHandler*>(v);
	th->addinfo(i);
	return 0;
};
struct uu_err_status {
	int derr;
	int last_t;
	TextHandler *th;
//	string last_m;
	uu_err_status(void):derr(0),last_t(-1),th(NULL){}
};
void uu_msg_callback(void *v,char *m,int t){
	if (t!=UUMSG_MESSAGE) {//######
		uu_err_status *es = (uu_err_status *)v;
		es->derr++;
		es->last_t = t;
		if (es->th && t!=UUMSG_NOTE) {
			es->th->adddecodeinfo(m);
		}
//		es->last_m = m;
		//++(*(int *)v);
	}
	if (quiet>=2 && (t==UUMSG_MESSAGE || t==UUMSG_NOTE))
		return;
	printf("uu_msg(%i):%s\n",t,m);
};

int uu_busy_callback(void *v,uuprogress *p){
//	if (!quiet) printf("uu_busy(%i):%s %i%%\r",p->action,p->curfile,(100*p->partno-p->percent)/p->numparts);
	if (!quiet) {printf(".");fflush(stdout);}
	return 0;
};
string fname_filter(const char *path, const char *fn){
	const char *fnp=fn;
	int slen=strlen(fnp);
	if (*fnp=='"') {
		fnp++;
		slen--;
	}
	if (fnp[slen-1]=='"')
		slen--;
	if (path)
		return path_join(path,string(fnp,slen));
	else
		return string(fnp,slen);
}
char * uu_fname_filter(void *v,char *fn){
	static string filtered;
	const string *s=(const string *)v;
	filtered = fname_filter(s->c_str(),fn);
	PDEBUG(DEBUG_MED,"uu_fname_filter: filtered %s to %s",fn,filtered.c_str());
	return const_cast<char*>(filtered.c_str()); //uulib isn't const-ified
}

const char *uutypetoa(int uudet) {
	switch (uudet){
		case YENC_ENCODED:return "yEnc";
		case UU_ENCODED:return "UUdata";
		case B64ENCODED:return "Base64";
		case XX_ENCODED:return "XXdata";
		case BH_ENCODED:return "BinHex";
		case PT_ENCODED:return "plaintext";
		case QP_ENCODED:return "qptext";
		default:return "unknown";
	}
}

string make_dupe_name(const string &path, const string &fn, c_nntp_file::ptr f) {
	string s;
	while (1) {
		ostringstream ss;
		ss << fn << '.' << f->badate() << '.' << rand();
		s = ss.str();
		if (!fexists(is_abspath(s.c_str())?s:path_join(path,s)))
			return s;
	}
}

int find_duplicate(const string &nfn, const string &orgfn) {
	string escfn;

	//glob-escape fn
	for (unsigned int i = 0; i < orgfn.length(); i++){
		if (orgfn[i] == '?' || orgfn[i] == '*' || orgfn[i] == '[')
			escfn += '\\' + orgfn[i];
		else
			escfn += orgfn[i];
	}

	escfn += '*';

	//find similar filenames with same prefix
	glob_t g;

	if (0 != glob(escfn.c_str(), 0, NULL, &g)){
		//no expansion or error. Whatever.
		return 0;
	}

	int found = 0;

	//find previously partly decoded files, or other files that happen to have the same name.
	for (unsigned int i = 0; i < g.gl_pathc; i++){
		if (0 == strcmp(nfn.c_str(), g.gl_pathv[i]))
			continue; //it's the new file itself.
		if (filecompare(nfn.c_str(),g.gl_pathv[i])){
			found = 1;
			break;
		}
	}

	globfree(&g);
	return found;
}

int remove_if_duplicate(const string &nfn, const string &orgfn) {
	int found_dup = find_duplicate(nfn,orgfn);
	if (found_dup){
		// if identical to a previously decoded file, delete the one we just downloaded
		unlink(nfn.c_str());
		printf("Duplicate File Removed %s\n", nfn.c_str());
		set_dupe_ok_status();
	}
	return found_dup;
}

void Decoder::addpart(int partno,char *fn) {
	fnbuf.push_back(pair<int,char*>(partno,fn));
}

int Decoder::decode(const nget_options &options, const c_nntp_file_retr::ptr &fr, dupe_file_checker &flist) {
	if (fnbuf.empty()) return 0;

	int optionflags = options.gflags;
	c_nntp_file::ptr f = fr->file;
	uu_err_status uustatus;
	c_regex_nosub uulib_textfn_re("^[0-9][0-9][0-9][0-9]+\\.txt$",REG_EXTENDED);
	int r,un;
	TextHandler texthandler(options.texthandling, options.save_text_for_binaries, options.mboxfname, fr, fnbuf.front().second);
	uustatus.th = &texthandler;
	if ((r=UUInitialize())!=UURET_OK)
		throw ApplicationExFatal(Ex_INIT,"UUInitialize: %s",UUstrerror(r));
	UUSetOption(UUOPT_DUMBNESS,1,NULL); // "smartness" barfs on some subjects
	//UUSetOption(UUOPT_FAST,1,NULL);//we store each message in a seperate file
	//actually, I guess that won't work, since some messages have multiple files in them anyway.
	UUSetOption(UUOPT_OVERWRITE,0,NULL);//no thanks.
	UUSetOption(UUOPT_USETEXT,1,NULL);//######hmmm...
	UUSetOption(UUOPT_DESPERATE,1,NULL);
	UUSetMsgCallback(&uustatus,uu_msg_callback);
	UUSetBusyCallback(NULL,uu_busy_callback,1000);
	UUSetFNameFilter((void*)&fr->path,uu_fname_filter);
	for(t_fnbuf_list::const_iterator fncurb = fnbuf.begin();fncurb!=fnbuf.end();++fncurb){
		UULoadFileWithPartNo((*fncurb).second,NULL,0,(*fncurb).first);
	}
	uulist * uul;
	for (un=0;;un++){
		if ((uul=UUGetFileListItem(un))==NULL)break;
		if (uul->filename==NULL) {
			printf("invalid uulist item, uul->filename==NULL\n");
			uustatus.derr++;
			//continue; // if not using UUOPT_DESPERATE.
			UURenameFile(uul,"noname");
		}
		if (!(uul->state & UUFILE_OK)){
			printf("%s not ok\n",uul->filename);
			texthandler.adddecodeinfo(string(uul->filename) + " not ok");
			uustatus.derr++;
			//continue; // if not using UUOPT_DESPERATE.
		}
		//				printf("\ns:%x d:%x\n",uul->state,uul->uudet);
		r=UUInfoFile(uul,&texthandler,uu_info_callback);
		if ((uul->uudet==PT_ENCODED || uul->uudet==QP_ENCODED) && uul->filename==uulib_textfn_re){
			continue; //ignore, as this should be handled by the UUInfoFile already..
		}
		//check if dest file exists before attempting decode, avoids having to hack around the uu_error that occurs when the destfile exists and overwriting is disabled.
		//also rename the file if it is not ok to avoid the impression that you have a correct file.
		int pre_decode_derr = uustatus.derr;
		string orig_fnp = fname_filter(fr->path.c_str(), uul->filename);
		if ((uul->state & UUFILE_OK) && !fexists(orig_fnp)) {
			r=UUDecodeFile(uul,NULL);
			if ((r!=UURET_OK || uustatus.derr!=pre_decode_derr) && fexists(orig_fnp)) {
				string nfnp;
				nfnp = make_dupe_name(fr->path.c_str(), orig_fnp, f);
				xxrename(orig_fnp.c_str(), nfnp.c_str());
				remove_if_duplicate(nfnp, orig_fnp);
			}
		}
		else {
			//all the following ugliness with fname_filter is due to uulib forgetting that we already filtered the name and giving us the original name instead.
			// Generate a new filename to use
			string nfn(make_dupe_name(fr->path.c_str(), fname_filter(NULL,uul->filename), f));
			UURenameFile(uul,const_cast<char*>(nfn.c_str())); //uulib isn't const-ified
			r=UUDecodeFile(uul,NULL);
			// did it decode something? (could still be incomplete or broken though)
			if (r == UURET_OK){
				string nfnp(path_join(fr->path,nfn));
				int removed_dup = remove_if_duplicate(nfnp, orig_fnp);

				if (fexists(orig_fnp) && !removed_dup){
					set_dupe_warn_status();
				}
			}
		}
		if (r!=UURET_OK){
			uustatus.derr++;
			printf("decode(%s): %s\n",uul->filename,UUstrerror(r));
			texthandler.adddecodeinfo(string("error decoding ")+uutypetoa(uul->uudet)+" "+uul->filename+": " + UUstrerror(r));
			continue;
		}
		else if (pre_decode_derr!=uustatus.derr){
			printf("decode(%s): %i derr(s)\n",uul->filename,uustatus.derr-pre_decode_derr);
			texthandler.adddecodeinfo(string("error decoding ")+uutypetoa(uul->uudet)+" "+uul->filename+": " + tostr(uustatus.derr-pre_decode_derr) + "derr(s)");
			continue;
		}else{
			texthandler.adddecodeinfo(string(uutypetoa(uul->uudet))+" "+uul->filename);
			if (!(optionflags&GETFILES_NODUPEFILECHECK))
				flist.addfile(fr->path, uul->filename); //#### is this the right place? what about dupes saved as different names??
			switch (uul->uudet){
				case YENC_ENCODED:set_yenc_ok_status();break;
				case UU_ENCODED:set_uu_ok_status();break;
				case B64ENCODED:set_base64_ok_status();break;
				case XX_ENCODED:set_xx_ok_status();break;
				case BH_ENCODED:set_binhex_ok_status();break;
				case PT_ENCODED:set_plaintext_ok_status();break;
				case QP_ENCODED:set_qp_ok_status();break;
				default:set_unknown_ok_status();
			}
		}
	}
	UUCleanUp();
	//handle posts that uulib says "no encoded data" for as text. (usually is posts with no body)
	if (uustatus.derr==1 && uustatus.last_t==UUMSG_NOTE && un==0 && f->req<=0 && fnbuf.size()==1) {
		uustatus.derr--; //HACK since this error will also cause a uu_note "No encoded data found", which will incr derr, but we don't want that.
		texthandler.set_save_whole_tempfile(true);//sometimes uulib will get confused and think a text post is an incomplete binary and will say "no encoded data" for it, so tell the texthandler to save the body of the message, if there is one.
		un++;
	}

	if (uustatus.derr==0)
		texthandler.save();

	if (uustatus.derr>0) {
		set_decode_error_status();
		printf(" %i decoding errors occured, keeping temp files.\n",uustatus.derr);
	}
	else if (un==0) {
		uustatus.derr=1;
		set_undecoded_warn_status();
		printf("hm.. nothing decoded.. keeping temp files\n");
	}
	else {
		assert(uustatus.derr==0);
		set_total_ok_status();
		if (optionflags&GETFILES_KEEPTEMP) {
			if (quiet<2)
				printf("decoded ok, keeping temp files.\n");
		}
		else {
			if (quiet<2) 
				printf("decoded ok, deleting temp files.\n");
			delete_tempfiles();
		}
	}

	return uustatus.derr;
}

void Decoder::delete_tempfiles(void) {
	for(t_fnbuf_list::iterator fncurb = fnbuf.begin();fncurb!=fnbuf.end();++fncurb)
		unlink((*fncurb).second);
}

Decoder::~Decoder() {
	for(t_fnbuf_list::iterator fncurb = fnbuf.begin();fncurb!=fnbuf.end();++fncurb)
		free((*fncurb).second);
}
