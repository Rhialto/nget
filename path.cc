#include "path.h"

#ifdef WIN32
#include <ctype.h>
bool is_abspath(const char *p) {
	if (is_pathsep(p[0]))
		return true;
	if (p[0]!=0 && p[1]!=0 && p[2]!=0 && isalpha(p[0]) && p[1]==':' && is_pathsep(p[2]))
		return true;
	return false;
}
#endif

string& path_append(string &a, string b) {
	char c=a[a.size()-1];
	if (!is_pathsep(c))
		a.push_back('/');// since '/' is a valid path sep on windows too, we don't need to specialize this.
	return a.append(b);
}

string path_join(string a, string b) {
	return path_append(a, b);
}
string path_join(string a, string b, string c) {
	return path_append(path_append(a,b), c);
}


