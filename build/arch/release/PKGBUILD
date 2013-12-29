# Maintainer: Nathan Sweetman <nathan.sweetman@gmail.com>
pkgname=vimpc
pkgver=0.09.0
pkgrel=3
epoch=
pkgdesc="ncurses based MPD client with vi-like key bindings"
arch=('i686' 'x86_64')
url="https://sourceforge.net/projects/vimpc/"
license=('GPL3')
groups=()
depends=('libmpdclient' 'ncurses' 'pcre' 'taglib')
makedepends=()
checkdepends=()
optdepends=()
provides=()
conflicts=()
replaces=()
backup=()
options=()
install=
changelog=
source=(http://downloads.sourceforge.net/project/vimpc/Release%20$pkgver/$pkgname-$pkgver.tar.gz)
noextract=()
md5sums=('10f1da0157f8f4309b4102d5e3fc4d84')

build() {
  cd "$srcdir/$pkgname"
  ./configure --prefix=/usr
  make
}

check() {
  cd "$srcdir/$pkgname"
  make -k check
}

package() {
  cd "$srcdir/$pkgname"
  make DESTDIR="$pkgdir/" install
}

# vim:set ts=2 sw=2 et:
