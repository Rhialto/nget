AC_DEFUN(SOCK_CHECK_TYPE,
[AC_REQUIRE([AC_HEADER_STDC])dnl
AC_MSG_CHECKING(for $1)
AC_CACHE_VAL(ac_cv_type_$1,
[AC_EGREP_CPP(dnl
changequote(<<,>>)dnl
<<$1[^a-zA-Z_0-9]>>dnl
changequote([,]), [#include <sys/types.h>
#include <sys/socket.h>
#if STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif], ac_cv_type_$1=yes, ac_cv_type_$1=no)])dnl
AC_MSG_RESULT($ac_cv_type_$1)
if test $ac_cv_type_$1 = no; then
  AC_DEFINE($1, $2, [Define to `$2' if <sys/types.h> doesn't define])
fi
])

AC_DEFUN(MY_CHECK_POPT_CONST,
[AC_CACHE_CHECK([if popt wants const argvs],
  ac_cv_popt_const_argv,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <popt.h>]], [[const char ** targv=NULL;poptContext c=poptGetContext(NULL,1,targv,NULL,0);]])],[ac_cv_popt_const_argv=yes],[ac_cv_popt_const_argv=no])])
if test $ac_cv_popt_const_argv = yes; then
  AC_DEFINE(POPT_CONST_ARGV,1,[Does popt want const argvs?])
fi
])


dnl AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[INCLUDES]], [[FUNCTION-BODY]])],[dnl             ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]],[])
AC_DEFUN(MY_CHECK_EXCEPTIONS,
 [
  AC_MSG_CHECKING(for exception handling support)
  AC_CACHE_VAL(my_cv_exceptions,[
   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>]], [[try { throw 1; } catch (int i) {int z=i;}]])],[my_cv_exceptions=yes],[])
   if test -z $my_cv_exceptions; then
     CXXFLAGS="$CXXFLAGS -fhandle-exceptions"
     AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>]], [[try { throw 1; } catch (int i) {int z=i;}]])],[my_cv_exceptions=yes],[my_cv_exceptions=no])
   fi
  ])
  AC_MSG_RESULT($my_cv_exceptions)
  if test "x$my_cv_exceptions" = xno; then
   AC_MSG_ERROR([exceptions not supported by your compiler,
if you need a special flag try CXXFLAGS="-flag-to-enable-exceptions" ./configure])
  fi
 ]
)

AC_DEFUN(MY_CHECK_FOMIT_FRAME_POINTER,[
 AC_MSG_CHECKING([for improper use of -fomit-frame-pointer])
 result=no
 if echo "$CXX $CXXFLAGS" | grep fomit-frame-pointer > /dev/null ; then
  if test -n "$host_alias" ; then
   my_host="$host_alias"
  else
   my_host=`uname -m`
  fi
  if echo "$my_host" | grep 86 > /dev/null ; then
   if test "$GXX" = "yes"; then
    case `$CXX -dumpversion` in
     [
     [012].* | 3.[012]* | 3.3 | 3.3.0* )
     ]dnl quote the tests with []'s to prevent m4 from eating the []'s in the tests.
       AC_MSG_WARN([cannot build with -fomit-frame-pointer on x86.
Old versions of gcc do not handle exceptions properly in code compiled with
-fomit-frame-pointer on x86 platforms.  See:
http://gcc.gnu.org/cgi-bin/gnatsweb.pl?cmd=view&pr=2447&database=gcc

Removing -fomit-frame-pointer from the compiler flags.
])
       CXX=`echo $CXX | sed "s/-fomit-frame-pointer//"`
       CXXFLAGS=`echo $CXXFLAGS | sed "s/-fomit-frame-pointer//"`
       result=removed
       ;;
    esac
   fi
  fi
 fi
 AC_MSG_RESULT($result)
])

AC_DEFUN(MY_DISABLE_OPT,[
 changequote(, )dnl
 CXXFLAGS=`echo $CXXFLAGS | sed "s/-O[1-9] *//"`
 changequote([, ])dnl
])

AC_DEFUN([MY_TRY_COMPILE_HASH_MAP],[
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#ifdef HAVE_HASH_MAP
#include <hash_map>
#elif HAVE_EXT_HASH_MAP
#include <ext/hash_map>
#elif HAVE_HASH_MAP_H
#include <hash_map.h>
#endif
using namespace std;
$1
	]], [[
	hash_map<int, long> h;
	hash_multimap<int, long> h2;
	]])],[$2],[$3
	])
])

AC_DEFUN([MY_CHECK_HASH_MAP],[
	AC_CHECK_HEADERS(hash_map ext/hash_map hash_map.h)
	AC_CACHE_CHECK([working hash_map], ac_cv_working_hash_map, [
		MY_TRY_COMPILE_HASH_MAP([],
		[ac_cv_working_hash_map=yes],[
			MY_TRY_COMPILE_HASH_MAP([using namespace __gnu_cxx;],
			[ac_cv_working_hash_map=yes-in_gnu_cxx_namespace],
			[ac_cv_working_hash_map=no]
			)
		])
	])

if test "$ac_cv_working_hash_map" = "yes"; then 
	AC_DEFINE(HAVE_WORKING_HASH_MAP,1,[define if system has a usable hash_map])
elif test "$ac_cv_working_hash_map" = "yes-in_gnu_cxx_namespace"; then
	AC_DEFINE(HAVE_WORKING_HASH_MAP,1,[define if system has a usable hash_map])
	AC_DEFINE(HASH_MAP_NEED_GNU_CXX_NAMESPACE,1,[define if hash_map classes are in __gnu_cxx namespace])
fi
])

AC_DEFUN([MY_SEARCH_LIBS],[
AC_CACHE_CHECK([for $5], [my_cv_$1],
[ac_func_search_save_LIBS=$LIBS
my_cv_$1=no
AC_LINK_IFELSE([AC_LANG_PROGRAM([[$2]], [[$3]])],[my_cv_$1="none required"],[])
if test "$my_cv_$1" = no; then
  for ac_lib in $4; do
    LIBS="-l$ac_lib $ac_func_search_save_LIBS"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[$2]], [[$3]])],[my_cv_$1="-l$ac_lib"
break],[])
  done
fi
LIBS=$ac_func_search_save_LIBS])
AS_IF([test "$my_cv_$1" != no],
  [test "$my_cv_$1" = "none required" || LIBS="$my_cv_$1 $LIBS"
  AC_DEFINE(HAVE_[]translit([$1], [a-z], [A-Z]),1,[Do we have $5?])])dnl
])

AC_DEFUN([MY_CHECK_TERMSTUFF],[
 MY_SEARCH_LIBS(netbsd_curses,
  [#include <termcap.h>],
  [struct tinfo *info;
   char *term="foo";
   int res = t_getent(&info, term);],
  [termcap],
  [netbsd-style termcap])
 if test "$my_cv_netbsd_curses" = no ; then
  MY_SEARCH_LIBS(new_curses,
   [#include <term.h>
    #include <stdio.h>],
   [int res = tputs(clr_bol, 1, putchar);],
   [termcap curses ncurses],
   [new curses terminfo])
 fi
 if test "$my_cv_netbsd_curses" = no -a "$my_cv_new_curses" = no ; then
  MY_SEARCH_LIBS(old_curses,
   [#include <curses.h>
    #include <term.h>],
   [setupterm((char *)0, 1, (int *)0);
    int res = putp(clr_bol);],
   [curses],
   [sysv terminfo])
 fi
])

AC_DEFUN([MY_CHECK_SOCKET],[MY_SEARCH_LIBS(socket,
[#include <sys/types.h>
#ifdef HAVE_WINSOCK_H
#include <winsock.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif],
[socket(AF_INET, SOCK_STREAM, 0);],
[socket ws2_32 wsock32],
[library containing socket])
])

# MY_CHECK_FUNC(FUNCTION, [ARGS], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND], [HEADERS])
# -----------------------------------------------------------------
AC_DEFUN([MY_CHECK_FUNC],
[AS_VAR_PUSHDEF([ac_var], [ac_cv_func_$1])dnl
AC_CACHE_CHECK([for $1], ac_var,
[AC_LINK_IFELSE([AC_LANG_PROGRAM([$5],[$1($2)])],
		[AS_VAR_SET(ac_var, yes)],
		[AS_VAR_SET(ac_var, no)])])
AS_IF([test AS_VAR_GET(ac_var) = yes], [$3], [$4])dnl
AS_VAR_POPDEF([ac_var])dnl
])# MY_CHECK_FUNC

# MY_CHECK_FUNCS(FUNCTION..., [ARGS], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND], [HEADERS])
# ---------------------------------------------------------------------
AC_DEFUN([MY_CHECK_FUNCS],
[m4_foreach_w([AC_Func],[$1],[AH_TEMPLATE(AS_TR_CPP(HAVE_[]AC_Func),
	       [Define to 1 if you have the `]AC_Func[' function.])])dnl
for ac_func in $1
do
MY_CHECK_FUNC($ac_func,[$2],
	      [AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_$ac_func])) $3],
	      [$4],[$5])
done
])



dnl @synopsis AC_PROTOTYPE(function, includes, code, TAG1, values1 [, TAG2, values2 [...]])
dnl
dnl Try all the combinations of <TAG1>, <TAG2>... to successfully compile <code>.
dnl <TAG1>, <TAG2>, ... are substituted in <code> and <include> with values found in
dnl <values1>, <values2>, ... respectively. <values1>, <values2>, ... contain a list of
dnl possible values for each corresponding tag and all combinations are tested.
dnl When AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[include]], [[code]])],[],[]) is successfull for a given substitution, the macro
dnl stops and defines the following macros: FUNCTION_TAG1, FUNCTION_TAG2, ... using AC_DEFINE()
dnl with values set to the current values of <TAG1>, <TAG2>, ...
dnl If no combination is successfull the configure script is aborted with a message.
dnl
dnl Intended purpose is to find which combination of argument types is acceptable for a
dnl given function <function>. It is recommended to list the most specific types first.
dnl For instance ARG1, [size_t, int] instead of ARG1, [int, size_t].
dnl
dnl Generic usage pattern:
dnl
dnl 1) add a call in configure.in
dnl
dnl  AC_PROTOTYPE(...)
dnl
dnl 2) call autoheader to see which symbols are not covered
dnl
dnl 3) add the lines in acconfig.h
dnl
dnl  /* Type of Nth argument of function */
dnl  #undef FUNCTION_ARGN
dnl
dnl 4) Within the code use FUNCTION_ARGN instead of an hardwired type
dnl
dnl Complete example:
dnl
dnl 1) configure.in
dnl
dnl  AC_PROTOTYPE(getpeername,
dnl  [
dnl   #include <sys/types.h>
dnl   #include <sys/socket.h>
dnl  ],
dnl  [
dnl   int a = 0;
dnl   ARG2 * b = 0;
dnl   ARG3 * c = 0;
dnl   getpeername(a, b, c);
dnl  ],
dnl  ARG2, [struct sockaddr, void],
dnl  ARG3, [socklen_t, size_t, int, unsigned int, long unsigned int])
dnl
dnl 2) call autoheader
dnl
dnl  autoheader: Symbol `GETPEERNAME_ARG2' is not covered by ./acconfig.h
dnl  autoheader: Symbol `GETPEERNAME_ARG3' is not covered by ./acconfig.h
dnl
dnl 3) acconfig.h
dnl
dnl  /* Type of second argument of getpeername */
dnl  #undef GETPEERNAME_ARG2
dnl
dnl  /* Type of third argument of getpeername */
dnl  #undef GETPEERNAME_ARG3
dnl
dnl 4) in the code
dnl      ...
dnl      GETPEERNAME_ARG2 name;
dnl      GETPEERNAME_ARG3 namelen;
dnl      ...
dnl      ret = getpeername(socket, &name, &namelen);
dnl      ...
dnl
dnl Implementation notes: generating all possible permutations of
dnl the arguments is not easily done with the usual mixture of shell and m4,
dnl that is why this macro is almost 100% m4 code. It generates long but simple
dnl to read code.
dnl
dnl @version $Id: ac_prototype.m4,v 1.2 2000/08/11 06:28:24 simons Exp $
dnl @author Loic Dachary <loic@senga.org>
dnl

