AC_INIT([vimpc], [0.02.2], [http://sourceforge.net/apps/trac/], [vimpc], [http://vimpc.sourceforge.net/])
AC_LANG_PUSH([C++])

AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([1.10 no-define])
AC_CONFIG_HEADERS([src/config.h])
AC_PROG_CXX

# Need to check this directory on bsd systems
CPPFLAGS="$CPPFLAGS -I/usr/local/include"

AC_CHECK_HEADERS(sys/ioctl.h termios.h) 

AC_MSG_CHECKING(for svnversion)
if hash svnversion; then
   AC_MSG_RESULT(yes)
   PACKAGE_SVN_REVISION=`svnversion`
   AC_DEFINE_UNQUOTED(PACKAGE_SVN_REVISION, "${PACKAGE_SVN_REVISION}", "Define to the subversion repository version of this package.")
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
            -D__DEBUG_ASSERT  \\
            -Wall -Wextra -W  \\
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
