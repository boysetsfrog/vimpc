AC_INIT([vimpc], [0.01], [nathan.sweetman@gmail.com], [vimpc], [http://vimpc.sourceforge.net/])

m4_include([m4/ax_boost_base.m4])
m4_include([m4/ax_boost_regex.m4])

AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([1.10 no-define])
AC_CONFIG_HEADERS([src/config.h])
AC_PROG_CXX

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
             [AC_MSG_ERROR(mpd client library is required)])

AX_BOOST_BASE([1.40],
              [LIBS="$LIBS -lboost_regex-mt"],
              [AC_MSG_ERROR(boost library > 1.40 is required)])

AX_BOOST_REGEX

AC_MSG_CHECKING(whether to enable debugging)
debug_default="no"
AC_ARG_ENABLE(debug, [ --enable-debug=[no/yes] turn on debugging [default=$debug_default]],, enable_debug=$debug_default)
if test "x$enable_debug" = "xyes"; then
   CXXFLAGS=""
   CPPFLAGS="$CPPFLAGS -g -D__DEBUG_ASSERT -Wall -Werror"
   AC_MSG_RESULT(yes)
else
   AC_MSG_RESULT(no)
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
