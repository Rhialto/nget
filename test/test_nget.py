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

import os, sys, unittest, glob, filecmp, re, time, errno

import nntpd, util

#allow nget executable to be tested to be overriden with TEST_NGET env var.
ngetexe = os.environ.get('TEST_NGET',os.path.join(os.pardir, 'nget'))
if os.sep in ngetexe or (os.altsep and os.altsep in ngetexe):
	ngetexe = os.path.abspath(ngetexe)

zerofile_fn_re = re.compile(r'(\d+)\.(\d+)\.txt$')

def textcmp(a, b):
	return open(a,'rb').read().splitlines()==open(b,'rb').read().splitlines()

class TestCase(unittest.TestCase):
	def vfailIf(self, expr, msg=None):
		"Include the expr in the error message"
		if expr:
			if msg:
				msg = '(%r) %s'%(expr, msg)
			else:
				msg = '(%r)'%(expr,)
			raise self.failureException, msg
	def vfailUnlessEqual(self, first, second, msg=None):
		"Include the exprs in the error message even if msg is given"
		if first != second:
			if msg:
				msg = '(%r != %r) %s'%(first, second, msg)
			else:
				msg = '(%r != %r)'%(first, second)
			raise self.failureException, msg
								


class DecodeTest_base:
	def addarticle_toserver(self, testnum, dirname, aname, server, **kw):
		article = nntpd.FileArticle(open(os.path.join("testdata",testnum,dirname,aname), 'rb'))
		server.addarticle(["test"], article, **kw)
		return article

	def addarticles_toserver(self, testnum, dirname, server):
		for fn in glob.glob(os.path.join("testdata",testnum,dirname,"*")):
			if fn.endswith("~") or not os.path.isfile(fn): #ignore backup files and non-files
				continue
			server.addarticle(["test"], nntpd.FileArticle(open(fn, 'rb')))

	def rmarticles_fromserver(self, testnum, dirname, server):
		for fn in glob.glob(os.path.join("testdata",testnum,dirname,"*")):
			if fn.endswith("~") or not os.path.isfile(fn): #ignore backup files and non-files
				continue
			article = nntpd.FileArticle(open(fn, 'rb'))
			server.rmarticle(article.mid)
			
	def addarticles(self, testnum, dirname, servers=None):
		if not servers:
			servers = self.servers.servers
		for server in servers:
			self.addarticles_toserver(testnum, dirname, server)
	
	def rmarticles(self, testnum, dirname, servers=None):
		if not servers:
			servers = self.servers.servers
		for server in servers:
			self.rmarticles_fromserver(testnum, dirname, server)

	def verifyoutput(self, testnums, tmpdir=None):
		if tmpdir is None:
			tmpdir = self.nget.tmpdir
		ok = []
		if type(testnums)==type(""):
			testnums=[testnums]
		outputs=[]
		for testnum in testnums:
			outputs.extend(glob.glob(os.path.join("testdata",testnum,"_output","*")))
		for fn in outputs:
			if fn.endswith("~") or not os.path.isfile(fn): #ignore backup files and non-files
				continue
			tail = os.path.split(fn)[1]

			r = zerofile_fn_re.match(tail)
			if r:
				dfnglob = os.path.join(tmpdir, r.group(1)+'.*.txt')
				g = glob.glob(dfnglob)
				self.failIf(len(g) == 0, "decoded zero file %s does not exist"%dfnglob)
				self.failIf(len(g) != 1, "decoded zero file %s matches multiple"%dfnglob)
				dfn = g[0]
			else:
				dfn = os.path.join(tmpdir, tail)
			self.failUnless(os.path.exists(dfn), "decoded file %s does not exist"%dfn)
			self.failUnless(os.path.isfile(dfn), "decoded file %s is not a file"%dfn)
			if r:
				self.failUnless(textcmp(fn, dfn), "decoded file %s differs from %s"%(dfn, fn))
			else:
				self.failUnless(filecmp.cmp(fn, dfn, shallow=0), "decoded file %s differs from %s"%(dfn, fn))
			ok.append(os.path.split(dfn)[1])

		extra = [fn for fn in os.listdir(tmpdir) if fn not in ok]
		self.failIf(extra, "extra files decoded: "+`extra`)


