#!/bin/sh

set -e

PKG=dsf2flac

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile $PKG";
	echo
	exit 1;
}

case `uname` in Darwin*) glibtoolize --copy ;;
	  *) libtoolize --copy ;; esac

echo "Generating configuration files for $PKG, please wait ..."
echo "Ignore all non-fatal warnings ..."
echo
ACLOCAL_FLAGS="$ACLOCAL_FLAGS -I m4 --install"
if test -d /usr/local/share/aclocal ; then
	ACLOCAL_FLAGS="$ACLOCAL_FLAGS -I /usr/local/share/aclocal"
fi

autoheader
aclocal $ACLOCAL_FLAGS
autoconf
automake --foreign --add-missing --force-missing --copy
autoreconf -i

if [ "$1" != "--no-configure" ]; then
	./configure $@
fi
