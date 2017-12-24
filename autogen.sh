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

aclocal $ACLOCAL_FLAGS
autoconf
autoheader
automake --foreign --add-missing --force-missing --copy
autoreconf -if

# --------------------------------------------------------------------
# This section checks to see if the architecture is arm based and 
# suggest adding some options to the ./autogen.sh call to fix potential
# issues and presents option to continue or not. If not arm, continues
# with the configure command.
# --------------------------------------------------------------------
arch=`uname -m`

cat > text_file << EOF

This seems to be running on an ARM system and you may need to add an 
option to specify where the boost lib directory is. For example you 
may try to re-run this script, pointing to your boost location.
ie > ./autogen.sh --with-boost-libdir=/usr/lib/arm-linux-gnueabihf

EOF

case "$arch" in
	*arm*)
		cat text_file
		read -p "Do you wish to continue anyways? Note that this may fail without this option. [y/n]  " yn
		case $yn in
			[Yy]*)
				if [ "$1" != "--no-configure" ]; then
					./configure $@
					echo "sucesses"
				fi;;
			[Nn]*)
				exit;;
			*)
				echo "Please answer yes or no.";;
		esac;;
	*)
		if [ "$1" != "--no-configure" ]; then
			./configure $@
			echo "sucesses"
		fi;;
esac