class DecodeTestCase(TestCase, DecodeTest_base):
	def setUp(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		
	def tearDown(self):
		self.servers.stop()
		self.nget.clean_all()

	def do_test(self, testnum, dirname):
		self.addarticles(testnum, dirname)

		self.vfailIf(self.nget.run("-g test -r ."))
		
		self.verifyoutput(testnum)
	
	def do_test_decodeerror(self, testnum, dirname):
		self.addarticles(testnum, dirname)

		self.vfailUnlessEqual(self.nget.run("-g test -r ."), 1, "nget process did not detect decode error")
	
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
	def test_0004_input(self):
		self.do_test_auto()

	def test_article_expiry(self):
		article = self.addarticle_toserver('0001', 'uuencode_single', 'testfile.001', self.servers.servers[0])
		self.addarticles('0002', 'uuencode_multi')
		self.vfailIf(self.nget.run("-g test"))
		self.servers.servers[0].rmarticle(article.mid)
		self.vfailUnlessEqual(self.nget.run("-G test -r ."), 8, "nget process did not detect retrieve error")
		self.verifyoutput('0002') #should have gotten the articles the server still has.


class RetrieveTestCase(TestCase, DecodeTest_base):
	def setUp(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.addarticles('0003', 'newspost_uue_0')
		self.addarticles('0002', 'uuencode_multi3')
		self.addarticles('0001', 'uuencode_single')
		self.servers.start()
		
	def tearDown(self):
		self.servers.stop()
		self.nget.clean_all()

	def test_mid(self):
		self.vfailIf(self.nget.run('-g test -R "mid .a67ier.6l5.1.bar. =="'))
		self.verifyoutput('0002')

	def test_not_mid(self):
		self.vfailIf(self.nget.run('-g test -R "mid .1.foo. !="'))
		self.verifyoutput(['0002','0001'])

	def test_mid_or_mid(self):
		self.vfailIf(self.nget.run('-g test -R "mid .a67ier.6l5.1.bar. == mid .1000.test. == ||"'))
		self.verifyoutput(['0002','0001'])

	def test_messageid(self):
		self.vfailIf(self.nget.run('-g test -R "messageid .a67ier.6l5.1.bar. =="'))
		self.verifyoutput('0002')
	
	def test_author(self):
		self.vfailIf(self.nget.run('-g test -R "author Matthew =="'))
		self.verifyoutput('0002')
	
	def test_r(self):
		self.vfailIf(self.nget.run('-g test -r joystick'))
		self.verifyoutput('0002')
	
	def test_r_quote(self):
		self.addarticles('0001', 'yenc_single')
		self.vfailIf(self.nget.run('-g test -r \'"\''))
		self.verifyoutput('0001')
	
	def test_r_case(self):
		self.vfailIf(self.nget.run('-g test -c -r jOyStIcK'))
		self.verifyoutput([])
	
	def test_r_nocase(self):
		self.vfailIf(self.nget.run('-g test -C -r jOyStIcK'))
		self.verifyoutput('0002')
	
	def test_multi_r_samepath(self):
		self.vfailIf(self.nget.run('-g test -r joystick -r foo'))
		self.verifyoutput(['0001','0002'])

	def test_multi_r_trysame(self):
		d1, d2 = os.path.join(self.nget.tmpdir,'d1'), os.path.join(self.nget.tmpdir,'d2')
		map(os.mkdir, (d1, d2))
		self.vfailIf(self.nget.run('-p %s -g test -r joystick -r foo -p %s -r "joystick|foo"'%(d1, d2)))
		self.verifyoutput(['0001','0002'], d1)
		self.verifyoutput([], d2)

	def test_multi_r_trydiff(self):
		d1, d2 = os.path.join(self.nget.tmpdir,'d1'), os.path.join(self.nget.tmpdir,'d2')
		map(os.mkdir, (d1, d2))
		self.vfailIf(self.nget.run('-p %s -g test -r joystick -p %s -r "joystick|foo"'%(d1, d2)))
		self.verifyoutput(['0002'], d1)
		self.verifyoutput(['0001'], d2)

	def test_subject(self):
		self.vfailIf(self.nget.run('-g test -R "subject joystick =="'))
		self.verifyoutput('0002')
	
	def test_r_l_toohigh(self):
		self.vfailIf(self.nget.run('-g test -l 434 -r .'))
		self.verifyoutput([])

	def test_r_l(self):
		self.vfailIf(self.nget.run('-g test -l 433 -r .'))
		self.verifyoutput('0002')

	def test_r_L_toolow(self):
		self.vfailIf(self.nget.run('-g test -L 15 -r .'))
		self.verifyoutput([])

	def test_r_L(self):
		self.vfailIf(self.nget.run('-g test -L 16 -r .'))
		self.verifyoutput('0001')

	def test_r_l_L(self):
		self.vfailIf(self.nget.run('-g test -l 20 -L 200 -r .'))
		self.verifyoutput('0003')

	def test_lines_le_toolow(self):
		self.vfailIf(self.nget.run('-g test -R "lines 15 <="'))
		self.verifyoutput([])

	def test_lines_le(self):
		self.vfailIf(self.nget.run('-g test -R "lines 16 <="'))
		self.verifyoutput('0001')

	def test_lines_lt_toolow(self):
		self.vfailIf(self.nget.run('-g test -R "lines 16 <"'))
		self.verifyoutput([])

	def test_lines_lt(self):
		self.vfailIf(self.nget.run('-g test -R "lines 17 <"'))
		self.verifyoutput('0001')

	def test_lines_ge_toohigh(self):
		self.vfailIf(self.nget.run('-g test -R "lines 434 >="'))
		self.verifyoutput([])

	def test_lines_ge(self):
		self.vfailIf(self.nget.run('-g test -R "lines 433 >="'))
		self.verifyoutput('0002')

	def test_lines_gt_toohigh(self):
		self.vfailIf(self.nget.run('-g test -R "lines 433 >"'))
		self.verifyoutput([])

	def test_lines_gt(self):
		self.vfailIf(self.nget.run('-g test -R "lines 432 >"'))
		self.verifyoutput('0002')

	def test_lines_eq(self):
		self.vfailIf(self.nget.run('-g test -R "lines 433 =="'))
		self.verifyoutput('0002')

	def test_lines_and_lines(self):
		self.vfailIf(self.nget.run('-g test -R "lines 20 > lines 200 < &&"'))
		self.verifyoutput('0003')

	def test_bytes(self):
		self.vfailIf(self.nget.run('-g test -R "bytes 2000 >"'))
		self.verifyoutput('0002')
	
	def test_have(self):
		self.vfailIf(self.nget.run('-g test -R "have 3 =="'))
		self.verifyoutput('0002')

	def test_req(self):
		self.vfailIf(self.nget.run('-g test -R "req 3 =="'))
		self.verifyoutput('0002')

	def test_date(self):
		self.vfailIf(self.nget.run('-g test -R "date \'Thu, 7 Mar 2002 11:20:59 +0000 (UTC)\' =="'))
		self.verifyoutput('0002')

	def test_date_iso(self):
		self.vfailIf(self.nget.run('-g test -R "date 20020307T112059+0000 =="'))
		self.verifyoutput('0002')
	
	def test_age(self):
		self.vfailIf(self.nget.run('-g test -R "age 3w2h5m1s <"'))
		self.verifyoutput([])
		self.vfailIf(self.nget.run('-g test -R "age 3w2h5m1s >"'))
		self.verifyoutput(['0002','0001','0003'])
	
	def test_R_extra_whitespace(self):
		self.vfailIf(self.nget.run('-g test -R "  \tlines  \t 20 \t  > \t  lines \t  200\t  <\t  &&\t  \t"'))
		self.verifyoutput('0003')
		
	def test_R_stack(self):
		self.vfailIf(self.nget.run('-g test -R "lines 20 > lines 200 < && bytes 2000 > bytes 90000 < && ||"'))
		self.verifyoutput(['0003','0002'])
	
	def test_R_stack4(self):
		self.vfailIf(self.nget.run('-g test -R "lines 2 > lines 200 < bytes 1000 > bytes 90000 < && && &&"'))
		self.verifyoutput(['0003'])
	
	def test_dupef(self):
		self.vfailIf(self.nget.run('-g test -r joy'))
		self.verifyoutput('0002')
		self.vfailIf(self.nget.run('-G test -U -D -r joy')) #remove from midinfo so that we can test if the file dupe check catches it
		self.vfailIf(self.nget.run('-G test -r joy'))
		self.vfailUnlessEqual(self.servers.servers[0].conns, 1)

	def test_dupef_D(self):
		self.vfailIf(self.nget.run('-g test -r joy'))
		self.verifyoutput('0002')
		self.vfailIf(self.nget.run('-G test -U -D -r joy'))
		self.vfailIf(self.nget.run('-G test -D -r joy'))
		self.verifyoutput('0002')
		self.vfailUnlessEqual(self.servers.servers[0].conns, 2)

	def test_dupei(self):
		self.vfailIf(self.nget.run('-g test -r .'))
		self.verifyoutput(['0002','0001','0003'])
		self.nget.clean_tmp()
		self.vfailIf(self.nget.run('-G test -r .'))
		self.verifyoutput([])
		self.vfailUnlessEqual(self.servers.servers[0].conns, 1)

	def test_dupei_D(self):
		self.vfailIf(self.nget.run('-g test -r .'))
		self.verifyoutput(['0002','0001','0003'])
		self.nget.clean_tmp()
		self.vfailIf(self.nget.run('-G test -D -r .'))
		self.verifyoutput(['0002','0001','0003'])
		self.vfailUnlessEqual(self.servers.servers[0].conns, 2)

	def test_p_relative(self):
		d = os.getcwd()
		try:
			os.chdir(self.nget.tmpdir)
			os.mkdir('aaa')
			os.mkdir('bbb')
			self.vfailIf(self.nget.run('-p bbb -g test -p aaa -r foo -p bbb -r joy'))
		finally:
			os.chdir(d)
		self.verifyoutput('0001', tmpdir=os.path.join(self.nget.tmpdir,'aaa'))
		self.verifyoutput('0002', tmpdir=os.path.join(self.nget.tmpdir,'bbb'))

	def test_list_p_relative(self):
		lpath = os.path.join(self.nget.rcdir, 'list.foo')
		f = open(lpath, 'w')
		f.write('-l 1')#whatever.
		f.close()
		d = os.getcwd()
		try:
			os.chdir(self.nget.tmpdir)
			os.mkdir('aaa')
			os.mkdir('bbb')
			self.vfailIf(self.nget.run('-p bbb -@ %s -g test -p aaa -r foo -p bbb -r joy'%lpath))
		finally:
			os.chdir(d)
		self.verifyoutput('0001', tmpdir=os.path.join(self.nget.tmpdir,'aaa'))
		self.verifyoutput('0002', tmpdir=os.path.join(self.nget.tmpdir,'bbb'))
	
	def test_list(self):
		ldir = os.path.join(self.nget.rcdir, 'lists')
		os.mkdir(ldir)
		lpath = os.path.join(ldir, 'list.foo')
		f = open(lpath, 'w')
		f.write('-g test -r .')
		f.close()
		self.vfailIf(self.nget.run('-@ list.foo'))
		self.verifyoutput(['0002','0001','0003'])

	def test_list_abspath(self):
		lpath = os.path.join(self.nget.rcdir, 'list.foo')
		f = open(lpath, 'w')
		f.write('-g test -r .')
		f.close()
		self.vfailIf(self.nget.run('-@ %s'%lpath))
		self.verifyoutput(['0002','0001','0003'])

	def test_list_multiline(self):
		lpath = os.path.join(self.nget.rcdir, 'list.foo')
		f = open(lpath, 'w')
		f.write('-g\ntest\n-r\n.')
		f.close()
		self.vfailIf(self.nget.run('-@ %s'%lpath))
		self.verifyoutput(['0002','0001','0003'])

	def test_list_list(self):
		lpath = os.path.join(self.nget.rcdir, 'list.foo')
		l2path = os.path.join(self.nget.rcdir, 'list2.foo')
		f = open(lpath, 'w')
		f.write('-@ %s'%l2path)
		f.close()
		f = open(l2path, 'w')
		f.write('-g\ntest\n-r\njoy')
		f.close()
		self.vfailIf(self.nget.run('-@ %s'%lpath))
		self.verifyoutput(['0002'])

	def test_list_optionscope(self):
		lpath = os.path.join(self.nget.rcdir, 'list.foo')
		f = open(lpath, 'w')
		f.write('-l 98765 -L 1 -p %s -G foog -M -w boofar -K -T\n'%self.nget.rcdir)
		f.close()
		self.vfailIf(self.nget.run('-gtest -@ %s -r joy'%lpath))
		self.verifyoutput(['0002'])

	def test_list_enoent(self):
		self.vfailUnlessEqual(self.nget.run('-@ foobar'), 4)
	
	def test_badskip_path(self):
		self.vfailUnlessEqual(self.nget.run('-p badpath -g test -r joy -p %s -r foo'%(self.nget.tmpdir)), 2)
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].retrs, 1)

	def test_badskip_temppath(self):
		self.vfailUnlessEqual(self.nget.run('-P badpath -g test -r joy -P %s -r foo'%(self.nget.tmpdir)), 2)
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].retrs, 1)

	def test_badskip_temppath_okpath(self):
		self.vfailUnlessEqual(self.nget.run('-P badpath -g test -r joy -p %s -r foo'%(self.nget.tmpdir)), 2) #-p resets -P too
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].retrs, 1)

	def test_badskip_path_oktemppath(self):
		self.vfailUnlessEqual(self.nget.run('-p badpath -g test -r joy -P %s -r foo'%(self.nget.tmpdir)), 2) #-P does not reset -p
		self.verifyoutput([])
		self.vfailUnlessEqual(self.servers.servers[0].retrs, 0)

	def test_badskip_host(self):
		self.vfailUnlessEqual(self.nget.run('-h badhost -g test -r joy -h host0 -r foo'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].retrs, 0)
		self.verifyoutput([])#bad -h should turn -g into -G, so that should have failed to get anything.
		self.vfailIf(self.nget.run('-g test')) #retrieve headers first and try again
		self.vfailUnlessEqual(self.nget.run('-h badhost -g test -r joy -h host0 -r foo'), 4)
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].retrs, 1)

	def test_badskip_pathhost(self):
		self.vfailUnlessEqual(self.nget.run('-g test -h badhost -p badpath -r . -p %s -r . -h host0 -r foo'%(self.nget.tmpdir)), 6)
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].retrs, 1)

	def test_badskip_hostpath(self):
		self.vfailUnlessEqual(self.nget.run('-g test -h badhost -p badpath -r . -h host0 -r . -p %s -r foo'%(self.nget.tmpdir)), 6)
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].retrs, 1)

	def test_badskip_okthenbadpath(self):
		self.vfailUnlessEqual(self.nget.run('-g test -r foo -p badpath -r .'), 2)
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].retrs, 1)

	def test_badskip_okthenbadhost(self):
		self.vfailUnlessEqual(self.nget.run('-g test -r foo -h badhost -r .'), 4)
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].retrs, 1)

	def test_badskip_emptyexpression(self):
		self.vfailUnlessEqual(self.nget.run('-g test -R "" -r joy'), 4)
		self.verifyoutput(['0002'])
	
	def test_badskip_retrievebadregex(self):
		self.vfailUnlessEqual(self.nget.run('-g test -r "*" -r joy'), 4)
		self.verifyoutput(['0002'])
	
	def test_badskip_retrievenogroup(self):
		self.vfailUnlessEqual(self.nget.run('-r . -g test -r joy'), 4)
		self.verifyoutput(['0002'])
	
	def test_cache_reloading_after_host(self):
		self.vfailIf(self.nget.run('-g test -r foo -h host0 -D -r foo'))
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].retrs, 2)

	def test_flush_nogroup(self):
		self.vfailUnlessEqual(self.nget.run('-F host0'), 4)
	
	def test_flush_badserver(self):
		self.vfailUnlessEqual(self.nget.run('-g test -F badserv'), 4)

	def test_bad_arg(self):
		self.vfailUnlessEqual(self.nget.run('-g test badarg -r .'), 4)
		self.verifyoutput([])

	def test_bad_argnotcomment(self):
		self.vfailUnlessEqual(self.nget.run('-g test "#badarg" -r .'), 4) #comments should not work on command line, only in listfile
		self.verifyoutput([])

	def test_list_comment(self):
		lpath = os.path.join(self.nget.rcdir, 'list.foo')
		f = open(lpath, 'w')
		f.write('-g test #comment -r . baaha\n-r joy')
		f.close()
		self.vfailIf(self.nget.run('-@ %s'%lpath))
		self.verifyoutput(['0002'])

	def test_list_quotedcomment(self):
		lpath = os.path.join(self.nget.rcdir, 'list.foo')
		f = open(lpath, 'w')
		f.write('-g test "#comment -r . baaha"\n -r .')
		f.close()
		self.vfailUnlessEqual(self.nget.run('-@ %s'%lpath), 4)
		self.verifyoutput([])


