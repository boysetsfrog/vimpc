AC_INIT([vimpc], [0.09.1], [https://github.com/boysetsfrog/vimpc], [vimpc], [https://github.com/boysetsfrog/vimpc])
AC_LANG_PUSH([C++])

AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([src/config.h])
AC_PROG_CXX

# Need to check this directory on bsd systems
CPPFLAGS="$CPPFLAGS -I/usr/local/include -I/opt/local/include"
CXXFLAGS="$CXXFLAGS -D__STRICT_ANSI__"
LDFLAGS="$LDFLAGS -L/opt/local/lib"

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

AX_PTHREAD([AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.])],
            [AC_MSG_ERROR(vimpc requires pthread library)])

CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
CXXFLAGS="$PTHREAD_CFLAGS $CXXFLAGS"
CC="$PTHREAD_CC"]

AX_CXX_COMPILE_STDCXX_11(noext,mandatory)

boost_default="no"

AC_MSG_CHECKING(whether to enable boost automatically)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <functional>] 
                                    [#include <atomic>] 
                                    [#include <thread>]], 
                                   [[std::thread thread; std::function<void()> function; std::atomic<bool> atom;]])],
	AC_MSG_RESULT([no]),
	[boost_default="yes"
    AC_MSG_RESULT([yes])]
)

AC_ARG_ENABLE(boost, [ --enable-boost=[no/yes] turn on/off boost support [default=$boost_default]],, enable_boost=$boost_default)
if test "x$enable_boost" = "xyes"; then
AC_DEFINE_UNQUOTED(BOOST_SUPPORT, "1", "Define to 1 if boost support enabled")

AX_BOOST_BASE([], [], [AC_MSG_ERROR(vimpc requires boost library)])
AX_BOOST_SYSTEM([], [], [AC_MSG_ERROR(vimpc requires boost system library)])
AX_BOOST_THREAD([], [], [AC_MSG_ERROR(vimpc requires boost thread library)])

AC_CHECK_HEADER(boost/thread.hpp,
                [AC_DEFINE_UNQUOTED(HAVE_BOOST_THREAD,1,Define to 1 if <boost/thread.hpp> header file)],
                [AC_MSG_ERROR(boostthread-dev library is required)])

AC_CHECK_HEADER(boost/foreach.hpp,
                [AC_DEFINE_UNQUOTED(HAVE_BOOST_FOREACH,1,Define to 1 if <boost/foreach.hpp> header file)],
                [AC_MSG_ERROR(boost-dev library is required)])

AC_CHECK_HEADER(boost/function.hpp,
                [AC_DEFINE_UNQUOTED(HAVE_BOOST_FUNCTION,1,Define to 1 if <boost/function.hpp> header file)],
                [AC_MSG_ERROR(boost-dev library is required)])

CPPFLAGS="$CPPFLAGS -DUSE_BOOST_THREAD -DUSE_BOOST_FUNCTIONAL -DUSE_BOOST_FOREACH"
LIBS="$LIBS $BOOST_SYSTEM_LIB $BOOST_THREAD_LIB"

AC_MSG_CHECKING([whether compiler supports lambda functions])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <boost/function.hpp>]], [[int a = 5; boost::function<int(int)> f = [&a](int b) { return a + b; }; f(8);]])],
	AC_MSG_RESULT([yes]),
	AC_MSG_RESULT([no])
		AC_MSG_ERROR([[Your compiler doesn't seem to support lambda functions, please upgrade (G++ >= 4.6)]]))
else
AC_MSG_CHECKING([whether compiler supports lambda functions])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <functional>]], [[int a = 5; std::function<int(int)> f = [&a](int b) { return a + b; }; f(8);]])],
	AC_MSG_RESULT([yes]),
	AC_MSG_RESULT([no])
		AC_MSG_ERROR([[Your compiler doesn't seem to support lambda functions, please upgrade (G++ >= 4.6)]]))
fi

MP_WITH_CURSES
HAVE_CURS=$mp_cv_curses
if test x$HAVE_CURS != xyes; then
      AC_MSG_ERROR([vimpc requires the curses or ncurses library. Please install before compiling vimpc.])
fi

LIBS="$LIBS $CURSES_LIB"

AC_SEARCH_LIBS([keypad], [tinfow tinfo])

PKG_CHECK_MODULES(pcre, libpcre)

AC_SUBST(pcre_LIBS)
AC_SUBST(pcre_CFLAGS)

AC_CHECK_HEADER(pcre.h,
                [],
                [AC_MSG_ERROR(libpcre-dev library is required)])

lyrics_default="yes"
AC_ARG_ENABLE(lyrics, [ --enable-lyrics=[no/yes] turn on/off lyric fetching support [default=$lyrics_default]],, enable_lyrics=$lyrics_default)

AM_CONDITIONAL([LYRICS_SUPPORT], [test x$enable_lyrics = xyes])

if test "x$enable_lyrics" = "xyes"; then
AC_DEFINE_UNQUOTED(LYRICS_SUPPORT, "1", "Define to 1 if lyrics support enabled")

PKG_CHECK_MODULES(curl, libcurl)

AC_SUBST(curl_LIBS)
AC_SUBST(curl_CFLAGS)

AC_CHECK_HEADER(curl/curl.h,
                [AC_DEFINE_UNQUOTED(HAVE_CURL_CURL_H,1,Define to 1 if <curl.h> header file)],
                [AC_MSG_ERROR(libcurl library is required)])
fi

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
