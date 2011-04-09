AC_INIT([vimpc], [0.01], [http://sourceforge.net/apps/trac/], [vimpc], [http://vimpc.sourceforge.net/])

AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([1.10 no-define])
AC_CONFIG_HEADERS([src/config.h])
AC_PROG_CXX

AC_CHECK_HEADERS(sys/ioctl.h termios.h) 

AC_MSG_CHECKING(for svnversion)
if type -P svnversion; then
   PACKAGE_SVN_REVISION=`svnversion`
   AC_DEFINE_UNQUOTED(PACKAGE_SVN_REVISION, "${PACKAGE_SVN_REVISION}", "Define to the subversion repository version of this package.")
else
   AC_MSG_RESULT(no)
fi

AC_CHECK_LIB([ncursesw],
             [addwstr],
             [LIBS="$LIBS -lncursesw"],
             [AC_MSG_ERROR(ncursesw library is required)])

AC_CHECK_LIB([mpdclient],
             [mpd_connection_new],
             [LIBS="$LIBS -lmpdclient"],
             [AC_MSG_ERROR(mpdclient library is required)])

AC_CHECK_LIB([pcrecpp],
             [pcre_free],
             [LIBS="$LIBS -lpcrecpp"],
             [AC_MSG_ERROR(pcre library is required)])


AC_CHECK_HEADER(ncursesw/ncurses.h,
                [],
                [AC_MSG_ERROR(ncursesw-dev library is required)])

AC_CHECK_HEADER(mpd/connection.h,
                [],
                [AC_MSG_ERROR(mpdclient-dev library is required)])

AC_CHECK_HEADER(pcre.h,
                [],
                [AC_MSG_ERROR(pcre library is required)])


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
            -Wcast-qual -Wcast-align -Wmissing-declarations -Wcomment \\
            -Werror -pedantic"
   AC_MSG_RESULT(yes)
else
   AC_MSG_RESULT(no)
fi

CPPFLAGS="$CPPFLAGS -DHELP_DIRECTORY='\"$docdir\"'"

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