class XoverTestCase(TestCase, DecodeTest_base):
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

		self.vfailIf(self.nget.run("-g test -r ."))
		self.vfailIf(self.fxnget.run("-g test -r ."))
		self.verifynonewfiles()
		
		self.addarticle_toserver('0002', 'uuencode_multi', '002', self.servers.servers[0])

		self.vfailIf(self.nget.run("-g test -r ."))
		self.vfailIf(self.fxnget.run("-g test -r ."))

		self.verifyoutput('0002')
		self.verifyoutput('0002', tmpdir=self.fxnget.tmpdir)

	def test_oldarticle(self):
		self.addarticle_toserver('0002', 'uuencode_multi', '001', self.servers.servers[0], anum=2)

		self.vfailIf(self.nget.run("-g test -r ."))
		self.vfailIf(self.fxnget.run("-g test -r ."))
		self.verifynonewfiles()
		
		self.addarticle_toserver('0002', 'uuencode_multi', '002', self.servers.servers[0], anum=1)

		self.vfailIf(self.nget.run("-g test -r ."))
		self.vfailIf(self.fxnget.run("-g test -r ."))

		self.verifyoutput('0002')
		self.verifyoutput('0002', tmpdir=self.fxnget.tmpdir)

	def test_insertedarticle(self):
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0], anum=1)
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0], anum=3)

		self.vfailIf(self.nget.run("-g test -r ."))
		self.vfailIf(self.fxnget.run("-g test -r ."))
		self.verifynonewfiles()
		
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0], anum=2)

		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifynonewfiles() #fullxover=0 should not be able to find new article
		self.vfailIf(self.fxnget.run("-g test -r ."))

		self.verifyoutput('0002', tmpdir=self.fxnget.tmpdir)
	
	def test_largearticlenumbers(self):
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0], anum=1)
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0], anum=2147483647)
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0], anum=4294967295L)

		self.vfailIf(self.nget.run("-g test -r ."))
		self.vfailIf(self.fxnget.run("-g test -r ."))

		self.verifyoutput('0002')
		self.verifyoutput('0002', tmpdir=self.fxnget.tmpdir)

	def test_lite_largearticlenumbers(self):
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0], anum=1)
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0], anum=2147483647)
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0], anum=4294967295L)

		litelist = os.path.join(self.nget.rcdir, 'lite.lst')
		self.vfailIf(self.nget.run("-w %s -g test -r ."%litelist))
		self.verifynonewfiles()
		self.vfailIf(self.nget.runlite(litelist))
		self.vfailIf(self.nget.run("-N -G test -r ."))

		self.verifyoutput('0002')


