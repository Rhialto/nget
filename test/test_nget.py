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
#ngetexe = '../../nget-dev/nget'
zerofile_fn_re = re.compile(r'(\d+)\.(\d+)\.txt$')

class DecodeTest_base(unittest.TestCase):
	def addarticle_toserver(self, testnum, dirname, aname, server, **kw):
		article = nntpd.FileArticle(open(os.path.join("testdata",testnum,dirname,aname), 'r'))
		server.addarticle(["test"], article, **kw)
		return article

	def addarticles_toserver(self, testnum, dirname, server):
		for fn in glob.glob(os.path.join("testdata",testnum,dirname,"*")):
			if fn.endswith("~") or not os.path.isfile(fn): #ignore backup files and non-files
				continue
			server.addarticle(["test"], nntpd.FileArticle(open(fn, 'r')))
			
	def addarticles(self, testnum, dirname, servers=None):
		if not servers:
			servers = self.servers.servers
		for server in servers:
			self.addarticles_toserver(testnum, dirname, server)

	def verifyoutput(self, testnum, nget=None):
		if nget is None:
			nget = self.nget
		ok = []
		for fn in glob.glob(os.path.join("testdata",testnum,"_output","*")):
			if fn.endswith("~") or not os.path.isfile(fn): #ignore backup files and non-files
				continue
			tail = os.path.split(fn)[1]

			r = zerofile_fn_re.match(tail)
			if r:
				dfnglob = os.path.join(nget.tmpdir, r.group(1)+'.*.txt')
				g = glob.glob(dfnglob)
				self.failIf(len(g) == 0, "decoded zero file %s does not exist"%dfnglob)
				self.failIf(len(g) != 1, "decoded zero file %s matches multiple"%dfnglob)
				dfn = g[0]
			else:
				dfn = os.path.join(nget.tmpdir, tail)
			self.failUnless(os.path.exists(dfn), "decoded file %s does not exist"%dfn)
			self.failUnless(os.path.isfile(dfn), "decoded file %s is not a file"%dfn)
			self.failUnless(filecmp.cmp(fn, dfn, shallow=0), "decoded file %s differs from %s"%(dfn, fn))
			ok.append(os.path.split(dfn)[1])

		extra = [fn for fn in os.listdir(nget.tmpdir) if fn not in ok]
		self.failIf(extra, "extra files decoded: "+`extra`)


