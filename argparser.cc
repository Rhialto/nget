#include "argparser.h"
#include <ctype.h>
#include "log.h"

void parseargs(arglist_t &argl, const char *optarg, bool ignorecomments){
	string curpart;
	const char *cur;
	char quote=0;
	bool esc=0;
	for(cur=optarg;*cur;cur++){
		if (esc){
			switch (*cur){
				case '"':case '\'':
					break;
				default:
					curpart+='\\';//avoid having to double escape stuff other than quotes..
					break;
			}
			esc=0;
			curpart+=*cur;
			continue;
		}
		if (isspace(*cur))
			if(!quote){
				if (!curpart.empty()){
					argl.push_back(curpart);
					curpart="";
				}
				continue;
			}
		if (ignorecomments && !quote && *cur=='#' && curpart.empty()) {
			printf("ignoring: %s\n",cur);
			return;
		}
		switch(*cur){
			case '\\':esc=1;continue;
			case '"':case '\'':
				if (quote==*cur) {quote=0;continue;}
				else if (!quote) {quote=*cur;continue;}
				//else drop through
			default:
				curpart+=*cur;
		}
	}
	if (quote) throw UserExFatal(Ex_INIT,"unterminated quote (%c)", quote);
	if (esc) throw UserExFatal(Ex_INIT,"expression ended with escape");
	if (!curpart.empty())
		argl.push_back(curpart);
}
