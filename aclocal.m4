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
[AC_TRY_COMPILE([#include <popt.h>], [const char ** targv=NULL;poptContext c=poptGetContext(NULL,1,targv,NULL,0);],
  ac_cv_popt_const_argv=yes, ac_cv_popt_const_argv=no)])
if test $ac_cv_popt_const_argv = yes; then
  AC_DEFINE(POPT_CONST_ARGV,1,[Does popt want const argvs?])
fi
])


dnl AC_TRY_COMPILE(INCLUDES, FUNCTION-BODY,
dnl             [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
AC_DEFUN(MY_CHECK_EXCEPTIONS,
 [
  AC_MSG_CHECKING(for exception handling support)
  AC_CACHE_VAL(my_cv_exceptions,[
   AC_TRY_COMPILE([#include <stdlib.h>], [try { throw 1; } catch (int i) {int z=i;}], my_cv_exceptions=yes)
   if test -z $my_cv_exceptions; then
     CXXFLAGS="$CXXFLAGS -fhandle-exceptions"
     AC_TRY_COMPILE([#include <stdlib.h>], [try { throw 1; } catch (int i) {int z=i;}], my_cv_exceptions=yes, my_cv_exceptions=no)
   fi
  ])
  AC_MSG_RESULT($my_cv_exceptions)
  if test "x$my_cv_exceptions" = xno; then
   AC_ERROR(exceptions not supported by your compiler,
if you need a special flag try CXXFLAGS="-flag-to-enable-exceptions" ./configure)
  fi
 ]
)

AC_DEFUN(MY_CHECK_OPTIMIZATION,
 [
  AC_MSG_CHECKING(for optimization problems)
  AC_CACHE_VAL(my_cv_optimization_problems,[
   my_cv_optimization_problems=no
   opt_prob_why=", should be ok, if not report the output of $CXX -v"
   ac_try="$CXX -v 2> conftest.out"
dnl   AC_MSG_RESULT(try: $ac_try)
   AC_TRY_EVAL(ac_try)
 dnl  AC_MSG_RESULT(blah)
 dnl  cat conftest.out
 dnl  AC_MSG_RESULT(blah1)
   changequote(, )dnl
   exp="s/^[^0-9]*\([0-9]*\.[0-9]*\).*/\1/p"
   gcc_ver=`grep "gcc version" conftest.out | sed -n $exp`
 dnl  AC_MSG_RESULT(exp: $exp)
   changequote([, ])dnl
 dnl  AC_MSG_RESULT(ver: $gcc_ver)
   if test -n '$gcc_ver'; then
    gcc_major=`echo $gcc_ver | sed "s/\..*//"`
    gcc_minor=`echo $gcc_ver | sed "s/.*\.//"`
    if test -n '$gcc_major' -a -n '$gcc_minor'; then
 dnl   AC_MSG_RESULT(maj: $gcc_major min: $gcc_minor)
     if test 0$gcc_major -le 2 -a 0$gcc_minor -le 7; then
      my_cv_optimization_problems=yes
	  MY_DISABLE_OPT
      opt_prob_why=", gcc <= 2.7.x detected. optimization disabled"
     fi
    fi
   fi
  ])
  AC_MSG_RESULT([$my_cv_optimization_problems$opt_prob_why])
 ]
)

AC_DEFUN(MY_DISABLE_OPT,[
 changequote(, )dnl
 CXXFLAGS=`echo $CXXFLAGS | sed "s/-O[1-9] *//"`
 changequote([, ])dnl
])


AC_DEFUN([MY_CHECK_TERMSTUFF],
[AC_CACHE_CHECK([for working term library], [my_cv_working_termstuff],
[ac_func_search_save_LIBS=$LIBS
my_includes=["#include <term.h>
#include <stdio.h>"]
my_func=["tputs(clr_bol, 1, putchar);"]
my_cv_working_termstuff=no
AC_TRY_LINK($my_includes, $my_func, [my_cv_working_termstuff="none required"])
if test "$my_cv_working_termstuff" = no; then
  for ac_lib in termcap curses ncurses; do
    LIBS="-l$ac_lib $ac_func_search_save_LIBS"
    AC_TRY_LINK($my_includes, $my_func,
                     [my_cv_working_termstuff="-l$ac_lib"
break])
  done
fi
LIBS=$ac_func_search_save_LIBS])
AS_IF([test "$my_cv_working_termstuff" != no],
  [test "$my_cv_working_termstuff" = "none required" || LIBS="$my_cv_working_termstuff $LIBS"
  AC_DEFINE(HAVE_WORKING_TERMSTUFF,1,[Do we have a working term.h, tputs, and clr_bol?])])dnl
])


dnl @synopsis AC_caolan_FUNC_WHICH_GETHOSTBYNAME_R
dnl
dnl Provides a test to determine the correct
dnl way to call gethostbyname_r:
dnl
dnl  - defines HAVE_FUNC_GETHOSTBYNAME_R_6 if it needs 6 arguments (e.g linux)
dnl  - defines HAVE_FUNC_GETHOSTBYNAME_R_5 if it needs 5 arguments (e.g. solaris)
dnl  - defines HAVE_FUNC_GETHOSTBYNAME_R_3 if it needs 3 arguments (e.g. osf/1)
dnl
dnl If used in conjunction in gethostname.c the api demonstrated
dnl in test.c can be used regardless of which gethostbyname_r
dnl exists. These example files found at
dnl <http://www.csn.ul.ie/~caolan/publink/gethostbyname_r>.
dnl
dnl Based on David Arnold's autoconf suggestion in the threads faq.
dnl
dnl @version $Id: ac_caolan_func_which_gethostbyname_r.m4,v 1.1 2000/08/30 08:50:09 simons Exp $
dnl @author Caolan McNamara <caolan@skynet.ie>
dnl
AC_DEFUN(AC_caolan_FUNC_WHICH_GETHOSTBYNAME_R,
[AC_CACHE_CHECK(for which type of gethostbyname_r, ac_cv_func_which_gethostname_r, [
AC_CHECK_FUNC(gethostbyname_r, [
        AC_TRY_COMPILE([
#               include <netdb.h>
        ],      [

        char *name;
        struct hostent *he;
        struct hostent_data data;
        (void) gethostbyname_r(name, he, &data);

                ],ac_cv_func_which_gethostname_r=three,
                        [
dnl                     ac_cv_func_which_gethostname_r=no
  AC_TRY_COMPILE([
#   include <netdb.h>
  ], [
        char *name;
        struct hostent *he, *res;
        char buffer[2048];
        int buflen = 2048;
        int h_errnop;
        (void) gethostbyname_r(name, he, buffer, buflen, &res, &h_errnop)
  ],ac_cv_func_which_gethostname_r=six,

  [
dnl  ac_cv_func_which_gethostname_r=no
  AC_TRY_COMPILE([
#   include <netdb.h>
  ], [
                        char *name;
                        struct hostent *he;
                        char buffer[2048];
                        int buflen = 2048;
                        int h_errnop;
                        (void) gethostbyname_r(name, he, buffer, buflen, &h_errnop)
  ],ac_cv_func_which_gethostname_r=five,ac_cv_func_which_gethostname_r=no)

  ]

  )
                        ]
                )]
        ,ac_cv_func_which_gethostname_r=no)])

if test $ac_cv_func_which_gethostname_r = six; then
  AC_DEFINE(HAVE_FUNC_GETHOSTBYNAME_R_6,1,[6 arg version])
elif test $ac_cv_func_which_gethostname_r = five; then
  AC_DEFINE(HAVE_FUNC_GETHOSTBYNAME_R_5,1,[5 arg version])
elif test $ac_cv_func_which_gethostname_r = three; then
  AC_DEFINE(HAVE_FUNC_GETHOSTBYNAME_R_3,1,[3 arg version])

fi

])


dnl @synopsis AC_raf_FUNC_WHICH_GETSERVBYNAME_R
dnl
dnl Provides a test to determine the correct way to call getservbyname_r:
dnl
dnl  - defines HAVE_FUNC_GETSERVBYNAME_R_6 if it needs 6 arguments (e.g linux)
dnl  - defines HAVE_FUNC_GETSERVBYNAME_R_5 if it needs 5 arguments (e.g. solaris)
dnl  - defines HAVE_FUNC_GETSERVBYNAME_R_4 if it needs 4 arguments (e.g. osf/1)
dnl
dnl An example use can be found at http://raf.org/autoconf/net_getservbyname.c
dnl
dnl Based on Caolan McNamara's gethostbyname_r macro.
dnl Based on David Arnold's autoconf suggestion in the threads faq.
dnl
dnl @version $Id: ac_raf_func_which_getservbyname_r.m4,v 1.1 2001/08/20 09:42:26 simons Exp $
dnl @author raf <raf@raf.org>
dnl
AC_DEFUN([AC_raf_FUNC_WHICH_GETSERVBYNAME_R],
[AC_CACHE_CHECK(for getservbyname_r, ac_cv_func_which_getservbyname_r, [
AC_CHECK_FUNC(getservbyname_r, [
        AC_TRY_COMPILE([
#               include <netdb.h>
        ],      [

        char *name;
        char *proto;
        struct servent *se;
        struct servent_data data;
        (void) getservbyname_r(name, proto, se, &data);

                ],ac_cv_func_which_getservbyname_r=four,
                        [
  AC_TRY_COMPILE([
#   include <netdb.h>
  ], [
        char *name;
        char *proto;
        struct servent *se, *res;
        char buffer[2048];
        int buflen = 2048;
        (void) getservbyname_r(name, proto, se, buffer, buflen, &res)
  ],ac_cv_func_which_getservbyname_r=six,

  [
  AC_TRY_COMPILE([
#   include <netdb.h>
  ], [
        char *name;
        char *proto;
        struct servent *se;
        char buffer[2048];
        int buflen = 2048;
        (void) getservbyname_r(name, proto, se, buffer, buflen)
  ],ac_cv_func_which_getservbyname_r=five,ac_cv_func_which_getservbyname_r=no)

  ]

  )
                        ]
                )]
        ,ac_cv_func_which_getservbyname_r=no)])

if test $ac_cv_func_which_getservbyname_r = six; then
  AC_DEFINE(HAVE_FUNC_GETSERVBYNAME_R_6,1,[6 arg version])
elif test $ac_cv_func_which_getservbyname_r = five; then
  AC_DEFINE(HAVE_FUNC_GETSERVBYNAME_R_5,1,[5 arg version])
elif test $ac_cv_func_which_getservbyname_r = four; then
  AC_DEFINE(HAVE_FUNC_GETSERVBYNAME_R_4,1,[4 arg version])

fi

])

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
AC_DEFUN([AC_donut_CHECK_PACKAGE_sub],
[
AC_ARG_WITH($1-prefix,
[AC_HELP_STRING([--with-$1-prefix=DIR],[use $1 and look in DIR/{include,lib}/])],
if test "$withval" != no; then
	with_$1=yes
	if test "$withval" != yes; then
		$1_include="${withval}/include"
		$1_libdir="${withval}/lib"
	fi
fi
)

AC_ARG_WITH($1-include,
[AC_HELP_STRING([--with-$1-include=DIR],[specify exact include dir for $1 headers])],
$1_include="$withval")

AC_ARG_WITH($1-lib,
[AC_HELP_STRING([--with-$1-lib=DIR],[specify exact library dir for $1 library])],
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
        AC_CHECK_LIB($3,$2,,no_good=yes)
        AC_CHECK_HEADER($4,,no_good=yes)
        if test "$no_good" = yes; then
dnl     broken
                ifelse([$6], , , [$6])

                LIBS=$OLD_LIBS
                LDFLAGS=$OLD_LDFLAGS
                CPPFLAGS=$OLD_CPPFLAGS
                CFLAGS=$OLD_CFLAGS
        else
dnl     fixed
                ifelse([$5], , , [$5])

                AC_DEFINE(HAVE_PKG_$1,1,[Define if you have $3 and $4])
        fi

fi

])

dnl package that defaults to enabled
AC_DEFUN([AC_donut_CHECK_PACKAGE_DEF],
[
AC_ARG_WITH($1,
AC_HELP_STRING([--without-$1], [disables $1 usage completely])
AC_HELP_STRING([--with-$1=DIR], [look in DIR for $1]),
with_$1=$withval
,
with_$1=yes
)
AC_donut_CHECK_PACKAGE_sub($1, $2, $3, $4, $5, $6)
]
)

dnl package that defaults to disabled
AC_DEFUN([AC_donut_CHECK_PACKAGE],
[
AC_ARG_WITH($1,
[AC_HELP_STRING([--with-$1(=DIR)], [use $1, optionally looking in DIR])],
with_$1=$withval
,
with_$1=no
)
AC_donut_CHECK_PACKAGE_sub($1, $2, $3, $4, $5, $6)
]
)

