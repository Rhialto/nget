#!/bin/sh

# Does everything needed to get a fresh cvs checkout to compilable state.

set -e
echo "running autoconf"
autoconf
echo "running autoheader"
autoheader
echo "running ./configure --enable-maintainer-mode" "$@"
./configure --enable-maintainer-mode "$@"

