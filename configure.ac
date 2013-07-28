AC_INIT([vimpc], [0.08], [https://github.com/boysetsfrog/vimpc], [vimpc], [https://github.com/boysetsfrog/vimpc])
AC_LANG_PUSH([C++])

AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([src/config.h])
AC_PROG_CXX

# Need to check this directory on bsd systems
CPPFLAGS="$CPPFLAGS -I/usr/local/include"

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

PKG_CHECK_MODULES(pcrecpp, libpcrecpp)

AC_SUBST(pcrecpp_LIBS)
AC_SUBST(pcrecpp_CFLAGS)

AC_CHECK_HEADER(mpd/connection.h,
                [],
                [AC_MSG_ERROR(mpdclient-dev library is required)])

PKG_CHECK_MODULES(mpdclient, libmpdclient)

AC_SUBST(mpdclient_LIBS)
AC_SUBST(mpdclient_CFLAGS)

AC_MSG_CHECKING(whether to enable debugging)
debug_default="no"
AC_ARG_ENABLE(debug, [ --enable-debug=[no/yes] turn on debugging [default=$debug_default]],, enable_debug=$debug_default)
if test "x$enable_debug" = "xyes"; then
   CXXFLAGS=""
   CPPFLAGS="$CPPFLAGS -g -pg \\
            -D__DEBUG_PRINTS  \\
            -D__DEBUG_ASSERT"
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


CPPFLAGS="$CPPFLAGS -std=c++11 -DHELP_DIRECTORY='\"$docdir\"'"

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