class DelayBeforeWriteNNTPRequestHandler(nntpd.NNTPRequestHandler):
	def nwrite(self, s):
		if getattr(self.server, '_do_delay', 0):
			time.sleep(2)
		nntpd.NNTPRequestHandler.nwrite(self, s)

class DelayBeforeArticleNNTPRequestHandler(nntpd.NNTPRequestHandler):
	def cmd_article(self, args):
		time.sleep(1)
		nntpd.NNTPRequestHandler.cmd_article(self, args)

class DelayAfterArticle2NNTPRequestHandler(nntpd.NNTPRequestHandler):
	def cmd_article(self, args):
		nntpd.NNTPRequestHandler.cmd_article(self, args)
		time.sleep(2)

class DiscoingNNTPRequestHandler(nntpd.NNTPRequestHandler):
	def cmd_article(self, args):
		nntpd.NNTPRequestHandler.cmd_article(self, args)
		raise nntpd.NNTPDisconnect

class ErrorDiscoingNNTPRequestHandler(nntpd.NNTPRequestHandler):
	def cmd_article(self, args):
		nntpd.NNTPRequestHandler.cmd_article(self, args)
		raise nntpd.NNTPDisconnect("503 You are now disconnected. Have a nice day.")

class OverArticleQuotaDiscoingNNTPRequestHandler(nntpd.NNTPRequestHandler):
	def cmd_article(self, args):
		raise nntpd.NNTPDisconnect("502 Over quota.  Goodbye.")

class OverXOverQuotaDiscoingNNTPRequestHandler(nntpd.NNTPRequestHandler):
	def cmd_xover(self, args):
		raise nntpd.NNTPDisconnect("502 Over quota.  Goodbye.")

class XOver1LineDiscoingNNTPRequestHandler(nntpd.NNTPRequestHandler):
	def cmd_xover(self, args):
		self.discocountdown = 2 #allow a single xover result to pass before disconnecting
		nntpd.NNTPRequestHandler.cmd_xover(self, args)
	def nwrite(self, s):
		if hasattr(self, 'discocountdown'):
			if self.discocountdown==0:
				raise nntpd.NNTPDisconnect
			self.discocountdown -= 1
		nntpd.NNTPRequestHandler.nwrite(self, s)