AC_DEFUN([AC_PROTOTYPE],[
dnl
dnl Upper case function name
dnl
 pushdef([function],translit([$1], [a-z], [A-Z]))
dnl
dnl Collect tags that will be substituted
dnl
 pushdef([tags],[AC_PROTOTYPE_TAGS(builtin([shift],builtin([shift],builtin([shift],$@))))])
dnl
dnl Wrap in a 1 time loop, when a combination is found break to stop the combinatory exploration
dnl
 for i in 1
 do
   AC_PROTOTYPE_LOOP(AC_PROTOTYPE_REVERSE($1, AC_PROTOTYPE_SUBST($2,tags),AC_PROTOTYPE_SUBST($3,tags),builtin([shift],builtin([shift],builtin([shift],$@)))))
   AC_MSG_ERROR($1 unable to find a working combination)
 done
 popdef([tags])
 popdef([function])
])

dnl
dnl AC_PROTOTYPE_REVERSE(list)
dnl
dnl Reverse the order of the <list>
dnl
AC_DEFUN([AC_PROTOTYPE_REVERSE],[ifelse($#,0,,$#,1,[[$1]],[AC_PROTOTYPE_REVERSE(builtin([shift],$@)),[$1]])])

dnl
dnl AC_PROTOTYPE_SUBST(string, tag)
dnl
dnl Substitute all occurence of <tag> in <string> with <tag>_VAL.
dnl Assumes that tag_VAL is a macro containing the value associated to tag.
dnl
AC_DEFUN([AC_PROTOTYPE_SUBST],[ifelse($2,,[$1],[AC_PROTOTYPE_SUBST(patsubst([$1],[$2],[$2[]_VAL]),builtin([shift],builtin([shift],$@)))])])

dnl
dnl AC_PROTOTYPE_TAGS([tag, values, [tag, values ...]])
dnl
dnl Generate a list of <tag> by skipping <values>.
dnl
AC_DEFUN([AC_PROTOTYPE_TAGS],[ifelse($1,,[],[$1, AC_PROTOTYPE_TAGS(builtin([shift],builtin([shift],$@)))])])

dnl
dnl AC_PROTOTYPE_DEFINES(tags)
dnl
dnl Generate a AC_DEFINE(function_tag, tag_VAL) for each tag in <tags> list
dnl Assumes that function is a macro containing the name of the function in upper case
dnl and that tag_VAL is a macro containing the value associated to tag.
dnl
AC_DEFUN([AC_PROTOTYPE_DEFINES],[ifelse($1,,[],[AC_DEFINE(function[]_$1, $1_VAL, Type of function $1) AC_PROTOTYPE_DEFINES(builtin([shift],$@))])])

dnl
dnl AC_PROTOTYPE_STATUS(tags)
dnl
dnl Generates a message suitable for argument to AC_MSG_* macros. For each tag
dnl in the <tags> list the message tag => tag_VAL is generated.
dnl Assumes that tag_VAL is a macro containing the value associated to tag.
dnl
AC_DEFUN([AC_PROTOTYPE_STATUS],[ifelse($1,,[],[$1 => $1_VAL AC_PROTOTYPE_STATUS(builtin([shift],$@))])])

dnl
dnl AC_PROTOTYPE_EACH(tag, values)
dnl
dnl Call AC_PROTOTYPE_LOOP for each values and define the macro tag_VAL to
dnl the current value.
dnl
AC_DEFUN([AC_PROTOTYPE_EACH],[
  ifelse($2,, [
  ], [
    pushdef([$1_VAL], $2)
    AC_PROTOTYPE_LOOP(rest)
    popdef([$1_VAL])
    AC_PROTOTYPE_EACH($1, builtin([shift], builtin([shift], $@)))
  ])
])

dnl
dnl AC_PROTOTYPE_LOOP([tag, values, [tag, values ...]], code, include, function)
dnl
dnl If there is a tag/values pair, call AC_PROTOTYPE_EACH with it.
dnl If there is no tag/values pair left, tries to compile the code and include
dnl using AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[],[]). If it compiles, AC_DEFINE all the tags to their
dnl current value and exit with success.
dnl
AC_DEFUN([AC_PROTOTYPE_LOOP],[
 ifelse(builtin([eval], $# > 3), 1,
   [
     pushdef([rest],[builtin([shift],builtin([shift],$@))])
     AC_PROTOTYPE_EACH($2,$1)
     popdef([rest])
   ], [
     AC_MSG_CHECKING($3 AC_PROTOTYPE_STATUS(tags))
dnl
dnl Activate fatal warnings if possible, gives better guess
dnl
     ac_save_CPPFLAGS="$CPPFLAGS"
	 dnl nget is c++, so we can hardcode this since autoconf barfs on the tests
     if test "$GXX" = "yes" ; then CPPFLAGS="$CPPFLAGS -Werror" ; fi
dnl     ifelse(AC_LANG,CPLUSPLUS,if test "$GXX" = "yes" ; then CPPFLAGS="$CPPFLAGS -Werror" ; fi)
dnl     ifelse(AC_LANG,C,if test "$GCC" = "yes" ; then CPPFLAGS="$CPPFLAGS -Werror" ; fi)
     AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[$2]], [$1])],[
      CPPFLAGS="$ac_save_CPPFLAGS"
      AC_MSG_RESULT(ok)
      AC_PROTOTYPE_DEFINES(tags)
      break;
     ],[
      CPPFLAGS="$ac_save_CPPFLAGS"
      AC_MSG_RESULT(not ok)
     ])
   ]
 )
])

