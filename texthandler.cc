/*
    texthandler.* - text post handler
    Copyright (C) 2002  Matthew Mueller <donut@azstarnet.com>

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

#include "texthandler.h"
#include "path.h"
#include "mylockfile.h"
#include "myregex.h"
#include <time.h>

char * make_text_file_name(c_nntp_file_retr::ptr fr) {
	char *nfn;
	//asprintf(&nfn,"%lu.txt",f->banum());//give it a (somewhat) more sensible name
	//give it a (somewhat) more sensible name //TODO: make it better (rand? blah.. message id or something but it might contain bad chars)
	asprintf(&nfn,"%s/%lu.%i.txt",fr->path.c_str(),fr->file->badate(),rand());
	return nfn;
}

void TextHandler::addinfo(const string &str) {
	info.push_back(str);
	infocount++;
}
void TextHandler::adddecodeinfo(const string &str) {
	info.push_back("[" + str + "]\n");
	decodeinfocount++;
}

c_regex_nosub From_re("^>*From ");
void writeline(c_file *f, const char *str, bool escape_From) {
	if (escape_From && str == From_re)
		f->putf(">");
	f->putf("%s", str);
}

void TextHandler::writeinfo(c_file *f, bool escape_From=false) {
	list<string>::const_iterator ii=info.begin();
	bool dupeheaders=true;
	c_file_fd headerf(firsttempfn.c_str(), O_RDONLY);
	headerf.initrbuf();
	while (headerf.bgets()>=0) {
		string hs = string(headerf.rbufp())+"\n";
		if (dupeheaders && hs == *ii) {
			++ii;
		}else
			dupeheaders=false;
//		if (hs.compare(0,13,"Content-Type:")==0 && hs.compare(14,10,"text/plain")!=0) {
		if (hs.substr(0,13)=="Content-Type:" && hs.substr(14,10)!="text/plain") { //gcc 2.95's STL has weird non-standard compare methods, so use substr instead.
			//If the message has a content-type other than text/plain (such as message/partial) then mail readers won't display the body, even though it now IS only text.  So write a new header and rename the original header to "X-NGet-Original-Content-Type"
			writeline(f, "Content-Type: text/plain\n", escape_From);
			writeline(f, ("X-NGet-Original-"+hs).c_str(), escape_From);
		}else
			writeline(f, hs.c_str(), escape_From);

		if (headerf.rbufp()[0]=='\0')
			break;
	}
	for (; ii!=info.end(); ++ii) {
		writeline(f, ii->c_str(), escape_From);
	}
}

TextHandler::TextHandler(t_text_handling texthandlin, bool save_text_for_binarie, const string &mboxnam, c_nntp_file_retr::ptr frp, const char *firsttempf): texthandling(texthandlin), save_text_for_binaries(save_text_for_binarie), mboxname(mboxnam), fr(frp), firsttempfn(firsttempf), infocount(0), decodeinfocount(0) {
}

TextHandler::~TextHandler() {
	if (infocount==0 && decodeinfocount && !save_text_for_binaries)
		return;
	switch (texthandling) {
		case TEXT_FILES: {
			char *textfn = make_text_file_name(fr);
			while (fexists(textfn))  {
				free(textfn);
				textfn = make_text_file_name(fr);
			}
			c_file_fd f(textfn, O_CREAT|O_WRONLY|O_EXCL, PUBMODE);
			writeinfo(&f, false);
			f.close();
			free(textfn);
			break;
		}
		case TEXT_MBOX: { // see http://www.qmail.org/man/man5/mbox.html for mbox format info.
			string mboxpath(fr->path);
			path_append(mboxpath, mboxname);
			c_file_fd f(mboxpath.c_str(), O_CREAT|O_WRONLY|O_APPEND, PUBMODE);
			c_lockfile locker(mboxpath.c_str(), WANT_EX_LOCK);
			time_t curtime = time(NULL);
			f.putf("From nget-"PACKAGE_VERSION" %s", ctime(&curtime)); //ctime has a \n already.
			writeinfo(&f, true);
			f.putf("\n");
			f.close();
			break;
		}
		case TEXT_IGNORE:
			break;
	}
}
