#!/usr/bin/env python
#
#    test_nget.py - test of nget
#    Copyright (C) 2002  Matthew Mueller <donut@azstarnet.com>
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

from __future__ import nested_scopes

import os, sys, unittest, glob, filecmp, re

import nntpd, util

ngetexe = os.path.join(os.pardir, 'nget')
zerofile_fn_re = re.compile(r'(\d+)\.(\d+)\.txt$')

class DecodeTestCase(unittest.TestCase):
	def setUp(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		
	def tearDown(self):
		self.servers.stop()
		self.nget.clean_all()

	def addarticles(self, testnum, dirname):
		for fn in glob.glob(os.path.join("testdata",testnum,dirname,"*")):
			if fn.endswith("~") or not os.path.isfile(fn): #ignore backup files and non-files
				continue
			self.servers.servers[0].addarticle(["test"], nntpd.FileArticle(open(fn, 'r')))

	def do_test(self, testnum, dirname):
		self.addarticles(testnum, dirname)

		self.failIf(self.nget.run("-g test -r ."), "nget process returned with an error")
		
		ok = []
		for fn in glob.glob(os.path.join("testdata",testnum,"_output","*")):
			if fn.endswith("~") or not os.path.isfile(fn): #ignore backup files and non-files
				continue
			tail = os.path.split(fn)[1]

			r = zerofile_fn_re.match(tail)
			if r:
				dfnglob = os.path.join(self.nget.tmpdir, r.group(1)+'.*.txt')
				g = glob.glob(dfnglob)
				self.failIf(len(g) == 0, "decoded zero file %s does not exist"%dfnglob)
				self.failIf(len(g) != 1, "decoded zero file %s matches multiple"%dfnglob)
				dfn = g[0]
			else:
				dfn = os.path.join(self.nget.tmpdir, tail)
			self.failUnless(os.path.exists(dfn), "decoded file %s does not exist"%dfn)
			self.failUnless(os.path.isfile(dfn), "decoded file %s is not a file"%dfn)
			self.failUnless(filecmp.cmp(fn, dfn, shallow=0), "decoded file %s differs from %s"%(dfn, fn))
			ok.append(os.path.split(dfn)[1])

		extra = [fn for fn in os.listdir(self.nget.tmpdir) if fn not in ok]
		self.failIf(extra, "extra files decoded: "+`extra`)
	
	def do_test_decodeerror(self, testnum, dirname):
		self.addarticles(testnum, dirname)

		self.failUnless(os.WEXITSTATUS(self.nget.run("-g test -r .")) & 1, "nget process did not detect decode error")
	
	def get_auto_args(self):
		#use some magic so we don't have to type out everything twice
		import inspect
		frame = inspect.currentframe().f_back.f_back
		foo, testnum, testname = frame.f_code.co_name.split('_',2)
		return testnum, testname
	
	def do_test_auto(self):
		self.do_test(*self.get_auto_args())

	def do_test_auto_decodeerror(self):
		self.do_test_decodeerror(*self.get_auto_args())
	
	def test_0001_yenc_single(self):
		self.do_test_auto()
	def test_0001_uuencode_single(self):
		self.do_test_auto()
	def test_0001_uuenview_uue_mime_single(self):
		self.do_test_auto()
	def test_0001_yenc_multi(self):
		self.do_test_auto()
	def test_0001_yenc_single_crc32_error(self):
		self.do_test_auto_decodeerror()
	def test_0001_yenc_multi_crc32_error(self):
		self.do_test_auto_decodeerror()
	def test_0001_yenc_multi_pcrc32_error(self):
		self.do_test_auto_decodeerror()
	def test_0002_yenc_multi(self):
		self.do_test_auto()
	def test_0002_uuencode_multi(self):
		self.do_test_auto()
	def test_0002_uuenview_uue_mime_multi(self):
		self.do_test_auto()
	def test_0003_newspost_uue_0(self):
		self.do_test_auto()

if __name__ == '__main__':
	unittest.main()