class DecodeTestCase(unittest.TestCase, DecodeTest_base):
	def setUp(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		
	def tearDown(self):
		self.servers.stop()
		self.nget.clean_all()

	def do_test(self, testnum, dirname):
		self.addarticles(testnum, dirname)

		self.failIf(self.nget.run("-g test -r ."), "nget process returned with an error")
		
		self.verifyoutput(testnum)
	
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
	def test_0001_yenc_single_rawtabs(self):
		self.do_test_auto()
	def test_0001_uuencode_single(self):
		self.do_test_auto()
	def test_0001_uuenview_uue_mime_single(self):
		self.do_test_auto()
	def test_0001_yenc_multi(self):
		self.do_test_auto()
	def test_0001_yenc_single_size_error(self):
		self.do_test_auto_decodeerror()
	def test_0001_yenc_multi_size_error(self):
		self.do_test_auto_decodeerror()
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
	def test_0002_uuencode_multi3(self):
		self.do_test_auto()
	def test_0002_uuenview_uue_mime_multi(self):
		self.do_test_auto()
	def test_0003_newspost_uue_0(self):
		self.do_test_auto()

	def test_article_expiry(self):
		article = self.addarticle_toserver('0001', 'uuencode_single', 'testfile.001', self.servers.servers[0])
		self.addarticles('0002', 'uuencode_multi')
		self.failIf(self.nget.run("-g test"), "nget process returned with an error")
		self.servers.servers[0].rmarticle(article.mid)
		self.failUnless(os.WEXITSTATUS(self.nget.run("-G test -r ."))==8, "nget process did not detect retrieve error")
		self.verifyoutput('0002') #should have gotten the articles the server still has.

class XoverTestCase(unittest.TestCase, DecodeTest_base):
	def setUp(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'fullxover':0}) 
		self.fxnget = util.TestNGet(ngetexe, self.servers.servers, options={'fullxover':1}) 
		self.servers.start()
		
	def tearDown(self):
		self.servers.stop()
		self.nget.clean_all()
		self.fxnget.clean_all()

	def verifynonewfiles(self):
		nf = os.listdir(self.nget.tmpdir)
		fxnf = os.listdir(self.fxnget.tmpdir)
		self.failIf(nf or fxnf, "new files: %s %s"%(nf, fxnf))

	def test_newarticle(self):
		self.addarticle_toserver('0002', 'uuencode_multi', '001', self.servers.servers[0])

		self.failIf(self.nget.run("-g test -r ."), "nget process returned with an error")
		self.failIf(self.fxnget.run("-g test -r ."), "nget process returned with an error")
		self.verifynonewfiles()
		
		self.addarticle_toserver('0002', 'uuencode_multi', '002', self.servers.servers[0])

		self.failIf(self.nget.run("-g test -r ."), "nget process returned with an error")
		self.failIf(self.fxnget.run("-g test -r ."), "nget process returned with an error")

		self.verifyoutput('0002')
		self.verifyoutput('0002', nget=self.fxnget)

	def test_oldarticle(self):
		self.addarticle_toserver('0002', 'uuencode_multi', '001', self.servers.servers[0], anum=2)

		self.failIf(self.nget.run("-g test -r ."), "nget process returned with an error")
		self.failIf(self.fxnget.run("-g test -r ."), "nget process returned with an error")
		self.verifynonewfiles()
		
		self.addarticle_toserver('0002', 'uuencode_multi', '002', self.servers.servers[0], anum=1)

		self.failIf(self.nget.run("-g test -r ."), "nget process returned with an error")
		self.failIf(self.fxnget.run("-g test -r ."), "nget process returned with an error")

		self.verifyoutput('0002')
		self.verifyoutput('0002', nget=self.fxnget)

	def test_insertedarticle(self):
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0], anum=1)
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0], anum=3)

		self.failIf(self.nget.run("-g test -r ."), "nget process returned with an error")
		self.failIf(self.fxnget.run("-g test -r ."), "nget process returned with an error")
		self.verifynonewfiles()
		
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0], anum=2)

		self.failIf(self.nget.run("-g test -r ."), "nget process returned with an error")
		self.verifynonewfiles() #fullxover=0 should not be able to find new article
		self.failIf(self.fxnget.run("-g test -r ."), "nget process returned with an error")

		self.verifyoutput('0002', nget=self.fxnget)
	
	def test_largearticlenumbers(self):
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0], anum=1)
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0], anum=2147483647)
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0], anum=4294967295L)

		self.failIf(self.nget.run("-g test -r ."), "nget process returned with an error")
		self.failIf(self.fxnget.run("-g test -r ."), "nget process returned with an error")

		self.verifyoutput('0002')
		self.verifyoutput('0002', nget=self.fxnget)

	def test_lite_largearticlenumbers(self):
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0], anum=1)
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0], anum=2147483647)
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0], anum=4294967295L)

		litelist = os.path.join(self.nget.rcdir, 'lite.lst')
		self.failIf(self.nget.run("-w %s -g test -r ."%litelist), "nget process returned with an error")
		self.verifynonewfiles()
		self.failIf(self.nget.runlite(litelist), "ngetlite process returned with an error")
		self.failIf(self.nget.run("-N -G test -r ."), "nget process returned with an error")

		self.verifyoutput('0002')


class DiscoingNNTPRequestHandler(nntpd.NNTPRequestHandler):
	def cmd_article(self, args):
		nntpd.NNTPRequestHandler.cmd_article(self, args)
		return -1

