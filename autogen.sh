#!/bin/sh

# Does everything needed to get a fresh cvs checkout to compilable state.

set -e
echo "running aclocal"
aclocal --force
echo "running autoconf"
autoconf --force
echo "running autoheader"
autoheader
echo "running ./configure --enable-maintainer-mode" "$@"
./configure --enable-maintainer-mode "$@"

