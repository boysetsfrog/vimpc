AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS  = ${ACLOCAL_FLAGS} -I m4

vimpc_CXXFLAGS = $(taglib_CLFAGS) $(pcre_CFLAGS) $(mpdclientCFLAGS) $(AM_CXXFLAGS) $(curl_CFLAGS)
vimpc_LDADD    = $(taglib_LIBS) $(pcre_LIBS) $(mpdclient_LIBS) $(curl_LIBS)

bin_PROGRAMS     = vimpc 

if BUILD_TEST
vimpc_CXXFLAGS += $(cppunit_CLFAGS)
vimpc_LDADD    += $(cppunit_LIBS)
endif

man_MANS         = doc/vimpc.1
doc_DATA         = doc/help.txt
vimpc_SOURCES    = src/algorithm.cpp \
                   src/algorithm.hpp \
                   src/assert.hpp \
                   src/attributes.hpp \
                   src/buffers.cpp \
                   src/buffers.hpp \
                   src/callback.hpp \
                   src/clientstate.cpp \
                   src/clientstate.hpp \
                   src/colours.cpp \
                   src/colours.hpp \
                   src/compiler.hpp \
                   src/config.hpp \
                   src/errorcodes.cpp \
                   src/errorcodes.hpp \
                   src/events.cpp \
                   src/events.hpp \
                   src/mpdclient.cpp \
                   src/mpdclient.hpp \
                   src/output.cpp \
                   src/output.hpp \
                   src/player.cpp \
                   src/player.hpp \
                   src/project.hpp \
                   src/regex.cpp \
                   src/regex.hpp \
                   src/screen.cpp \
                   src/screen.hpp \
                   src/settings.cpp \
                   src/settings.hpp \
                   src/song.hpp \
                   src/song.cpp \
                   src/vimpc.cpp \
                   src/vimpc.hpp \
                   src/buffer/browse.cpp \
                   src/buffer/browse.hpp \
                   src/buffer/buffer.hpp \
                   src/buffer/library.cpp \
                   src/buffer/library.hpp \
                   src/buffer/directory.cpp \
                   src/buffer/directory.hpp \
                   src/buffer/list.hpp \
                   src/buffer/outputs.hpp \
                   src/buffer/playlist.hpp \
                   src/mode/command.cpp \
                   src/mode/command.hpp \
                   src/mode/inputmode.cpp \
                   src/mode/inputmode.hpp \
                   src/mode/mode.hpp \
                   src/mode/normal.cpp \
                   src/mode/normal.hpp \
                   src/mode/search.cpp \
                   src/mode/search.hpp \
                   src/window/browsewindow.cpp \
                   src/window/browsewindow.hpp \
                   src/window/console.cpp \
                   src/window/console.hpp \
                   src/window/debug.cpp \
                   src/window/debug.hpp \
                   src/window/directorywindow.cpp \
                   src/window/directorywindow.hpp \
                   src/window/error.cpp \
                   src/window/error.hpp \
                   src/window/help.cpp \
                   src/window/help.hpp \
                   src/window/infowindow.cpp \
                   src/window/infowindow.hpp \
                   src/window/librarywindow.cpp \
                   src/window/librarywindow.hpp \
                   src/window/listwindow.cpp \
                   src/window/listwindow.hpp \
                   src/window/modewindow.cpp \
                   src/window/modewindow.hpp \
                   src/window/outputwindow.cpp \
                   src/window/outputwindow.hpp \
                   src/window/pagerwindow.cpp \
                   src/window/pagerwindow.hpp \
                   src/window/playlistwindow.cpp \
                   src/window/playlistwindow.hpp \
                   src/window/result.hpp \
                   src/window/scrollwindow.hpp \
                   src/window/scrollwindow.cpp \
                   src/window/selectwindow.hpp \
                   src/window/selectwindow.cpp \
                   src/window/songwindow.hpp \
                   src/window/songwindow.cpp \
                   src/window/window.cpp \
                   src/window/window.hpp \
                   src/window/windowselector.cpp \
                   src/window/windowselector.hpp \
                   src/main.cpp

if LYRICS_SUPPORT
vimpc_SOURCES     += src/lyricsloader.cpp \
                     src/lyricsloader.hpp \
						   src/lyricsfetcher.cpp \
                     src/lyricsfetcher.hpp \
                     src/window/lyricswindow.cpp \
                     src/window/lyricswindow.hpp
endif

if BUILD_TEST
vimpc_SOURCES     += src/test/algorithms.cpp \
                     src/test/command.cpp \
                     src/test/regex.cpp \
                     src/test/screen.cpp \
                     src/test/settings.cpp \
                     src/test/window.cpp
endif


dist_noinst_SCRIPTS = autogen.sh

