#!/usr/bin/env python
#
#    ngetf.py - run nntpd and nget with a single command
#
#    Copyright (C) 2003  Matthew Mueller <donut AT dakotacom.net>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import nntpd, util
import os, sys

#allow nget executable to be tested to be overriden with TEST_NGET env var.
ngetexe = os.environ.get('TEST_NGET','nget')
if os.sep in ngetexe or (os.altsep and os.altsep in ngetexe):
	ngetexe = os.path.abspath(ngetexe)

def main():
	if len(sys.argv)<=1:
		print 'Usage: ngetf.py  [articles]...  [--  [nget args]...]'
		return 0
	articles,ngetargs = [],[]
	c = articles
	for a in sys.argv[1:]:
		if a=='--' and c is not ngetargs:
			c=ngetargs
		else:
			c.append(a)
	if not ngetargs:
		ngetargs = ['-gtest','-r.']
	servers = nntpd.NNTPD_Master(1)
	nget = util.TestNGet(ngetexe, servers.servers) 
	try:
		for a in articles:
			servers.servers[0].addarticle(["test"], nntpd.FileArticle(open(a)))
		servers.start()

		ret = nget.runv(ngetargs)
		print "nget exit status: ",ret

	finally:
		nget.clean_all()
		servers.stop()

	if ret:
		return 1
	return 0

if __name__=="__main__":
	sys.exit(main())
