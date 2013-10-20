AC_INIT([vimpc], [0.08.1], [https://github.com/boysetsfrog/vimpc], [vimpc], [https://github.com/boysetsfrog/vimpc])
AC_LANG_PUSH([C++])

AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([src/config.h])
AC_PROG_CXX

# Need to check this directory on bsd systems
CPPFLAGS="$CPPFLAGS -I/usr/local/include"
CXXFLAGS="$CXXFLAGS -D__STRICT_ANSI__"

AC_CHECK_HEADERS(sys/ioctl.h termios.h)

AC_MSG_CHECKING(for svnversion)
if test -d .svn && hash svnversion 2>/dev/null; then
   AC_MSG_RESULT(yes)
   PACKAGE_SVN_REVISION=`svnversion`
   AC_DEFINE_UNQUOTED(PACKAGE_SVN_REVISION, "${PACKAGE_SVN_REVISION}", "Define to the subversion repository version of this package.")
else
   AC_MSG_RESULT(no)
fi

AC_MSG_CHECKING(for git)
if test -d .git && hash git 2>/dev/null; then
    AC_MSG_RESULT(yes)
    PACKAGE_GIT_REVISION=`git rev-parse HEAD`
    AC_DEFINE_UNQUOTED(PACKAGE_GIT_REVISION, "${PACKAGE_GIT_REVISION}", "Define to the git repository version of this package.")
else
    AC_MSG_RESULT(no)
fi

MP_WITH_CURSES
HAVE_CURS=$mp_cv_curses
if test x$HAVE_CURS != xyes; then
      AC_MSG_ERROR([vimpc requires the curses or ncurses library. Please install before compiling vimpc.])
fi

LIBS="$LIBS $CURSES_LIB"

AX_PTHREAD([AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.])],
            [AC_MSG_ERROR(vimpc requires pthread library)])

CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
CXXFLAGS="$PTHREAD_CFLAGS $CXXFLAGS"
CC="$PTHREAD_CC"]

PKG_CHECK_MODULES(pcrecpp, libpcrecpp)

AC_SUBST(pcrecpp_LIBS)
AC_SUBST(pcrecpp_CFLAGS)

AC_CHECK_HEADER(pcrecpp.h,
                [],
                [AC_MSG_ERROR(libpcre-dev library is required)])

taglib_default="yes"
AC_ARG_ENABLE(taglib, [ --enable-taglib=[no/yes] turn on/off taglib support [default=$taglib_default]],, enable_taglib=$taglib_default)
if test "x$enable_taglib" = "xyes"; then
AC_DEFINE_UNQUOTED(TAG_SUPPORT, "1", "Define to 1 if tag support enabled")

PKG_CHECK_MODULES(taglib, taglib)

AC_SUBST(taglib_LIBS)
AC_SUBST(taglib_CFLAGS)

AC_CHECK_HEADER(taglib/taglib.h,
                [AC_DEFINE_UNQUOTED(HAVE_TAGLIB_H,1,Define to 1 if <taglib.h> header file)],
                [AC_MSG_ERROR(taglib-dev library is required)])
fi

test_default="no"
AC_ARG_ENABLE(test, [ --enable-test=[no/yes] turn on/off test support [default=$test_default]],, enable_test=$test_default)

AM_CONDITIONAL([BUILD_TEST], [test x$enable_test = xyes])

if test "x$enable_test" = "xyes"; then
AC_DEFINE_UNQUOTED(TEST_ENABLED, "1", "Define to 1 if test support is enabled")

PKG_CHECK_MODULES(cppunit, cppunit)

AC_SUBST(cppunit_LIBS)
AC_SUBST(cppunit_CFLAGS)

AC_CHECK_HEADER(cppunit/Test.h,
                [AC_DEFINE_UNQUOTED(HAVE_TEST_H,1,Define to 1 if <Test.h> header file)],
                [AC_MSG_ERROR(libcppunit-dev library is required)])
fi

profiler_default="no"
AC_ARG_ENABLE(profiler, [ --enable-profiler=[no/yes] turn on/off profiler support [default=$profiler_default]],, enable_profiler=$profiler_default)

AM_CONDITIONAL([BUILD_PROFILER], [test x$enable_profiler = xyes])

if test "x$enable_profiler" = "xyes"; then
AC_DEFINE_UNQUOTED(PROFILER_ENABLED, "1", "Define to 1 if profiler support is enabled")

LIBS="$LIBS -lprofiler"
fi


AC_CHECK_HEADER(mpd/client.h,
                [],
                [AC_MSG_ERROR(mpdclient-dev library is required)])

PKG_CHECK_MODULES(mpdclient, libmpdclient)

AC_SUBST(mpdclient_LIBS)
AC_SUBST(mpdclient_CFLAGS)

AC_MSG_CHECKING(whether to enable debugging)
debug_default="no"
AC_ARG_ENABLE(debug, [ --enable-debug=[no/yes] turn on debugging [default=$debug_default]],, enable_debug=$debug_default)
if test "x$enable_debug" = "xyes"; then
   CPPFLAGS="$CPPFLAGS -g -D__DEBUG_PRINTS -D__DEBUG_ASSERT"
   AC_MSG_RESULT(yes)
else
   AC_MSG_RESULT(no)
fi

if test "x$enable_debug" = "xyes"; then
   AC_CHECK_HEADER(execinfo.h, [AC_DEFINE_UNQUOTED(HAVE_EXECINFO_H,1,Define to 1 if there is an <execinfo.h> header file)],[])
fi

AX_CXX_COMPILE_STDCXX_11

AC_MSG_CHECKING(whether to enable warnings)
warn_default="no"
AC_ARG_ENABLE(warnings, [ --enable-warnings=[no/yes] turn on warnings [default=$warn_default]],, enable_warnings=$warn_default)
if test "x$enable_warnings" = "xyes"; then
   CPPFLAGS="$CPPFLAGS  \\
            -Waddress -Warray-bounds -Wc++0x-compat -Wchar-subscripts -Wenum-compare -Wcomment -Wformat \\
            -Wmissing-braces -Wparentheses -Wreorder -Wreturn-type -Wsequence-point -Wsign-compare -Wstrict-aliasing \\
            -Wstrict-overflow=1 -Wswitch -Wtrigraphs -Wuninitialized \\
            -Wformat -Wold-style-cast -Woverloaded-virtual -Wctor-dtor-privacy -Wsign-promo \\
            -Wswitch-default -Wswitch-enum -Wfloat-equal -Wshadow -Winit-self -Wformat-security \\
            -Wcast-qual -Wcast-align -Wmissing-declarations -Wcomment"
   AC_MSG_RESULT(yes)
else
   AC_MSG_RESULT(no)
fi


CPPFLAGS="$CPPFLAGS -DHELP_DIRECTORY='\"$docdir\"'"

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