AC_DEFUN([AC_PROTOTYPE_RECV],[
AC_PROTOTYPE(recv,
 [
  #include <sys/types.h>
#ifdef WIN32
  #include <winsock.h>
#else
  #include <sys/socket.h>
#endif
 ],
 [
  ARG2 b = 0;
  recv(0, b, 0, 0);
 ],
 ARG2, [const void*, const char*, void*, char*])
])


dnl @synopsis AC_FUNC_MKDIR
dnl
dnl Check whether mkdir() is mkdir or _mkdir, and whether it takes one or two
dnl arguments.
dnl
dnl This macro can define HAVE_MKDIR, HAVE__MKDIR, and MKDIR_TAKES_ONE_ARG,
dnl which are expected to be used as follow:
dnl
dnl   #if HAVE_MKDIR
dnl   # if MKDIR_TAKES_ONE_ARG
dnl      /* Mingw32 */
dnl   #  define mkdir(a,b) mkdir(a)
dnl   # endif
dnl   #else
dnl   # if HAVE__MKDIR
dnl      /* plain Win32 */
dnl   #  define mkdir(a,b) _mkdir(a)
dnl   # else
dnl   #  error "Don't know how to create a directory on this system."
dnl   # endif
dnl   #endif
dnl
dnl @version $Id: ac_func_mkdir.m4,v 1.1 2001/03/02 11:39:22 simons Exp $
dnl @author Alexandre Duret-Lutz <duret_g@epita.fr>
dnl
AC_DEFUN([AC_FUNC_MKDIR],
[AC_CHECK_FUNCS([mkdir _mkdir])
AC_CACHE_CHECK([whether mkdir takes one argument],
                [ac_cv_mkdir_takes_one_arg],
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/stat.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
]], [[mkdir (".");]])],[ac_cv_mkdir_takes_one_arg=yes],[ac_cv_mkdir_takes_one_arg=no])])
if test x"$ac_cv_mkdir_takes_one_arg" = xyes; then
  AC_DEFINE([MKDIR_TAKES_ONE_ARG],1,
            [Define if mkdir takes only one argument.])