class ConnectionTestCase(TestCase, DecodeTest_base):
	def tearDown(self):
		if hasattr(self, 'servers'):
			self.servers.stop()
		if hasattr(self, 'nget'):
			self.nget.clean_all()

	def test_DeadServer(self):
		servers = [nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)]
		self.nget = util.TestNGet(ngetexe, servers) 
		servers[0].server_close()
		self.vfailUnlessEqual(self.nget.run("-g test -r ."), 16, "nget process did not detect connection error")
	
	def test_DeadServerRetr(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.vfailIf(self.nget.run("-g test"))
		self.servers.stop()
		del self.servers
		self.vfailUnlessEqual(self.nget.run("-G test -r ."), 8, "nget process did not detect connection error")
	
	def test_DeadServerPenalization(self):
		self.servers = nntpd.NNTPD_Master(1)
		deadservers = nntpd.NNTPD_Master(1)
		#by setting tries and maxconnections to 1, we can observe how many times nget tried to connect to the dead server by how many times it had to (re)connect to the good server
		self.nget = util.TestNGet(ngetexe, deadservers.servers+self.servers.servers, priorities=[3, 1], options={'tries':1, 'penaltystrikes':2, 'maxconnections':1})
		self.servers.start()
		deadservers.start()
		self.addarticles('0002', 'uuencode_multi3')
		self.addarticles('0002', 'uuencode_multi3', deadservers.servers)
		self.vfailIf(self.nget.run("-g test"))
		deadservers.stop()
		self.vfailIf(self.nget.run("-G test -r ."))
		self.vfailUnlessEqual(self.servers.servers[0].conns, 3)
		self.verifyoutput('0002')
	
	def test_SleepingServerPenalization(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), DelayBeforeWriteNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, priorities=[3,1], options={'tries':1, 'timeout':1, 'penaltystrikes':2, 'maxconnections':1})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		self.vfailIf(self.nget.run("-g test"))
		self.servers.servers[0]._do_delay=1
		self.vfailIf(self.nget.run("-G test -r ."))
		self.verifyoutput('0002')
		self.vfailUnlessEqual(self.servers.servers[0].conns, 3)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 3)

	def test_OverXOverQuotaServerPenalization(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), OverXOverQuotaDiscoingNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'tries':1, 'penaltystrikes':2, 'maxconnections':1})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		self.vfailIf(self.nget.run("-g test -g test -g test -g test -g test"))
		self.vfailUnlessEqual(self.servers.servers[0].conns, 2)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 2)

	def test_OverArticleQuotaServerPenalization(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), OverArticleQuotaDiscoingNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, priorities=[3,1], options={'tries':1, 'penaltystrikes':2, 'maxconnections':1})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		self.vfailIf(self.nget.run("-g test"))
		self.vfailIf(self.nget.run("-G test -r ."))
		self.verifyoutput('0002')
		self.vfailUnlessEqual(self.servers.servers[0].conns, 3)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 3)

	def test_NoPenalty_g(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'maxconnections':1, 'penaltystrikes':1})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		self.vfailIf(self.nget.run("-g test -g test -g test"))
		self.vfailUnlessEqual(self.servers.servers[0].conns, 2)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 2)

	def test_NoPenalty_g_NoGroup(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'maxconnections':1, 'penaltystrikes':1})
		self.servers.start()
		self.vfailUnlessEqual(self.nget.run("-g test -g test -g test"), 16)
		self.vfailUnlessEqual(self.servers.servers[0].conns, 2)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 2)

	def test_NoPenalty_r(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'maxconnections':1, 'penaltystrikes':1})
		self.servers.start()
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[1])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0])
		self.vfailIf(self.nget.run("-g test"))
		self.vfailIf(self.nget.run("-G test -r ."))
		self.vfailUnlessEqual(self.servers.servers[0].conns, 3)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 2)
		self.verifyoutput('0002')

	def test_NoPenalty_r_Expired(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'maxconnections':1, 'penaltystrikes':1})
		self.servers.start()
		self.addarticles('0001', 'uuencode_single')
		self.addarticles('0002', 'uuencode_multi3')
		self.addarticles('0003', 'newspost_uue_0')
		self.vfailIf(self.nget.run("-g test"))
		self.rmarticles('0001', 'uuencode_single')
		self.rmarticles('0002', 'uuencode_multi3')
		self.rmarticles('0003', 'newspost_uue_0')
		self.vfailUnlessEqual(self.nget.run("-G test -r ."), 8)
		self.vfailUnlessEqual(self.servers.servers[0].conns, 3)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 3)

	def test_OneLiveServer(self):
		self.servers = nntpd.NNTPD_Master(1)
		deadserver = nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)
		self.nget = util.TestNGet(ngetexe, [deadserver]+self.servers.servers+[deadserver], priorities=[3, 1, 3])
		deadserver.server_close()
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifyoutput('0002')
	
	def test_OneLiveServerRetr(self):
		self.servers = nntpd.NNTPD_Master(1)
		deadservers = nntpd.NNTPD_Master(1)
		self.nget = util.TestNGet(ngetexe, deadservers.servers+self.servers.servers+deadservers.servers, priorities=[3, 1, 3])
		deadservers.start()
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.addarticles('0002', 'uuencode_multi', deadservers.servers)
		self.vfailIf(self.nget.run("-g test"))
		deadservers.stop()
		self.vfailIf(self.nget.run("-G test -r ."))
		self.verifyoutput('0002')
	
	def test_DiscoServer(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), DiscoingNNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()

		self.addarticles('0002', 'uuencode_multi')
		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifyoutput('0002')
		self.vfailUnlessEqual(self.servers.servers[0].conns, 2)
		
	def test_TwoDiscoServers(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), DiscoingNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), DiscoingNNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()

		self.addarticles('0002', 'uuencode_multi')
		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifyoutput('0002')

	def test_ForceDiscoServer(self):
		"Test if errors are handled correctly in article retrieval with force_host"
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), DiscoingNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), DiscoingNNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()

		self.addarticles('0002', 'uuencode_multi')
		self.vfailIf(self.nget.run("-g test"))
		self.vfailIf(self.nget.run("-h host1 -G test -r ."))
		self.verifyoutput('0002')
		self.vfailUnlessEqual(self.servers.servers[0].conns, 1)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 3)

	def test_TwoXOverDiscoServers(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), XOver1LineDiscoingNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), XOver1LineDiscoingNNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'tries':3})
		self.servers.start()

		self.addarticles('0001', 'yenc_multi')
		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifyoutput('0001')
		self.vfailUnlessEqual(self.servers.servers[0].conns, 3)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 3)

	def test_ForceXOverDiscoServer(self):
		"Test if errors are handled correctly in header retrieval with force_host"
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), XOver1LineDiscoingNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), XOver1LineDiscoingNNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'tries':3})
		self.servers.start()

		self.addarticles('0001', 'yenc_multi')
		self.vfailIf(self.nget.run("-h host1 -g test"))
		self.vfailUnlessEqual(self.servers.servers[0].conns, 0)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 3)
		self.vfailIf(self.nget.run("-G test -r ."))
		self.verifyoutput('0001')

	def test_ForceWrongServer(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.addarticles_toserver('0002', 'uuencode_multi', self.servers.servers[0])
		self.vfailIf(self.nget.run("-g test"))
		self.vfailUnlessEqual(self.nget.run("-h host1 -G test -r ."), 8, "nget process did not detect retrieve error")
		self.vfailUnlessEqual(self.servers.servers[0].conns, 1)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 1)

	def test_ForceServer(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.addarticles('0001', 'yenc_multi')
		self.vfailIf(self.nget.run("-h host1 -g test"))
		self.vfailUnlessEqual(self.servers.servers[0].conns, 0)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 1)
		self.vfailIf(self.nget.run("-h host0 -g test"))
		self.vfailUnlessEqual(self.servers.servers[0].conns, 1)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 1)
		self.vfailIf(self.nget.run("-h host1 -G test -r ."))
		self.vfailUnlessEqual(self.servers.servers[0].conns, 1)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 2)
		self.verifyoutput('0001')

	def test_FlushServer(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.addarticles_toserver('0002', 'uuencode_multi', self.servers.servers[0])
		self.addarticles_toserver('0001', 'yenc_multi', self.servers.servers[1])
		self.vfailIf(self.nget.run("-g test"))
		self.vfailIf(self.nget.run("-G test -F host0"))
		self.vfailIf(self.nget.run("-G test -r ."))
		self.verifyoutput('0001')
		self.vfailUnlessEqual(self.servers.servers[0].conns, 1)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 2)
	
	def test_AbruptTimeout(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), DiscoingNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[1])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0])
		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifyoutput('0002')

	def test_ErrorTimeout(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), ErrorDiscoingNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[1])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0])
		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifyoutput('0002')

	def test_SockPool(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers)
		self.servers.start()
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[1])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0])
		self.vfailIf(self.nget.run("-g test -r ."))
		self.vfailUnlessEqual(self.servers.servers[0].conns, 1)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 1)
		self.verifyoutput('0002')

	def test_timeout(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), DelayAfterArticle2NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'timeout':1})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifyoutput('0002')
		self.vfailUnlessEqual(self.servers.servers[0].conns, 3)

	def test_idletimeout(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), DelayBeforeArticleNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'idletimeout':1})
		self.servers.start()
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[1])
		self.vfailIf(self.nget.run("-g test -r ."))
		self.vfailUnlessEqual(self.servers.servers[0].conns, 1)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 2)
		self.verifyoutput('0002')

	def test_maxconnections(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'maxconnections':1})
		self.servers.start()
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[1])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0])
		self.vfailIf(self.nget.run("-g test -r ."))
		self.vfailUnlessEqual(self.servers.servers[0].conns, 3)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 2)
		self.verifyoutput('0002')

	def test_maxconnections_2(self):
		self.servers = nntpd.NNTPD_Master(3)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'maxconnections':2})
		self.servers.start()
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[2])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[1])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0])
		self.vfailIf(self.nget.run("-g test -r ."))
		self.vfailUnlessEqual(self.servers.servers[0].conns, 2)
		self.vfailUnlessEqual(self.servers.servers[1].conns, 1)
		self.vfailUnlessEqual(self.servers.servers[2].conns, 1)
		self.verifyoutput('0002')


