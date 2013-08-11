# vimpc
> Client for mpd with vi-like key bindings

vimpc provides an alternative to other mpd clients (such as ncmpc and ncmpcpp) that tries to provide an interface similar to that of the vim text editor.

Type `:help` in the client or see `doc/help.txt` for more details.

![Screenshot](/doc/screenshot.png "Screenshot")

## Installation

    ./autogen.sh
    ./configure
    make
    # as root:
    make install

## Dependencies
    * libmpdclient
    * pcre
    * libncursesw
    * taglib (can be disabled with ./configure --enable-taglib=no)

> NOTE: On debian you will also require the *-dev packages. All Debian
> dependencies can be installed with the following command:

    sudo apt-get install build-essential autoconf \
        libmpdclient2 libmpdclient-dev libpcre3 libpcre3-dev \
        libncursesw5 libncursesw5-dev libncurses5-dev \
        libtagc0 libtagc0-dev

## License

Copyright (c) 2010 - 2013 Nathan Sweetman

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