fi
])

dnl Note:
dnl =====
dnl I have not implemented the following suggestion because I don't have
dnl access to such a broken environment to test the macro.  So I'm just
dnl appending the comments here in case you have, and want to fix
dnl AC_FUNC_MKDIR that way.
dnl
dnl |Thomas E. Dickey (dickey@herndon4.his.com) said:
dnl |  it doesn't cover the problem areas (compilers that mistreat mkdir
dnl |  may prototype it in dir.h and dirent.h, for instance).
dnl |
dnl |Alexandre:
dnl |  Would it be sufficient to check for these headers and #include
dnl |  them in the AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[],[]) block?  (and is AC_HEADER_DIRENT
dnl |  suitable for this?)
dnl |
dnl |Thomas:
dnl |  I think that might be a good starting point (with the set of recommended
dnl |  ifdef's and includes for AC_HEADER_DIRENT, of course).


dnl @synopsis AC_donut_CHECK_PACKAGE(PACKAGE, FUNCTION, LIBRARY, HEADERFILE [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Provides --with-PACKAGE, --with-PACKAGE-include and --with-PACKAGE-lib
dnl options to configure. Supports --with-PACKAGE=DIR approach which looks
dnl first in DIR and then in DIR/{include,lib} but also allows the include
dnl and lib directories to be specified seperately.
dnl
dnl adds the extra -Ipath to CFLAGS if needed
dnl adds extra -Lpath to LD_FLAGS if needed
dnl searches for the FUNCTION in the LIBRARY with
dnl AC_CHECK_LIBRARY and thus adds the lib to LIBS
dnl
dnl defines HAVE_PKG_PACKAGE if it is found, (where PACKAGE in the
dnl HAVE_PKG_PACKAGE is replaced with the actual first parameter passed)
dnl
dnl Based on:
dnl @version $Id: ac_caolan_check_package.m4,v 1.5 2000/08/30 08:50:25 simons Exp $
dnl @author Caolan McNamara <caolan@skynet.ie> with fixes from Alexandre Duret-Lutz <duret_g@lrde.epita.fr>.
dnl
AC_DEFUN([AC_donut_CHECK_PACKAGE_pre],
[
AC_ARG_WITH($1-prefix,
[AS_HELP_STRING([--with-$1-prefix=DIR],[use $1 and look in DIR/{include,lib}/])],
if test "$withval" != no; then
	with_$1=yes
	if test "$withval" != yes; then
		$1_include="${withval}/include"
		$1_libdir="${withval}/lib"
	fi
fi
)

AC_ARG_WITH($1-include,
[AS_HELP_STRING([--with-$1-include=DIR],[specify exact include dir for $1 headers])],
$1_include="$withval")

AC_ARG_WITH($1-lib,
[AS_HELP_STRING([--with-$1-lib=DIR],[specify exact library dir for $1 library])],
$1_libdir="$withval")

if test "${with_$1}" != no ; then
        OLD_LIBS=$LIBS
        OLD_LDFLAGS=$LDFLAGS
        OLD_CFLAGS=$CFLAGS
        OLD_CPPFLAGS=$CPPFLAGS

        if test "${$1_libdir}" ; then
                LDFLAGS="$LDFLAGS -L${$1_libdir}"
	elif test "${with_$1}" != yes ; then
                LDFLAGS="$LDFLAGS -L${with_$1}"
        fi
        if test "${$1_include}" ; then
                CPPFLAGS="$CPPFLAGS -I${$1_include}"
                CFLAGS="$CFLAGS -I${$1_include}"
	elif test "${with_$1}" != yes ; then
                CPPFLAGS="$CPPFLAGS -I${with_$1}"
                CFLAGS="$CFLAGS -I${with_$1}"
        fi

	no_good=no
                
	ifelse([$7], , , [$7])

        if test "$no_good" = yes; then
dnl     broken
                LIBS=$OLD_LIBS
                LDFLAGS=$OLD_LDFLAGS
                CPPFLAGS=$OLD_CPPFLAGS
                CFLAGS=$OLD_CFLAGS
		
                ifelse([$6], , , [$6])
        else
dnl     fixed
                ifelse([$5], , , [$5])

                AC_DEFINE(HAVE_PKG_$1,1,[Define if you have $3 and $4])
        fi

else
        dnl pointless, but sh barfs if the else..fi is empty (when arg 6 is not used)
        no_good=yes 

        ifelse([$6], , , [$6])

fi

])

AC_DEFUN([AC_donut_CHECK_PACKAGE_sub],
[
	AC_donut_CHECK_PACKAGE_pre([$1], [$2], [$3], [$4], [$5], [$6], [
        AC_CHECK_LIB($3,$2,,no_good=yes)
        AC_CHECK_HEADERS($4,,no_good=yes)
	])
])

dnl package that defaults to enabled
AC_DEFUN([AC_donut_CHECK_PACKAGE_DEF],
[
AC_ARG_WITH($1,
AS_HELP_STRING([--without-$1],[disables $1 usage completely])
AS_HELP_STRING([--with-$1=DIR],[look in DIR for $1]),
with_$1=$withval
,
with_$1=yes
)
AC_donut_CHECK_PACKAGE_sub([$1], [$2], [$3], [$4], [$5], [$6])
]
)

dnl package that defaults to disabled
AC_DEFUN([AC_donut_CHECK_PACKAGE],
[
AC_ARG_WITH($1,
[AS_HELP_STRING([--with-$1(=DIR)],[use $1, optionally looking in DIR])],
with_$1=$withval
,
with_$1=no
)
AC_donut_CHECK_PACKAGE_sub([$1], [$2], [$3], [$4], [$5], [$6])
]
)



dnl fooo 
AC_DEFUN([AC_donut_SEARCH_PACKAGE_sub],
[
	AC_donut_CHECK_PACKAGE_pre([$1], [$2], [$3], [$4], [$5], [$6], [
        AC_SEARCH_LIBS($2,$3,,no_good=yes)
        AC_CHECK_HEADERS($4,,no_good=yes)
	])
])

dnl package that defaults to enabled
AC_DEFUN([AC_donut_SEARCH_PACKAGE_DEF],
[
AC_ARG_WITH($1,
AS_HELP_STRING([--without-$1],[disables $1 usage completely])
AS_HELP_STRING([--with-$1=DIR],[look in DIR for $1]),
with_$1=$withval
,
with_$1=yes
)
AC_donut_SEARCH_PACKAGE_sub([$1], [$2], [$3], [$4], [$5], [$6])
]
)

dnl package that defaults to disabled
AC_DEFUN([AC_donut_SEARCH_PACKAGE],
[
AC_ARG_WITH($1,
[AS_HELP_STRING([--with-$1(=DIR)],[use $1, optionally looking in DIR])],
with_$1=$withval
,
with_$1=no
)
AC_donut_SEARCH_PACKAGE_sub([$1], [$2], [$3], [$4], [$5], [$6])
]
)