class AuthTestCase(TestCase, DecodeTest_base):
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
		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifyoutput('0002')

	def test_lite_GroupAuth(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.servers.servers[0].adduser('ralph','5') #ralph has full auth
		self.servers.servers[0].adduser('','',{'group':0}) #default can't do GROUP
		self.nget = util.TestNGet(ngetexe, self.servers.servers, hostoptions=[{'user':'ralph', 'pass':'5'}])
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		litelist = os.path.join(self.nget.rcdir, 'lite.lst')
		self.vfailIf(self.nget.run("-w %s -g test -r ."%litelist))
		self.vfailIf(self.nget.runlite(litelist))
		self.vfailIf(self.nget.run("-N -G test -r ."))
		self.verifyoutput('0002')

	def test_ArticleAuth(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.servers.servers[0].adduser('ralph','5') #ralph has full auth
		self.servers.servers[0].adduser('','',{'article':0}) #default can't do ARTICLE
		self.nget = util.TestNGet(ngetexe, self.servers.servers, hostoptions=[{'user':'ralph', 'pass':'5'}])
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifyoutput('0002')
		
	def test_lite_ArticleAuth(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.servers.servers[0].adduser('ralph','5') #ralph has full auth
		self.servers.servers[0].adduser('','',{'article':0}) #default can't do ARTICLE
		self.nget = util.TestNGet(ngetexe, self.servers.servers, hostoptions=[{'user':'ralph', 'pass':'5'}])
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		litelist = os.path.join(self.nget.rcdir, 'lite.lst')
		self.vfailIf(self.nget.run("-w %s -g test -r ."%litelist))
		self.vfailIf(self.nget.runlite(litelist))
		self.vfailIf(self.nget.run("-N -G test -r ."))
		self.verifyoutput('0002')
		
	def test_FailedAuth(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.servers.servers[0].adduser('ralph','5') #ralph has full auth
		self.servers.servers[0].adduser('','',{'group':0}) #default can't do GROUP
		self.nget = util.TestNGet(ngetexe, self.servers.servers, hostoptions=[{'user':'ralph', 'pass':'WRONG'}])
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.vfailUnlessEqual(self.nget.run("-g test -r ."), 16, "nget process did not detect auth error")

	def test_NoAuth(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.servers.servers[0].adduser('ralph','5') #ralph has full auth
		self.servers.servers[0].adduser('','',{'group':0}) #default can't do GROUP
		self.nget = util.TestNGet(ngetexe, self.servers.servers)
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.vfailUnlessEqual(self.nget.run("-g test -r ."), 16, "nget process did not detect auth error")

#if os.system('sf --help'):
if os.system('sf date'):
	class SubterfugueTestCase(TestCase):
		def test_SUBTERFUGUE_NOT_INSTALLED(self):
			raise "SUBTERFUGUE does not appear to be installed, some tests skipped"
else:
	trickdir=os.path.abspath('tricks')
	ppath = os.environ.get('PYTHONPATH', '')
	if ppath.find(trickdir)<0:
		if ppath:
			ppath = ppath + ':'
		os.environ['PYTHONPATH'] = ppath + trickdir
	
	class SubterfugueTest_base(DecodeTest_base):
		def setUp(self):
			self.do_setUp(1, getattr(self,'ngetoptions',{}))

		def do_setUp(self, numservers, ngetoptions):
			self.servers = nntpd.NNTPD_Master(numservers)
			self.addarticles('0001', 'uuencode_single')
			self.nget = util.TestNGet(ngetexe, self.servers.servers, **ngetoptions)
			self.servers.start()
			self.sfexe = "sf -t ExitStatus"
			self.outputfn = os.path.join(self.nget.rcdir, "output.log")
			self.post = " > %s 2>&1"%self.outputfn
			self._exitstatusre = re.compile(r'### \[.*\] exited (.*) = (.*) ###$')
			
		def tearDown(self):
			self.servers.stop()
			self.nget.clean_all()

		def readoutput(self):
			self.output = open(self.outputfn,'rb').read()
			#print 'output was:',`self.output`
			print self.output
			x = self._exitstatusre.search(self.output)
			if not x:
				return ('unknown', -1)
			if x.group(1) == 'status':
				return int(x.group(2))
			return x.group(1), x.group(2)
		
		def check_for_errormsg(self, msg, err=errno.EIO, dupe=0):
			errmsg = re.escape(os.strerror(err))
			self.failUnless(re.search(msg+'.*'+errmsg, self.output), "did not find expected error message in output")
			self.failIf(re.search(msg+'.*'+msg+'.*'+errmsg, self.output), "name duplicated in error message")
			dupe2=re.search(msg+'.*'+errmsg+'.*'+msg+'.*'+errmsg, self.output, re.DOTALL)
			if not dupe:
				self.failIf(dupe2, "expected error message duplicated in output")
			else:
				self.failIf(not dupe2, "expected error message not duplicated in output")

		def runnget(self, args, trick):
			self.nget.run(args + self.post, self.sfexe + " -t \""+trick+"\" ")
			return self.readoutput()
		def runlite(self, args, trick):
			self.nget.runlite(args + self.post, self.sfexe + " -t \""+trick+"\" ")
			return self.readoutput()


	class FileTest_base:
		"Holds all the tests that need to be done with both usegz and without"
		def test_cache_openread_error(self):
			self.vfailUnlessEqual(self.runnget("-G foo -r bar", "IOError:f=[('foo,cache','r',-1)]"), 128)
			self.check_for_errormsg(r'foo,cache')

		def test_cache_read_error(self):
			self.vfailIf(self.nget.run("-g test"))
			self.vfailUnlessEqual(self.runnget("-G test -r bar", "IOError:f=[('test,cache','r',60)]"), 128)
			self.check_for_errormsg(r'test,cache')
			self.vfailUnlessEqual(self.runnget("-G test -r bar", "IOError:f=[('test,cache','r',20)]"), 128)
			self.check_for_errormsg(r'test,cache')
			self.vfailUnlessEqual(self.runnget("-G test -r bar", "IOError:f=[('test,cache','r',10)]"), 128)
			self.check_for_errormsg(r'test,cache')

		def test_cache_zerobyte_read_error(self):
			self.vfailIf(self.nget.run("-g test"))
			self.vfailUnlessEqual(self.runnget("-G test -r bar", "IOError:f=[('test,cache','r',0)]"), 128, self.UpgradeZLibMessage)
			self.check_for_errormsg(r'test,cache')

		def test_cache_close_read_error(self):
			self.vfailIf(self.nget.run("-g test"))
			self.vfailUnlessEqual(self.runnget("-G test -r bar", "IOError:f=[('test,cache','r','c')]"), 128)
			self.check_for_errormsg(r'test,cache')
			
		def test_cache_openwrite_error(self):
			self.vfailUnlessEqual(self.runnget("-g test", "IOError:f=[('test,cache','w',-1)]"), 128)
			self.check_for_errormsg(r'test,cache')

		def test_cache_zerobyte_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test", "IOError:f=[('test,cache','w',0)]"), 128)
			self.check_for_errormsg(r'test,cache')

		def test_cache_empty_zerobyte_write_error(self):
			self.vfailIf(self.nget.run("-g test -r ."))
			self.rmarticles('0001', 'uuencode_single')
			self.vfailUnlessEqual(self.runnget("-g test", "IOError:f=[('test,cache','w',0)]"), 128)
			self.check_for_errormsg(r'test,cache')

		def test_cache_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test", "IOError:f=[('test,cache','w',20)]"), 128)
			self.check_for_errormsg(r'test,cache')

		def test_cache_rename_error(self):
			self.vfailUnlessEqual(self.runnget("-g test", "IOError:r=[('test,cache.*tmp','test,cache')]"), 128)
			self.check_for_errormsg(r'test,cache.*tmp.*test,cache')

		def test_cache_close_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test", "IOError:f=[('test,cache','w','c')]"), 128)
			self.check_for_errormsg(r'test,cache')

		def test_midinfo_open_read_error(self):
			self.vfailUnlessEqual(self.runnget("-G foo -r bar", "IOError:f=[('foo,midinfo','r',-1)]"), 128)
			self.check_for_errormsg(r'foo,midinfo')

		def test_midinfo_read_error(self):
			self.vfailIf(self.nget.run("-g test -M -r ."))
			self.vfailUnlessEqual(self.runnget("-G test -r bar", "IOError:f=[('test,midinfo','r',20)]"), 128)
			self.check_for_errormsg(r'test,midinfo')

		def test_midinfo_close_read_error(self):
			self.vfailIf(self.nget.run("-g test -M -r ."))
			self.vfailUnlessEqual(self.runnget("-G test -r bar", "IOError:f=[('test,midinfo','r','c')]"), 128)
			self.check_for_errormsg(r'test,midinfo')

		def test_midinfo_zerobyte_read_error(self):
			self.vfailIf(self.nget.run("-g test -M -r ."))
			self.vfailUnlessEqual(self.runnget("-G test -r bar", "IOError:f=[('test,midinfo','r',0)]"), 128, self.UpgradeZLibMessage)
			self.check_for_errormsg(r'test,midinfo')

		def test_midinfo_zerobyte_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -M -r .", "IOError:f=[('test,midinfo','w',0)]"), 128)
			self.check_for_errormsg(r'test,midinfo')

		def test_midinfo_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -M -r .", "IOError:f=[('test,midinfo','w',20)]"), 128)
			self.check_for_errormsg(r'test,midinfo')

		def test_midinfo_close_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -M -r .", "IOError:f=[('test,midinfo','w','c')]"), 128)
			self.check_for_errormsg(r'test,midinfo')

		def test_midinfo_rename_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -M -r .", "IOError:r=[('test,midinfo.*tmp','test,midinfo')]"), 128)
			self.check_for_errormsg(r'test,midinfo.*tmp.*test,midinfo')

	
	class GZFileErrorTestCase(FileTest_base, SubterfugueTest_base, TestCase):
		UpgradeZLibMessage = "nget did not detect read error on 0-th byte.  *** Upgrade zlib. See http://nget.sf.net/patches/ ***" #update with version number when fixed version gets released
		ngetoptions={'options':{'usegz':9}}

	
	UpgradeUULibMessage = "*** Upgrade uulib. See http://nget.sf.net/patches/ *** " #update with version number when fixed version gets released
		
	class FileErrorTestCase(FileTest_base, SubterfugueTest_base, TestCase):
		UpgradeZLibMessage = None
		ngetoptions={'options':{'usegz':0}}

		def test_ngetrc_open_error(self):
			self.vfailUnlessEqual(self.runnget("-G foo", "IOError:f=[('ngetrc$','r',-1)]"), 128)
			self.failUnless(self.output.find('man nget')<0, "nget gave config help message for io error")
			self.check_for_errormsg(r'ngetrc\b')

		def test_ngetrc_read_error(self):
			self.vfailUnlessEqual(self.runnget("-G foo", "IOError:f=[('ngetrc$','r',0)]"), 128)
			self.failUnless(self.output.find('man nget')<0, "nget gave config help message for io error")
			self.check_for_errormsg(r'ngetrc\b')

		def test_ngetrc_close_error(self):
			self.vfailUnlessEqual(self.runnget("-G foo", "IOError:f=[('ngetrc$','r','c')]"), 128)
			self.failUnless(self.output.find('man nget')<0, "nget gave config help message for io error")
			self.check_for_errormsg(r'ngetrc\b')

		def test_tempfile_open_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -r .", "IOError:f=[('\.[-0-9][0-9]{2}$','w',-1)]"), 128)
			self.check_for_errormsg(r'\.[-0-9][0-9]{2}\b')
			self.verifyoutput([])

		def test_tempfile_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -r .", "IOError:f=[('\.[-0-9][0-9]{2}$','w',0)]"), 128)
			self.check_for_errormsg(r'\.[-0-9][0-9]{2}\b')
			self.verifyoutput([]) #ensure bad tempfile is deleted

		def test_tempfile_close_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -r .", "IOError:f=[('\.[-0-9][0-9]{2}$','w','c')]"), 128)
			self.check_for_errormsg(r'\.[-0-9][0-9]{2}\b')
			self.verifyoutput([]) #ensure bad tempfile is deleted

		def test_tempfile_open_read_error(self):
			self.vfailIf(self.nget.run("-g test -K -r ."))
			self.vfailUnlessEqual(self.runnget("-G test -r .", "IOError:f=[('\.[-0-9][0-9]{2}$','r',-1)]"), 1)####128?
			self.check_for_errormsg(r'\.[-0-9][0-9]{2}\b')

		def test_tempfile_zerobyte_read_error(self):
			self.vfailIf(self.nget.run("-g test -K -r ."))
			self.vfailUnlessEqual(self.runnget("-G test -r .", "IOError:f=[('\.[-0-9][0-9]{2}$','r',0)]"), 1, UpgradeUULibMessage)####128?
			self.check_for_errormsg(r'\.[-0-9][0-9]{2}\b')

		def test_tempfile_read_error(self):
			self.rmarticles('0001', 'uuencode_single')
			self.addarticles('0002', 'uuencode_multi')#need a bigger part
			self.vfailIf(self.nget.run("-g test -K -r ."))
			self.vfailUnlessEqual(self.runnget("-G test -r .", "IOError:f=[('\.001$','r',9900)]"), 1)####128?
			self.check_for_errormsg(r'\.[-0-9][0-9]{2}\b')

		#def test_tempfile_close_read_error(self):
		#	self.vfailIf(self.nget.run("-g test -K -r ."))
		#	self.vfailUnlessEqual(self.runnget("-G test -r .", "IOError:f=[('\.[-0-9][0-9]{2}$','r','c')]"), 1)####128?
		#	self.check_for_errormsg(r'\.[-0-9][0-9]{2}\b')
		
		def test_uutempfile_open_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -r .", "IOError:f=[('^/tmp/[^/]*$','w',-1)]"), 1)
			self.check_for_errormsg(r'uu_msg.*/tmp/[^/]*[ :]')

		def test_uutempfile_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -r .", "IOError:f=[('^/tmp/[^/]*$','w',0)]"), 1, UpgradeUULibMessage)
			self.check_for_errormsg(r'uu_msg.*temp file') #uulib doesn't say the filename in this message

		def test_uutempfile_close_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -r .", "IOError:f=[('^/tmp/[^/]*$','w','c')]"), 1, UpgradeUULibMessage)
			self.check_for_errormsg(r'uu_msg.*temp file') #uulib doesn't say the filename in this message

		def test_uutempfile_open_read_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -r .", "IOError:f=[('^/tmp/[^/]*$','r',-1)]"), 1)
			self.check_for_errormsg(r'uu_msg.*/tmp/[^/]*[ :]')

		def test_uutempfile_read_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -r .", "IOError:f=[('^/tmp/[^/]*$','r',0)]"), 1)
			self.check_for_errormsg(r'uu_msg.*/tmp/[^/]*[ :]')

		#def test_uutempfile_close_read_error(self):
		#	self.vfailUnlessEqual(self.runnget("-g test -r .", "IOError:f=[('^/tmp/[^/]*$','r','c')]"), 1)
		#	self.check_for_errormsg(r'uu_msg.*/tmp/[^/]*[ :]')

		def test_destfile_open_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -r .", "IOError:f=[('testfile\.txt$','w',-1)]"), 1)
			self.check_for_errormsg(r'testfile\.txt\b')

		def test_destfile_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -r .", "IOError:f=[('testfile\.txt$','w',0)]"), 1, UpgradeUULibMessage)
			self.check_for_errormsg(r'testfile\.txt\b')

		def test_destfile_close_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test -r .", "IOError:f=[('testfile\.txt$','w','c')]"), 1, UpgradeUULibMessage)
			self.check_for_errormsg(r'testfile\.txt\b')

		def test_dupefile_src_open_error(self):
			self.vfailIf(self.nget.run("-g test -r ."))
			self.vfailUnlessEqual(self.runnget("-G test -D -r .", "IOError:f=[('testfile\.txt$','r',-1)]"), 128)
			self.check_for_errormsg(r'testfile\.txt[^.]')
		
		def test_dupefile_src_read_error(self):
			self.vfailIf(self.nget.run("-g test -r ."))
			self.vfailUnlessEqual(self.runnget("-G test -D -r .", "IOError:f=[('testfile\.txt$','r',0)]"), 128)
			self.check_for_errormsg(r'testfile\.txt[^.]')
		
		def test_dupefile_dst_open_error(self):
			self.vfailIf(self.nget.run("-g test -r ."))
			self.vfailUnlessEqual(self.runnget("-G test -D -r .", "IOError:f=[('testfile\.txt\..*\..*$','r',-1)]"), 128)
			self.check_for_errormsg(r'testfile\.txt\..*\.')
		
		def test_dupefile_dst_read_error(self):
			self.vfailIf(self.nget.run("-g test -r ."))
			self.vfailUnlessEqual(self.runnget("-G test -D -r .", "IOError:f=[('testfile\.txt\..*\..*$','r',0)]"), 128)
			self.check_for_errormsg(r'testfile\.txt\..*\.')
		


	class SockErrorTestCase(SubterfugueTest_base, TestCase):
		ngetoptions={'options':{'tries':1}}

		def test_socket_error(self):
			self.vfailUnlessEqual(self.runnget("-g test", "IOError:s=[('rw',-2)]"), 16)
			self.check_for_errormsg(r'TransportEx.*127.0.0.1')

		def test_connect_error(self):
			self.vfailUnlessEqual(self.runnget("-g test", "IOError:s=[('rw',-1)]"), 16)
			self.check_for_errormsg(r'TransportEx.*127.0.0.1')

		def test_sock_write_error(self):
			self.vfailUnlessEqual(self.runnget("-g test", "IOError:s=[('w',0)]"), 16)
			self.check_for_errormsg(r'TransportEx.*127.0.0.1')

		def test_sock_read_error(self):
			self.vfailUnlessEqual(self.runnget("-g test", "IOError:s=[('r',0)]"), 16)
			self.check_for_errormsg(r'TransportEx.*127.0.0.1')

		def test_sock_close_error_onexit(self):
			self.vfailUnlessEqual(self.runnget("-g test", "IOError:s=[('rw','c')]"), 0) #error on sock close doesn't really need an error return, since it can't cause any problems.  (Any problem causing errors would be caught before sock.close() gets called.)
			self.check_for_errormsg(r'127.0.0.1')#'TransportEx.*127.0.0.1')
		
		def test_sock_close_error(self):
			#somewhat hacky stuff since we need 2 servers for this test only.
			self.tearDown()
			self.do_setUp(2, {'options':{'tries':1, 'maxconnections':1}})
			self.vfailUnlessEqual(self.runnget("-g test", "IOError:s=[('rw','c')]"), 0) #error on sock close doesn't really need an error return, since it can't cause any problems.  (Any problem causing errors would be caught before sock.close() gets called.)
			self.check_for_errormsg(r'127.0.0.1', dupe=1)#'TransportEx.*127.0.0.1')


	class LiteErrorTest_base(SubterfugueTest_base):
		def setUp(self):
			SubterfugueTest_base.setUp(self)
			self.litelist = os.path.join(self.nget.rcdir, 'lite.lst')
			try:
				self.vfailIf(self.nget.run("-w %s -g test -r ."%self.litelist))
			except:
				self.tearDown()
				raise

	class LiteFileErrorTestCase(LiteErrorTest_base, TestCase):
		def test_list_open_error(self):
			self.runlite(self.litelist, "IOError:f=[('lite\.lst$','r',-1)]") #ngetlite doesn't signal this .. hrm
			self.check_for_errormsg(r'lite\.lst\b')
			self.verifyoutput([])

		def test_list_read_error(self):
			self.runlite(self.litelist, "IOError:f=[('lite\.lst$','r',0)]") #ngetlite doesn't signal this .. hrm
			self.check_for_errormsg(r'lite\.lst\b')
			self.verifyoutput([])

		def test_tempfile_open_error(self):
			self.vfailUnlessEqual(self.runlite(self.litelist, r"IOError:f=[('ngetlite\.\d+$','w',-1)]"), 255)
			self.check_for_errormsg(r'ngetlite\.\d+\b')
			self.verifyoutput([])

		def test_tempfile_write_error(self):
			self.vfailUnlessEqual(self.runlite(self.litelist, r"IOError:f=[('ngetlite\.\d+$','w',0)]"), 255)
			self.check_for_errormsg(r'ngetlite\.\d+\b')
			#self.verifyoutput([])

		def test_tempfile_close_error(self):
			self.vfailUnlessEqual(self.runlite(self.litelist, r"IOError:f=[('ngetlite\.\d+$','w','c')]"), 255)
			self.check_for_errormsg(r'ngetlite\.\d+\b')
			#self.verifyoutput([])
	
	class LiteSockErrorTestCase(LiteErrorTest_base, TestCase):
		def setUp(self):
			LiteErrorTest_base.setUp(self)
			os.environ['NGETLITE_TRIES'] = '1'
			os.environ['NGETLITE_TIMEOUT'] = '10'

		def test_socket_error(self):
			#self.failUnlessEqual(
			self.runlite(self.litelist, "IOError:s=[('rw',-2)]")#, 64)
			self.check_for_errormsg(r'TransportEx.*127.0.0.1')

		def test_connect_error(self):
			#self.failUnlessEqual(self.runlite(self.litelist, "IOError:s=[('rw',-1)]"), 64)
			self.runlite(self.litelist, "IOError:s=[('rw',-1)]")
			self.check_for_errormsg(r'TransportEx.*127.0.0.1')

		def test_sock_write_error(self):
			#self.failUnlessEqual(
			self.runlite(self.litelist, "IOError:s=[('w',0)]")#, 64)
			self.check_for_errormsg(r'TransportEx.*127.0.0.1')

		def test_sock_read_error(self):
			#self.failUnlessEqual(
			self.runlite(self.litelist, "IOError:s=[('r',0)]")#, 64)
			self.check_for_errormsg(r'TransportEx.*127.0.0.1')

		def test_sock_close_error(self):
			self.failUnlessEqual(self.runlite(self.litelist, "IOError:s=[('rw','c')]"), 0) #error on sock close doesn't really need an error return, since it can't cause any problems.  (Any problem causing errors would be caught before sock.close() gets called.)
			self.check_for_errormsg(r'127.0.0.1')#'TransportEx.*127.0.0.1')



class CppUnitTestCase(TestCase):
	def test_TestRunner(self):
		self.vfailIf(util.exitstatus(os.system(os.path.join(os.curdir,'TestRunner'))), "CppUnit TestRunner returned an error")

if __name__ == '__main__':
	unittest.main()
