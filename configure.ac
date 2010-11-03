AC_INIT([vimpc], [0.01], [nathan.sweetman@gmail.com], [vimpc], [http://vimpc.sourceforge.net/])
AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([1.10 no-define])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CXX

AC_MSG_CHECKING(whether to enable debugging)
debug_default="no"
AC_ARG_ENABLE(debug, [ --enable-debug=[no/yes] turn on debugging [default=$debug_default]],, enable_debug=$debug_default)
if test "x$enable_debug" = "xyes"; then
   CXXFLAGS=""
   CPPFLAGS="$CPPFLAGS -g -D_DEBUG -Wall -Werror"
   AC_MSG_RESULT(yes)
else
   AC_MSG_RESULT(no)
fi

AC_CHECK_LIB([ncurses],
             [initscr],
             [LIBS="$LIBS -lncurses"],
             [AC_MSG_ERROR(ncurses library is required)])

AC_CHECK_LIB([mpdclient],
             [mpd_connection_new],
             [LIBS="$LIBS -lmpdclient"],
             [AC_MSG_ERROR(mpd client library is required)])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