class ConnectionErrorTestCase(unittest.TestCase, DecodeTest_base):
	def tearDown(self):
		if hasattr(self, 'servers'):
			self.servers.stop()
		if hasattr(self, 'nget'):
			self.nget.clean_all()

	def test_DeadServer(self):
		servers = [nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)]
		self.nget = util.TestNGet(ngetexe, servers) 
		servers[0].server_close()
		self.failUnless(os.WEXITSTATUS(self.nget.run("-g test -r .")) & 64, "nget process did not detect connection error")
	
	def test_DeadServerRetr(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.failIf(self.nget.run("-g test"), "nget process returned with an error")
		self.servers.stop()
		del self.servers
		self.failUnless(os.WEXITSTATUS(self.nget.run("-G test -r .")) & 8, "nget process did not detect connection error")
	
	def test_OneLiveServer(self):
		self.servers = nntpd.NNTPD_Master(1)
		deadserver = nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)
		self.nget = util.TestNGet(ngetexe, [deadserver]+self.servers.servers+[deadserver], priorities=[3, 1, 3])
		deadserver.server_close()
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.failIf(self.nget.run("-g test -r ."), "nget process returned with an error")
		self.verifyoutput('0002')
	
	def test_OneLiveServerRetr(self):
		self.servers = nntpd.NNTPD_Master(1)
		deadservers = nntpd.NNTPD_Master(1)
		self.nget = util.TestNGet(ngetexe, deadservers.servers+self.servers.servers+deadservers.servers, priorities=[3, 1, 3])
		deadservers.start()
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.addarticles('0002', 'uuencode_multi', deadservers.servers)
		self.failIf(self.nget.run("-g test"), "nget process returned with an error")
		deadservers.stop()
		self.failIf(self.nget.run("-G test -r ."), "nget process returned with an error")
		self.verifyoutput('0002')
	
	def test_DiscoServer(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), DiscoingNNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()

		self.addarticles('0002', 'uuencode_multi')
		self.failIf(self.nget.run("-g test -r ."), "nget process returned with an error")
		self.verifyoutput('0002')
		
	def test_TwoDiscoServers(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), DiscoingNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), DiscoingNNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()

		self.addarticles('0002', 'uuencode_multi')
		self.failIf(self.nget.run("-g test -r ."), "nget process returned with an error")
		self.verifyoutput('0002')

	def test_ForceWrongServer(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.addarticles_toserver('0002', 'uuencode_multi', self.servers.servers[0])
		self.failIf(self.nget.run("-g test"), "nget process returned with an error")
		self.failUnless(os.WEXITSTATUS(self.nget.run("-h host1 -G test -r .")) & 8, "nget process did not detect retrieve error")


class AuthTestCase(unittest.TestCase, DecodeTest_base):
	def tearDown(self):
		if hasattr(self, 'servers'):
			self.servers.stop()
		if hasattr(self, 'nget'):
			self.nget.clean_all()

	def test_GroupAuth(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.servers.servers[0].adduser('ralph','5') #ralph has full auth
		self.servers.servers[0].adduser('','',{'group':0}) #default can't do GROUP
		self.nget = util.TestNGet(ngetexe, self.servers.servers, hostoptions=[{'user':'ralph', 'pass':'5'}])
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.failIf(self.nget.run("-g test -r ."), "nget process returned with an error")
		self.verifyoutput('0002')

	def test_lite_GroupAuth(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.servers.servers[0].adduser('ralph','5') #ralph has full auth
		self.servers.servers[0].adduser('','',{'group':0}) #default can't do GROUP
		self.nget = util.TestNGet(ngetexe, self.servers.servers, hostoptions=[{'user':'ralph', 'pass':'5'}])
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		litelist = os.path.join(self.nget.rcdir, 'lite.lst')
		self.failIf(self.nget.run("-w %s -g test -r ."%litelist), "nget process returned with an error")
		self.failIf(self.nget.runlite(litelist), "ngetlite process returned with an error")
		self.failIf(self.nget.run("-N -G test -r ."), "nget process returned with an error")
		self.verifyoutput('0002')

	def test_ArticleAuth(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.servers.servers[0].adduser('ralph','5') #ralph has full auth
		self.servers.servers[0].adduser('','',{'article':0}) #default can't do ARTICLE
		self.nget = util.TestNGet(ngetexe, self.servers.servers, hostoptions=[{'user':'ralph', 'pass':'5'}])
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.failIf(self.nget.run("-g test -r ."), "nget process returned with an error")
		self.verifyoutput('0002')
		
	def test_lite_ArticleAuth(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.servers.servers[0].adduser('ralph','5') #ralph has full auth
		self.servers.servers[0].adduser('','',{'article':0}) #default can't do ARTICLE
		self.nget = util.TestNGet(ngetexe, self.servers.servers, hostoptions=[{'user':'ralph', 'pass':'5'}])
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		litelist = os.path.join(self.nget.rcdir, 'lite.lst')
		self.failIf(self.nget.run("-w %s -g test -r ."%litelist), "nget process returned with an error")
		self.failIf(self.nget.runlite(litelist), "ngetlite process returned with an error")
		self.failIf(self.nget.run("-N -G test -r ."), "nget process returned with an error")
		self.verifyoutput('0002')
		
	def test_FailedAuth(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.servers.servers[0].adduser('ralph','5') #ralph has full auth
		self.servers.servers[0].adduser('','',{'group':0}) #default can't do GROUP
		self.nget = util.TestNGet(ngetexe, self.servers.servers, hostoptions=[{'user':'ralph', 'pass':'WRONG'}])
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.failUnless(os.WEXITSTATUS(self.nget.run("-g test -r ."))==64, "nget process did not detect auth error")

	def test_NoAuth(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.servers.servers[0].adduser('ralph','5') #ralph has full auth
		self.servers.servers[0].adduser('','',{'group':0}) #default can't do GROUP
		self.nget = util.TestNGet(ngetexe, self.servers.servers)
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.failUnless(os.WEXITSTATUS(self.nget.run("-g test -r ."))==64, "nget process did not detect auth error")


class CppUnitTestCase(unittest.TestCase):
	def test_TestRunner(self):
		self.failIf(os.system(os.path.join(os.curdir,'TestRunner')), "CppUnit TestRunner returned an error")

if __name__ == '__main__':
	unittest.main()
