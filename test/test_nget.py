#!/usr/bin/env python
#
#    test_nget.py - test of nget
#    Copyright (C) 2002-2003  Matthew Mueller <donut@azstarnet.com>
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
import StringIO
import nntpd, util

#allow nget executable to be tested to be overriden with TEST_NGET env var.
ngetexe = os.environ.get('TEST_NGET',os.path.join(os.pardir, 'nget'))
if os.sep in ngetexe or (os.altsep and os.altsep in ngetexe):
	ngetexe = os.path.abspath(ngetexe)

zerofile_fn_re = re.compile(r'(\d+)\.(\d+)\.txt$')

broken_system_return = os.system("NTDOEUNTBKFOOOBAR")==0

def textcmp(expected, decoded, mbox=0):
	if not hasattr(expected, "read"):
		expected=open(expected,'rb')
	if not hasattr(decoded, "read"):
		decoded=open(decoded,'rb')
	el=expected.read().splitlines()
	dl=decoded.read().splitlines()
	if len(el)!=len(dl):
		print "textcmp: length mismatch",len(el),len(dl)
		return 0
	for e,d in zip(el,dl):
		x = re.match(r'^\[(\w+) (.+?)(\.\d+\.\d+)?\]$',d)
		if x:
			#text files can have info about what decoded files were in them, and that info can change if the file is a dupe..
			if e != '['+x.group(1)+' '+x.group(2)+']':
				print "textcmp: decode mismatch",e,d
				return 0
		elif mbox and e.startswith("From nget"):
			if not re.match("^From nget-[0-9.]+ \w\w\w \w\w\w [ 0-9][0-9] [0-9:]{8} [0-9]{4}$", d):
				print "textcmp: From mismatch",e,d
				return 0
		else:
			if e != d:
				print "textcmp: normal mismatch",e,d
				return 0
	return 1

class TestTextcmp(unittest.TestCase):
	def test_eq(self):
		self.failUnless(textcmp(StringIO.StringIO("foo\nbar"), StringIO.StringIO("foo\r\nbar")))
	def test_eq_decodeinfo(self):
		self.failUnless(textcmp(StringIO.StringIO("foo\n[UUdata foo.txt]\n"), StringIO.StringIO("foo\r\n[UUdata foo.txt.19873.987634]\n")))
	def test_eq_mbox_From(self):
		self.failUnless(textcmp(StringIO.StringIO("From nget-foo bar"), StringIO.StringIO("From nget-0.21 Sun Sep  8 20:19:53 2002"), mbox=1))
	def test_ne(self):
		self.failIf(textcmp(StringIO.StringIO("b"), StringIO.StringIO("a")))
	def test_ne_diff_len(self):
		self.failIf(textcmp(StringIO.StringIO("a\nb"), StringIO.StringIO("a")))


class TestCase(unittest.TestCase):
	def vfailIf(self, expr, msg=None):
		"Include the expr in the error message"
		if expr:
			if msg:
				msg = '(%r) %s'%(expr, msg)
			else:
				msg = '(%r)'%(expr,)
			raise self.failureException, msg
	def vfailUnlessExitstatus(self, first, second, msg=None):
		# os.system on windows 95/98 seems to always return 0.. so just skip testing the exit status in that case. blah.
		if not broken_system_return:
			self.vfailUnlessEqual(first, second, msg)
	def vfailUnlessEqual(self, first, second, msg=None):
		"Include the exprs in the error message even if msg is given"
		if first != second:
			if msg:
				msg = '(%r != %r) %s'%(first, second, msg)
			else:
				msg = '(%r != %r)'%(first, second)
			raise self.failureException, msg
								


class DecodeTest_base:
	def addarticle_toserver(self, testnum, dirname, aname, server, groups=["test"], **kw):
		article = nntpd.FileArticle(open(os.path.join("testdata",testnum,dirname,aname), 'rb'))
		server.addarticle(groups, article, **kw)
		return article

	def addarticles_toserver(self, testnum, dirname, server, fname="*", groups=["test"]):
		for fn in glob.glob(os.path.join("testdata",testnum,dirname,fname)):
			if fn.endswith("~") or not os.path.isfile(fn): #ignore backup files and non-files
				continue
			server.addarticle(groups, nntpd.FileArticle(open(fn, 'rb')))

	def rmarticle_fromserver(self, testnum, dirname, aname, server):
		article = nntpd.FileArticle(open(os.path.join("testdata",testnum,dirname,aname), 'rb'))
		server.rmarticle(article.mid)

	def rmarticles_fromserver(self, testnum, dirname, server, fname="*"):
		for fn in glob.glob(os.path.join("testdata",testnum,dirname,fname)):
			if fn.endswith("~") or not os.path.isfile(fn): #ignore backup files and non-files
				continue
			article = nntpd.FileArticle(open(fn, 'rb'))
			server.rmarticle(article.mid)
			
	def addarticles(self, testnum, dirname, servers=None, **kw):
		if not servers:
			servers = self.servers.servers
		for server in servers:
			self.addarticles_toserver(testnum, dirname, server, **kw)
	
	def rmarticles(self, testnum, dirname, servers=None, **kw):
		if not servers:
			servers = self.servers.servers
		for server in servers:
			self.rmarticles_fromserver(testnum, dirname, server, **kw)

	def verifyoutput(self, testnums, tmpdir=None):
		if tmpdir is None:
			tmpdir = self.nget.tmpdir
		ok = []
		if type(testnums)==type(""):
			testnums=[testnums]
		outputs=[]
		if type(testnums)==type({}):
			for testnum,expected in testnums.items():
				for n in expected:
					parts = n.split('/')
					if len(parts)==1:
						outputs.append(os.path.join("testdata",testnum,"_output",n))
					elif len(parts)==2:
						outputs.append(os.path.join("testdata",testnum,parts[0],parts[1]))
		else:
			for testnum in testnums:
				outputs.extend(glob.glob(os.path.join("testdata",testnum,"_output","*")))
		for fn in outputs:
			assert os.path.exists(fn), 'testdata %s not found'%fn
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
				dfnglob = os.path.join(tmpdir, tail+'.*.*')
				gfns = glob.glob(dfnglob)
				dfn = os.path.join(tmpdir, tail)
			self.failUnless(os.path.exists(dfn), "decoded file %s does not exist"%dfn)
			self.failUnless(os.path.isfile(dfn), "decoded file %s is not a file"%dfn)
			if r:
				self.failUnless(textcmp(fn, dfn), "decoded file %s differs from %s"%(dfn, fn))
			elif tail.endswith(".mbox"):
				self.failUnless(textcmp(fn, dfn, mbox=1), "decoded mbox %s differs from %s"%(dfn, fn))
			else:
				if gfns:
					goodgfn=0
					for dfn in [dfn]+gfns:
						if filecmp.cmp(fn, dfn, shallow=0):
							goodgfn=1
							break
					self.failUnless(goodgfn, "no decoded files match %s"%(fn))
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

	def do_test(self, testnum, dirname, msg=None):
		self.addarticles(testnum, dirname)

		self.vfailIf(self.nget.run("-g test -r ."), msg)
		
		self.verifyoutput(testnum)
	
	def do_test_decodeerror(self):
		self.vfailUnlessExitstatus(self.nget.run("-g test -r ."), 1, "nget process did not detect decode error")
		self.vfailUnlessExitstatus(self.nget.run("-dF -G test -r ."), 1, "nget process did not detect decode error on 2nd run (midinfo problem?)")
	
	def get_auto_args(self):
		#use some magic so we don't have to type out everything twice
		import inspect
		frame = inspect.currentframe().f_back.f_back
		foo, testnum, testname = frame.f_code.co_name.split('_',2)
		return testnum, testname
	
	def do_test_auto(self, **kw):
		self.do_test(*self.get_auto_args(), **kw)

	def do_test_auto_decodeerror(self):
		self.addarticles(*self.get_auto_args())

		self.do_test_decodeerror()
	
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
	def test_0002_uuencode_noencodeddata_article_error(self):
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0])
		article = self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0])
		wrongarticle = nntpd.FileArticle(open(os.path.join("testdata",'0004','input','001'), 'rb'))
		article.text = wrongarticle.text
		self.do_test_decodeerror()
	def test_0002_uuenview_uue_mime_multi(self):
		self.do_test_auto()
	def test_0003_newspost_uue_0(self):
		self.do_test_auto()
	def test_0004_input(self):
		self.do_test_auto()
	def test_0005_input(self):
		self.do_test_auto()
	def test_mbox01(self):
		self.addarticles("mbox01", "input")
		self.vfailIf(self.nget.run("--text=mbox -g test -r ."))
		self.verifyoutput("mbox01")
	def test_mergesa01_input(self):
		self.do_test_auto()
	def test_textnotuu_input(self):
		self.do_test_auto(msg="your uulib likes to misidentify text as uudata.  *** See http://nget.sf.net/patches/ ***")

	def test_article_expiry(self):
		article = self.addarticle_toserver('0001', 'uuencode_single', 'testfile.001', self.servers.servers[0])
		self.addarticles('0002', 'uuencode_multi')
		self.vfailIf(self.nget.run("-g test"))
		self.servers.servers[0].rmarticle(article.mid)
		self.vfailUnlessExitstatus(self.nget.run("-G test -r ."), 8, "nget process did not detect retrieve error")
		self.verifyoutput('0002') #should have gotten the articles the server still has.

	def test_article_expiry_incomplete_status(self):
		#test that -g flushing remembers to update the incomplete status of the file
		article = self.addarticle_toserver('0002', 'uuencode_multi', '001', self.servers.servers[0], anum=1)
		self.addarticle_toserver('0002', 'uuencode_multi', '002', self.servers.servers[0], anum=2)
		self.vfailIf(self.nget.run("-g test"))
		self.servers.servers[0].rmarticle(article.mid)
		self.vfailIf(self.nget.run('-g test -r .'))#should notice the article expired and not try to get anything
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 0)
	
	def test_nosavetext_on_decodeerror(self):
		self.addarticles("0001", "yenc_single_crc32_error")

		self.vfailUnlessExitstatus(self.nget.run("--save-binary-info=yes -g test -r ."), 1, "nget process did not detect decode error")

		output = os.listdir(self.nget.tmpdir)
		if 'testfile.txt' in output:
			output.remove('testfile.txt')
		import time
		time.sleep(10)
		self.failUnless(len(output)==1, "extra output: %s"%output)
		self.failUnless(output[0].endswith(".-01"), "wrong output: %s"%output)



class RetrieveTest_base(DecodeTest_base):
	def setUp(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.addarticles('0005', 'input')
		self.addarticles('0002', 'uuencode_multi3')
		self.addarticles('0001', 'uuencode_single')
		self.servers.start()
		
	def tearDown(self):
		self.servers.stop()
		self.nget.clean_all()
	
	def test_r(self):
		self.vfailIf(self.nget_run('-g test -r joystick'))
		self.verifyoutput('0002')
	
	def test_r_quote(self):
		self.addarticles('0001', 'yenc_single')
		self.vfailIf(self.nget_run('-g test -r \'"\''))
		self.verifyoutput('0001')
	
	def test_r_case(self):
		self.vfailIf(self.nget_run('-g test -c -r jOyStIcK'))
		self.verifyoutput([])
	
	def test_r_nocase(self):
		self.vfailIf(self.nget_run('-g test -C -r jOyStIcK'))
		self.verifyoutput('0002')
	
	def test_multi_r_samepath(self):
		self.vfailIf(self.nget_run('-g test -r joystick -r foo'))
		self.verifyoutput(['0001','0002'])

	def test_multi_r_trysame(self):
		d1, d2 = os.path.join(self.nget.tmpdir,'d1'), os.path.join(self.nget.tmpdir,'d2')
		map(os.mkdir, (d1, d2))
		self.vfailIf(self.nget_run('-p %s -g test -r joystick -r foo -p %s -r joystick -r foo'%(d1, d2)))
		self.verifyoutput(['0001','0002'], d1)
		self.verifyoutput([], d2)

	def test_multi_r_trydiff(self):
		d1, d2 = os.path.join(self.nget.tmpdir,'d1'), os.path.join(self.nget.tmpdir,'d2')
		map(os.mkdir, (d1, d2))
		self.vfailIf(self.nget_run('-p %s -g test -r joystick -p %s -r joystick -r foo'%(d1, d2)))
		self.verifyoutput(['0002'], d1)
		self.verifyoutput(['0001'], d2)
	
	def test_r_l_toohigh(self):
		self.vfailIf(self.nget_run('-g test -l 434 -r .'))
		self.verifyoutput([])

	def test_r_l(self):
		self.vfailIf(self.nget_run('-g test -l 433 -r .'))
		self.verifyoutput('0002')

	def test_r_L_toolow(self):
		self.vfailIf(self.nget_run('-g test -L 15 -r .'))
		self.verifyoutput([])

	def test_r_L(self):
		self.vfailIf(self.nget_run('-g test -L 16 -r .'))
		self.verifyoutput('0001')

	def test_r_l_L(self):
		self.vfailIf(self.nget_run('-g test -l 20 -L 200 -r .'))
		self.verifyoutput('0005')
	
	def test_dupef(self):
		self.vfailIf(self.nget_run('-g test -r foo'))
		self.verifyoutput('0001')
		self.vfailIf(self.nget_run('-G test -U -D -r foo')) #remove from midinfo so that we can test if the file dupe check catches it
		self.vfailIf(self.nget_run('-G test -r foo'))
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 1)

	def test_dupef_D(self):
		self.vfailIf(self.nget_run('-g test -r foo'))
		self.verifyoutput('0001')
		self.vfailIf(self.nget_run('-G test -U -D -r foo'))
		self.vfailIf(self.nget_run('-G test -D -r foo'))
		self.verifyoutput('0001')
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 2)

	def test_dupei(self):
		self.vfailIf(self.nget_run('-g test -r .'))
		self.verifyoutput(['0002','0001','0005'])
		self.nget.clean_tmp()
		self.vfailIf(self.nget_run('-g test -r .'))
		self.verifyoutput([])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 5)

	def test_dupei_D(self):
		self.vfailIf(self.nget_run('-g test -r .'))
		self.verifyoutput(['0002','0001','0005'])
		self.nget.clean_tmp()
		self.vfailIf(self.nget_run('-g test -D -r .'))
		self.verifyoutput(['0002','0001','0005'])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 10)
	
	def test_available_overrides_group(self):
		self.vfailIf(self.nget_run('-g test -A -T -r .'))
		self.verifyoutput([])
	
	def test_group_overrides_available(self):
		self.vfailIf(self.nget_run('-a -g test -r joy'))
		self.verifyoutput(['0002'])

	def test_decode_overrides_k_and_K(self):
		self.vfailIf(self.nget_run('-k -g test --decode -r joy'))
		self.vfailIf(self.nget_run('-K -g test --decode -r foo'))
		self.verifyoutput(['0002','0001'])
	
	def test_dupepath(self):
		self.vfailIf(self.nget_run('-g test -r joy'))
		self.verifyoutput(['0002'])
		tmp2dir = os.path.join(self.nget.rcdir, 'tmp2')
		os.mkdir(tmp2dir)
		self.vfailIf(self.nget_run('-dI -G test -p %s --dupepath %s -r .'%(tmp2dir,self.nget.tmpdir)))
		self.verifyoutput(['0001','0005'],tmpdir=tmp2dir)
		
	def test_path_clears_dupepaths(self):
		self.vfailIf(self.nget_run('-g test -r joy'))
		self.verifyoutput(['0002'])
		tmp2dir = os.path.join(self.nget.rcdir, 'tmp2')
		os.mkdir(tmp2dir)
		self.vfailIf(self.nget_run('-dI -G test --dupepath %s -p %s -r .'%(self.nget.tmpdir, tmp2dir)))
		self.verifyoutput(['0001','0002','0005'],tmpdir=tmp2dir)
	
	def test_noautoparhandling(self):
		self.addarticles('par01', 'input')
		self.vfailIf(self.nget_run('-g test --no-autopar -r par.test'))
		self.verifyoutput('par01')

	def test_autoparhandling(self):
		self.addarticles('par01', 'input')
		self.vfailIf(self.nget_run('-g test -r par.test'))
		self.verifyoutput({'par01':['01.dat','02.dat','03.dat','04.dat','05.dat','a b.par']})
	
	def test_autoparhandling_existingpar(self):
		self.addarticles('par01', 'input')
		self.vfailUnlessExitstatus(self.nget_run('-g test -r "par.test.*a b.par"'), 2)
		self.vfailIf(self.nget_run('-G test -r par.test'))
		self.verifyoutput({'par01':['01.dat','02.dat','03.dat','04.dat','05.dat','a b.par']})
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 6)

	def test_autoparhandling_existingpxx(self):
		self.addarticles('par01', 'input')
		self.rmarticle_fromserver('par01','input','par',self.servers.servers[0])
		self.vfailUnlessExitstatus(self.nget_run('-g test -r "par.test.*a b.p01"'), 2)
		self.vfailIf(self.nget_run('-G test -r par.test'))
		self.verifyoutput({'par01':['01.dat','02.dat','03.dat','04.dat','05.dat','a b.p01']})
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 6)

	def test_autoparhandling_missingfile(self):
		self.addarticles('par01', 'input')
		self.rmarticle_fromserver('par01','input','dat2',self.servers.servers[0])
		self.rmarticle_fromserver('par01','input','dat4',self.servers.servers[0])
		self.vfailIf(self.nget_run('-g test -r par.test'))
		self.verifyoutput({'par01':['01.dat','03.dat','05.dat','a b.par','a b.p01','a b.p02']})
		
	def test_autoparhandling_missingfile_nofilematches(self):
		self.addarticles('par01', 'input')
		self.rmarticle_fromserver('par01','input','dat2',self.servers.servers[0])
		self.rmarticle_fromserver('par01','input','dat4',self.servers.servers[0])
		self.vfailUnlessExitstatus(self.nget_run('-g test -r "par.test.*\.[dp]a[tr]"'), 2)
		self.vfailIf(self.nget_run('-g test -r par.test'))
		self.verifyoutput({'par01':['01.dat','03.dat','05.dat','a b.par','a b.p01','a b.p02']})
		
	def test_autoparhandling_incomplete_pessimistic(self):
		self.addarticle_toserver('par01', 'input', 'dat2', self.servers.servers[0])
		self.addarticle_toserver('par01', 'input', 'par', self.servers.servers[0])
		self.addarticle_toserver('par01', 'input', 'par2', self.servers.servers[0])
		self.addarticle_toserver('par01', 'input', 'par3', self.servers.servers[0])
		self.vfailUnlessExitstatus(self.nget_run('-g test -r par.test'), 2)
		self.verifyoutput({'par01':['02.dat','a b.par','a b.p02','a b.p03']})
		
	def test_autoparhandling_incomplete_optimistic(self):
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'autopar_optimistic':1})
		self.addarticle_toserver('par01', 'input', 'dat2', self.servers.servers[0])
		self.addarticle_toserver('par01', 'input', 'par', self.servers.servers[0])
		self.addarticle_toserver('par01', 'input', 'par2', self.servers.servers[0])
		self.addarticle_toserver('par01', 'input', 'par3', self.servers.servers[0])
		self.vfailUnlessExitstatus(self.nget_run('-g test -r par.test'), 2)
		self.verifyoutput({'par01':['02.dat','a b.par']})
		
	def test_autoparhandling_corruptfile(self):
		self.addarticles('par01', 'input')
		self.rmarticle_fromserver('par01','input','dat2',self.servers.servers[0])
		self.rmarticle_fromserver('par01','input','dat4',self.servers.servers[0])
		self.addarticles('par01', 'corrupt_input')
		self.vfailIf(self.nget_run('-g test -r par.test'))
		self.verifyoutput({'par01':['01.dat','_corrupt_output/02.dat','03.dat','_corrupt_output/04.dat','05.dat','a b.par','a b.p01','a b.p02']})
		
	def test_autoparhandling_corruptfile_correctdupe(self):
		self.addarticles('par01', 'corrupt_input')
		self.vfailIf(self.nget_run('-g test -r par.test'))
		self.addarticles('par01', 'input')
		self.vfailIf(self.nget_run('-dF -g test -r par.test'))
		self.verifyoutput({'par01':['01.dat','02.dat','_corrupt_output/02.dat','03.dat','04.dat','_corrupt_output/04.dat','05.dat','a b.par']})
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 8)
		
	def test_autoparhandling_corruptpxx(self):
		self.addarticles('par01', 'input')
		self.rmarticle_fromserver('par01','input','dat2',self.servers.servers[0])
		self.rmarticle_fromserver('par01','input','dat4',self.servers.servers[0])
		self.rmarticle_fromserver('par01','input','par1',self.servers.servers[0])
		self.rmarticle_fromserver('par01','input','par3',self.servers.servers[0])
		self.addarticles('par01', 'corrupt_pxxs')
		self.vfailIf(self.nget_run('-g test -r par.test'))
		self.verifyoutput({'par01':['01.dat','03.dat','05.dat','a b.par','_corrupt_pxxs_output/a b.p01','_corrupt_pxxs_output/a b.p03','a b.p02','a b.p04']})
	
	def test_autoparhandling_reply(self):
		self.addarticles('par01', 'input')
		self.addarticles('par01', 'reply')
		self.vfailIf(self.nget_run('-g test -r par.test'))
		self.verifyoutput({'par01':['01.dat','02.dat','03.dat','04.dat','05.dat','a b.par','_reply_output/1041725934.0.txt']})
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 7)
		
	def test_autoparhandling_reply_existingpar(self):
		self.addarticles('par01', 'input')
		self.addarticles('par01', 'reply')
		self.vfailUnlessExitstatus(self.nget_run('-g test -r "par.test.*a b.par"'), 2)
		self.vfailIf(self.nget_run('-G test -r par.test'))
		self.verifyoutput({'par01':['01.dat','02.dat','03.dat','04.dat','05.dat','a b.par','_reply_output/1041725934.0.txt']})
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 7)

	def test_autoparhandling_0file(self):
		self.addarticles('par02', 'input')
		self.addarticles('par02', '0file')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par02':['p2-01.dat','p2-02.dat','p2-03.dat','p2-04.dat','p2-05.dat','p2.par','_0file_output/1041648329.0.txt']})
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 7)
	
	def test_autoparhandling_differingcasefile(self):
		self.addarticles('par02', 'input')
		self.rmarticle_fromserver('par02','input','dat4',self.servers.servers[0])
		self.rmarticle_fromserver('par02','input','dat5',self.servers.servers[0])
		self.addarticles('par02', 'case_input')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par02':['p2-01.dat','p2-02.dat','p2-03.dat','_case_output/P2-04.dAt','_case_output/p2-05.DaT','p2.par']})
		
	def test_autoparhandling_differingparfilenames(self):
		self.addarticle_toserver('par02', 'input', 'dat2', self.servers.servers[0])
		self.addarticle_toserver('par02', 'input', 'par', self.servers.servers[0])
		self.addarticle_toserver('par02', 'input', 'par2', self.servers.servers[0])
		self.addarticle_toserver('par02', 'input', 'par4', self.servers.servers[0])
		self.addarticles('par02', 'a_b_par_input')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par02':['p2-02.dat','p2.par','p2.p02', 'p2.p04', '_a_b_par_output/a b.p01', '_a_b_par_output/a b.p03', '_a_b_par_output/a b.par']})
		
	def test_autoparhandling_differingparfilenames_nopar(self):
		self.addarticle_toserver('par02', 'input', 'dat2', self.servers.servers[0])
		self.addarticle_toserver('par02', 'input', 'par2', self.servers.servers[0])
		self.addarticle_toserver('par02', 'input', 'par4', self.servers.servers[0])
		self.addarticles('par02', 'a_b_par_input')
		self.rmarticle_fromserver('par02','a_b_par_input','par',self.servers.servers[0])
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par02':['p2-02.dat','p2.p02', 'p2.p04', '_a_b_par_output/a b.p01', '_a_b_par_output/a b.p03']})
		
	def test_autoparhandling_multiparset(self):
		self.addarticles('par01', 'input')
		self.addarticles('par02', 'input')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par01':['01.dat','02.dat','03.dat','04.dat','05.dat','a b.par'],
			'par02':['p2-01.dat','p2-02.dat','p2-03.dat','p2-04.dat','p2-05.dat','p2.par']})
		
	def test_autoparhandling_multiparset_samename(self):
		self.addarticles('par01', 'input')
		self.addarticles('par02', 'input', fname='dat*')
		self.addarticles('par02', 'a_b_par_input')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par01':['01.dat','02.dat','03.dat','04.dat','05.dat','a b.par'],
			'par02':['p2-01.dat','p2-02.dat','p2-03.dat','p2-04.dat','p2-05.dat','_a_b_par_output/a b.par']})
		
	def test_autoparhandling_multiparset_samename_missingfile(self):
		self.addarticles('par01', 'input')
		self.rmarticle_fromserver('par01','input','dat2',self.servers.servers[0])
		self.rmarticle_fromserver('par01','input','dat4',self.servers.servers[0])
		self.addarticle_toserver('par02', 'input', 'dat2', self.servers.servers[0])
		self.addarticle_toserver('par02', 'input', 'dat4', self.servers.servers[0])
		self.addarticles('par02', 'a_b_par_input')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par01':['01.dat','03.dat','05.dat','a b.par','a b.p01','a b.p02']+
			['a b.p03', 'a b.p04', 'a b.p05', 'a b.p06'],#unfortunatly, these have to be grabbed first before nget will get to try the pars from the second set, since they were posted at a later date.
			'par02':['p2-02.dat','p2-04.dat','_a_b_par_output/a b.par','_a_b_par_output/a b.p01','_a_b_par_output/a b.p02','_a_b_par_output/a b.p03']})
		
	def test_autoparhandling_multiparset_existingpar(self):
		self.addarticles('par01', 'input')
		self.addarticles('par02', 'input')
		self.vfailUnlessExitstatus(self.nget_run('-g test -r "par.*test.*\.par"'), 2)
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par01':['01.dat','02.dat','03.dat','04.dat','05.dat','a b.par'],
			'par02':['p2-01.dat','p2-02.dat','p2-03.dat','p2-04.dat','p2-05.dat','p2.par']})
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 12)
		
	def test_autoparhandling_multiparset_missingapar(self):
		self.addarticles('par01', 'input')
		self.rmarticle_fromserver('par01','input','par',self.servers.servers[0])
		self.addarticles('par02', 'input')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par01':['01.dat','02.dat','03.dat','04.dat','05.dat','a b.p01'],
			'par02':['p2-01.dat','p2-02.dat','p2-03.dat','p2-04.dat','p2-05.dat','p2.par']})
		
	def test_autoparhandling_multiparset_missingfile(self):
		self.addarticles('par01', 'input')
		self.rmarticle_fromserver('par01','input','dat2',self.servers.servers[0])
		self.rmarticle_fromserver('par01','input','dat4',self.servers.servers[0])
		self.addarticles('par02', 'input')
		self.rmarticle_fromserver('par02','input','dat1',self.servers.servers[0])
		self.rmarticle_fromserver('par02','input','dat3',self.servers.servers[0])
		self.rmarticle_fromserver('par02','input','dat5',self.servers.servers[0])
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par01':['01.dat','03.dat','05.dat','a b.par','a b.p01','a b.p02'],
			'par02':['p2-02.dat','p2-04.dat','p2.par','p2.p01','p2.p02','p2.p03']})
		
	def test_autoparhandling_multiparset_missingfile_nofilematches(self):
		self.addarticles('par01', 'input')
		self.rmarticle_fromserver('par01','input','dat2',self.servers.servers[0])
		self.rmarticle_fromserver('par01','input','dat4',self.servers.servers[0])
		self.addarticles('par02', 'input')
		self.rmarticle_fromserver('par02','input','dat1',self.servers.servers[0])
		self.rmarticle_fromserver('par02','input','dat3',self.servers.servers[0])
		self.rmarticle_fromserver('par02','input','dat5',self.servers.servers[0])
		self.vfailUnlessExitstatus(self.nget_run('-g test -r "par.*test.*\.[dp]a[tr]"'), 2)
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par01':['01.dat','03.dat','05.dat','a b.par','a b.p01','a b.p02'],
			'par02':['p2-02.dat','p2-04.dat','p2.par','p2.p01','p2.p02','p2.p03']})
	
	def test_noautopar2handling(self):
		self.addarticles('par2-01', 'input')
		self.vfailIf(self.nget_run('-g test --no-autopar -r par2.test'))
		self.verifyoutput('par2-01')

	def test_autopar2handling(self):
		self.addarticles('par2-01', 'input')
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 02.dat','c d 03.dat','c d 04.dat','c d 05.dat','c d.par2']})
		
	def test_autopar2handling_existingpar(self):
		self.addarticles('par2-01', 'input')
		self.vfailUnlessExitstatus(self.nget_run('-g test -r "par2.test.*c d.par2"'), 2)
		self.vfailIf(self.nget_run('-G test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 02.dat','c d 03.dat','c d 04.dat','c d 05.dat','c d.par2']})
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 6)

	def test_autopar2handling_missingpar(self):
		self.addarticles('par2-01', 'input')
		self.rmarticle_fromserver('par2-01','input','par',self.servers.servers[0])
		self.rmarticle_fromserver('par2-01','input','par1',self.servers.servers[0])
		self.rmarticle_fromserver('par2-01','input','par2',self.servers.servers[0])
		self.rmarticle_fromserver('par2-01','input','par3',self.servers.servers[0])
		self.addarticles('par2-01', 'corrupt_pxxs') #these one have a newer post date than the pars in the "input" dir, so this will test that nget downloads the smallest first rather than the oldest or newest.
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 02.dat','c d 03.dat','c d 04.dat','c d 05.dat','_corrupt_pxxs_output/c d.vol01+02.par2']})
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 6)

	def test_autopar2handling_existingpxx(self):
		self.addarticles('par2-01', 'input')
		self.rmarticle_fromserver('par2-01','input','par',self.servers.servers[0])
		self.vfailUnlessExitstatus(self.nget_run('-g test -r "par2.test.*c d.vol00.01.par2"'), 2)
		self.vfailIf(self.nget_run('-G test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 02.dat','c d 03.dat','c d 04.dat','c d 05.dat','c d.vol00+01.par2']})
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 6)

	def test_autopar2handling_missingfile1(self):
		self.addarticles('par2-01', 'input')
		self.rmarticle_fromserver('par2-01','input','dat1',self.servers.servers[0])
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 02.dat','c d 03.dat','c d 04.dat','c d 05.dat','c d.par2','c d.vol00+01.par2']})
		
	def test_autopar2handling_missingfile2(self):
		self.addarticles('par2-01', 'input')
		self.rmarticle_fromserver('par2-01','input','dat2',self.servers.servers[0])
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 03.dat','c d 04.dat','c d 05.dat','c d.par2','c d.vol01+02.par2']})
	
	def test_autopar2handling_missingfile_nofilematches(self):
		self.addarticles('par2-01', 'input')
		self.rmarticle_fromserver('par2-01','input','dat1',self.servers.servers[0])
		self.rmarticle_fromserver('par2-01','input','dat2',self.servers.servers[0])
		self.vfailUnlessExitstatus(self.nget_run('-g test -r "par2.test.*\.dat" -r "par2.test.*c d.par2"'), 2)
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 03.dat','c d 04.dat','c d 05.dat','c d.par2','c d.vol00+01.par2','c d.vol01+02.par2']})
		
	def test_autopar2handling_incomplete_pessimistic(self):
		self.addarticle_toserver('par2-01', 'input', 'dat2', self.servers.servers[0])
		self.addarticle_toserver('par2-01', 'input', 'par', self.servers.servers[0])
		self.addarticle_toserver('par2-01', 'input', 'par2', self.servers.servers[0])
		self.addarticle_toserver('par2-01', 'input', 'par3', self.servers.servers[0])
		self.vfailUnlessExitstatus(self.nget_run('-g test -r par2.test'), 2)
		self.verifyoutput({'par2-01':['c d 02.dat','c d.par2','c d.vol01+02.par2','c d.vol03+04.par2']})
		
	def test_autopar2handling_incomplete_optimistic(self):
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'autopar_optimistic':1})
		self.addarticle_toserver('par2-01', 'input', 'dat2', self.servers.servers[0])
		self.addarticle_toserver('par2-01', 'input', 'par', self.servers.servers[0])
		self.addarticle_toserver('par2-01', 'input', 'par2', self.servers.servers[0])
		self.addarticle_toserver('par2-01', 'input', 'par3', self.servers.servers[0])
		self.vfailUnlessExitstatus(self.nget_run('-g test -r par2.test'), 2)
		self.verifyoutput({'par2-01':['c d 02.dat','c d.par2']})
		
	def test_autopar2handling_corruptfile(self):
		self.addarticles('par2-01', 'input')
		self.rmarticle_fromserver('par2-01','input','dat2',self.servers.servers[0])
		self.rmarticle_fromserver('par2-01','input','dat4',self.servers.servers[0])
		self.addarticles('par2-01', 'corrupt_input')
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','_corrupt_output/c d 02.dat','c d 03.dat','_corrupt_output/c d 04.dat','c d 05.dat','c d.par2','c d.vol01+02.par2','c d.vol07+08.par2']})
		
	def test_autopar2handling_corruptfile_correctdupe(self):
		self.addarticles('par2-01', 'corrupt_input')
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.addarticles('par2-01', 'input')
		self.vfailIf(self.nget_run('-dF -g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 02.dat','_corrupt_output/c d 02.dat','c d 03.dat','c d 04.dat','_corrupt_output/c d 04.dat','c d 05.dat','c d.par2']}) #### TODO requires mods to par2cmdline
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 8)
		
	def test_autopar2handling_corruptfileblock(self):
		self.addarticles('par2-01', 'input')
		self.rmarticle_fromserver('par2-01','input','dat5',self.servers.servers[0])
		self.addarticles('par2-01', 'corrupt_inputblock')
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 02.dat','c d 03.dat','c d 04.dat','_corrupt_outputblock/c d 05.dat','c d.par2','c d.vol00+01.par2','c d.vol01+02.par2']})
		
	def test_autopar2handling_corruptpxx(self):
		self.addarticles('par2-01', 'input')
		self.rmarticle_fromserver('par2-01','input','dat2',self.servers.servers[0])
		self.rmarticle_fromserver('par2-01','input','dat3',self.servers.servers[0])
		self.rmarticle_fromserver('par2-01','input','par2',self.servers.servers[0])
		self.rmarticle_fromserver('par2-01','input','par3',self.servers.servers[0])
		self.addarticles('par2-01', 'corrupt_pxxs')
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 04.dat','c d 05.dat','c d.par2','_corrupt_pxxs_output/c d.vol01+02.par2','_corrupt_pxxs_output/c d.vol03+04.par2','c d.vol07+08.par2']})
		
	def test_autopar2handling_corruptpxx_correctdupe(self):
		self.addarticle_toserver('par2-01', 'input', 'par', self.servers.servers[0])
		self.addarticle_toserver('par2-01', 'input', 'dat1', self.servers.servers[0])
		self.addarticle_toserver('par2-01', 'input', 'dat4', self.servers.servers[0])
		self.addarticle_toserver('par2-01', 'input', 'dat5', self.servers.servers[0])
		self.addarticles('par2-01', 'corrupt_pxxs')
		self.vfailUnlessExitstatus(self.nget_run('-g test -r par2.test'), 2)
		self.addarticle_toserver('par2-01', 'input', 'par1', self.servers.servers[0])
		self.addarticle_toserver('par2-01', 'input', 'par2', self.servers.servers[0])
		self.addarticle_toserver('par2-01', 'input', 'par3', self.servers.servers[0])
		self.addarticle_toserver('par2-01', 'input', 'par4', self.servers.servers[0])
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 04.dat','c d 05.dat','c d.par2','_corrupt_pxxs_output/c d.vol01+02.par2','_corrupt_pxxs_output/c d.vol03+04.par2','c d.vol01+02.par2','c d.vol03+04.par2']})

	def test_autopar2handling_corruptpxxblock(self):
		self.addarticles('par2-01', 'input')
		self.rmarticle_fromserver('par2-01','input','dat3',self.servers.servers[0])
		self.rmarticle_fromserver('par2-01','input','par3',self.servers.servers[0])
		self.addarticles('par2-01', 'corrupt_pxxblocks')
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 02.dat','c d 04.dat','c d 05.dat','c d.par2','_corrupt_pxxblocks_output/c d.vol03+04.par2','c d.vol01+02.par2']})
		
	def test_autopar2handling_corruptmainpkt(self):
		self.addarticles('par2-01', 'input',fname='dat?')
		self.addarticles('par2-01', 'input',fname='par?')
		self.addarticles('par2-01', 'corrupt_mainpkt')
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 02.dat','c d 03.dat','c d 04.dat','c d 05.dat','_corrupt_mainpkt_output/c d.par2','c d.vol00+01.par2']})
		
	def test_autopar2handling_missingfiledescpkt(self):
		self.addarticles('par2-01', 'input',fname='dat?')
		self.addarticles('par2-01', 'input',fname='par?')
		self.addarticles('par2-01', 'missing_filedescpkt')
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 02.dat','c d 03.dat','c d 04.dat','c d 05.dat','_missing_filedescpkt_output/c d.par2','c d.vol00+01.par2']})
		
	def test_autopar2handling_missingifscpkt(self):
		self.addarticles('par2-01', 'input',fname='dat?')
		self.addarticles('par2-01', 'input',fname='par?')
		self.addarticles('par2-01', 'missing_ifscpkt')
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 02.dat','c d 03.dat','c d 04.dat','c d 05.dat','_missing_ifscpkt_output/c d.par2','c d.vol00+01.par2']})
		
	def test_autopar2handling_reply(self):
		self.addarticles('par2-01', 'input')
		self.addarticles('par2-01', 'reply')
		self.vfailIf(self.nget_run('-g test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 02.dat','c d 03.dat','c d 04.dat','c d 05.dat','c d.par2','_reply_output/1058814262.0.txt']})
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 7)
		
	def test_autopar2handling_reply_existingpar(self):
		self.addarticles('par2-01', 'input')
		self.addarticles('par2-01', 'reply')
		self.vfailUnlessExitstatus(self.nget_run('-g test -r "par2.test.*c d.par2"'), 2)
		self.vfailIf(self.nget_run('-G test -r par2.test'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 02.dat','c d 03.dat','c d 04.dat','c d 05.dat','c d.par2','_reply_output/1058814262.0.txt']})
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 7)
		
	def test_autopar2handling_0file(self):
		self.addarticles('par02', 'input', fname='dat*')
		self.addarticles('par02', 'par2_input')
		self.addarticles('par02', 'par2_0file')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par02':['p2-01.dat','p2-02.dat','p2-03.dat','p2-04.dat','p2-05.dat','_par2_output/p2.par2','_par2_0file_output/1059095652.0.txt']})
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 7)
	
	def test_autopar2handling_differingcasefile(self):
		self.addarticles('par02', 'input', fname='dat[1-3]')
		self.addarticles('par02', 'par2_input')
		self.addarticles('par02', 'case_input')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par02':['p2-01.dat','p2-02.dat','p2-03.dat','_case_output/P2-04.dAt','_case_output/p2-05.DaT','_par2_output/p2.par2']}) #### TODO requires mods to par2cmdline
		
	def test_autopar2handling_differingparfilenames(self):
		self.addarticles('par02', 'input',fname='dat2')
		self.addarticles('par02', 'par2_input',fname='par')
		self.addarticles('par02', 'par2_input',fname='par2')
		self.addarticles('par02', 'c_d_par2_input')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par02':['p2-02.dat','_par2_output/p2.par2','_par2_output/p2.vol1+2.par2', '_c_d_par2_output/c d.vol03+02.par2', '_c_d_par2_output/c d.par2']})
		
	def test_autopar2handling_differingparfilenames_nopar(self):
		self.addarticles('par02', 'input',fname='dat2')
		self.addarticles('par02', 'par2_input',fname='par2')
		self.addarticles('par02', 'c_d_par2_input',fname='par3')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par02':['p2-02.dat','_par2_output/p2.vol1+2.par2', '_c_d_par2_output/c d.vol03+02.par2']})

	def test_autopar2handling_multiparset(self):
		self.addarticles('par2-01', 'input')
		self.addarticles('par02', 'input', fname='dat*')
		self.addarticles('par02', 'par2_input')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 02.dat','c d 03.dat','c d 04.dat','c d 05.dat','c d.par2'],
			'par02':['p2-01.dat','p2-02.dat','p2-03.dat','p2-04.dat','p2-05.dat','_par2_output/p2.par2']})
		
	def test_autopar2handling_multiparset_samename(self):
		self.addarticles('par2-01', 'input', fname='dat*')
		self.addarticles('par2-01', 'input', fname='par')
		self.addarticles('par2-01', 'input', fname='par[12]')
		self.addarticles('par02', 'input', fname='dat*')
		self.addarticles('par02', 'c_d_par2_input', fname='par')
		self.addarticles('par02', 'c_d_par2_input', fname='par[12]')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par2-01':['c d 01.dat','c d 02.dat','c d 03.dat','c d 04.dat','c d 05.dat','c d.par2'],
			'par02':['p2-01.dat','p2-02.dat','p2-03.dat','p2-04.dat','p2-05.dat','_c_d_par2_output/c d.par2']}) 
		
	#### TODO: further par2 tests
	
	def test_autoparhandling_multiparversions(self):
		self.addarticles('par02', 'input')
		self.addarticles('par02', 'par2_input')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par02':['p2-01.dat','p2-02.dat','p2-03.dat','p2-04.dat','p2-05.dat','p2.par','_par2_output/p2.par2']})

	def test_autoparhandling_multiparversions_missingfile(self):
		self.addarticles('par02', 'input')
		self.addarticles('par02', 'par2_input')
		self.rmarticles('par02','input', fname='dat4')
		self.vfailIf(self.nget_run('-g test -r "par.*test"'))
		self.verifyoutput({'par02':['p2-01.dat','p2-02.dat','p2-03.dat','p2-05.dat','p2.par','_par2_output/p2.par2','p2.p01','_par2_output/p2.vol0+1.par2']}) #### should it try to only grab one of the par1 or the par2?


class NoCacheRetrieveTestCase(TestCase, RetrieveTest_base):
	def setUp(self):
		RetrieveTest_base.setUp(self)
	def tearDown(self):
		if self.servers.servers[0].count("group")>0:
			assert self.servers.servers[0].count("xpat")>0
		RetrieveTest_base.tearDown(self)
	def nget_run(self, cmd):
		newcmd = re.sub('-[Gg] ', '-x ', cmd)
		return self.nget.run(newcmd)

class RetrieveTestCase(TestCase, RetrieveTest_base):
	def setUp(self):
		RetrieveTest_base.setUp(self)
	def tearDown(self):
		assert self.servers.servers[0].count("xpat")==0
		RetrieveTest_base.tearDown(self)
	def nget_run(self, cmd):
		return self.nget.run(cmd)

	def test_mid(self):
		self.vfailIf(self.nget.run('-g test -R "mid .a67ier.6l5.1.bar. =="'))
		self.verifyoutput('0002')

	def test_not_mid(self):
		self.vfailIf(self.nget.run('-g test -R "mid 1.1041808207.48.dumbnntpd !="'))
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
	
	def test_subject(self):
		self.vfailIf(self.nget.run('-g test -R "subject joystick =="'))
		self.verifyoutput('0002')

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
		self.verifyoutput('0005')

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
		self.verifyoutput(['0002','0001','0005'])
	
	def test_references(self):
		self.addarticles('refs01', 'input')
		self.addarticles('refs02', 'input')
		self.vfailIf(self.nget.run('-g test -R "references foo01 =="'))
		self.verifyoutput('refs01')
	
	def test_R_extra_whitespace(self):
		self.vfailIf(self.nget.run('-g test -R "  \tlines  \t 20 \t  > \t  lines \t  200\t  <\t  &&\t  \t"'))
		self.verifyoutput('0005')
		
	def test_R_stack(self):
		self.vfailIf(self.nget.run('-g test -R "lines 20 > lines 200 < && bytes 2000 > bytes 90000 < && ||"'))
		self.verifyoutput(['0005','0002'])
	
	def test_R_stack4(self):
		self.vfailIf(self.nget.run('-g test -R "lines 2 > lines 200 < bytes 1000 > bytes 90000 < && && &&"'))
		self.verifyoutput(['0005'])

	def test_p_mkdir(self):
		path = os.path.join(self.nget.tmpdir,'aaa','bbb','ccc')
		self.vfailIf(self.nget.run('-m yes -g test -p '+path+' -r foo'))
		self.verifyoutput('0001', tmpdir=path)

	def test_p_mkdirmaxcreate(self):
		path = os.path.join(self.nget.tmpdir,'aaa','bbb','ccc')
		self.vfailUnlessExitstatus(self.nget.run('-m no -g test -p '+path+' -r foo'), 4)
		self.vfailUnlessExitstatus(self.nget.run('-m 0 -g test -p '+path+' -r foo'), 4)
		self.vfailUnlessExitstatus(self.nget.run('-m 2 -g test -p '+path+' -r foo'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 0)
		self.vfailIf(self.nget.run('-m 3 -g test -p '+path+' -r foo'))
		self.verifyoutput('0001', tmpdir=path)

	def test_p_relative_mkdir(self):
		d = os.getcwd()
		tail = os.path.join('aaa','bbb','ccc')
		try:
			os.chdir(self.nget.tmpdir)
			self.vfailIf(self.nget.run('-m yes -g test -p '+tail+' -r foo'))
		finally:
			os.chdir(d)
		self.verifyoutput('0001', tmpdir=os.path.join(self.nget.tmpdir,tail))

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
		self.verifyoutput(['0002','0001','0005'])

	def test_list_abspath(self):
		lpath = os.path.join(self.nget.rcdir, 'list.foo')
		f = open(lpath, 'w')
		f.write('-g test -r .')
		f.close()
		self.vfailIf(self.nget.run('-@ %s'%lpath))
		self.verifyoutput(['0002','0001','0005'])

	def test_list_multiline(self):
		lpath = os.path.join(self.nget.rcdir, 'list.foo')
		f = open(lpath, 'w')
		f.write('-g\ntest\n-r\n.')
		f.close()
		self.vfailIf(self.nget.run('-@ %s'%lpath))
		self.verifyoutput(['0002','0001','0005'])

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
		self.vfailUnlessExitstatus(self.nget.run('-@ foobar'), 4)
	
	def test_badskip_path(self):
		self.vfailUnlessExitstatus(self.nget.run('-p badpath -g test -r joy -p %s -r foo'%(self.nget.tmpdir)), 4)
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 1)

	def test_badskip_temppath(self):
		self.vfailUnlessExitstatus(self.nget.run('-P badpath -g test -r joy -P %s -r foo'%(self.nget.tmpdir)), 4)
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 1)

	def test_badskip_temppath_okpath(self):
		self.vfailUnlessExitstatus(self.nget.run('-P badpath -g test -r joy -p %s -r foo'%(self.nget.tmpdir)), 4) #-p resets -P too
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 1)

	def test_badskip_path_oktemppath(self):
		self.vfailUnlessExitstatus(self.nget.run('-p badpath -g test -r joy -P %s -r foo'%(self.nget.tmpdir)), 4) #-P does not reset -p
		self.verifyoutput([])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 0)

	def test_badskip_host(self):
		self.vfailUnlessExitstatus(self.nget.run('-h badhost -g test -r joy -h host0 -r foo'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 0)
		self.verifyoutput([])#bad -h should turn -g into -G, so that should have failed to get anything.
		self.vfailIf(self.nget.run('-g test')) #retrieve headers first and try again
		self.vfailUnlessExitstatus(self.nget.run('-h badhost -g test -r joy -h host0 -r foo'), 4)
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 1)

	def test_badskip_pathhost(self):
		self.vfailUnlessExitstatus(self.nget.run('-g test -h badhost -p badpath -r . -p %s -r . -h host0 -r foo'%(self.nget.tmpdir)), 4)
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 1)

	def test_badskip_hostpath(self):
		self.vfailUnlessExitstatus(self.nget.run('-g test -h badhost -p badpath -r . -h host0 -r . -p %s -r foo'%(self.nget.tmpdir)), 4)
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 1)

	def test_badskip_okthenbadpath(self):
		self.vfailUnlessExitstatus(self.nget.run('-g test -r foo -p badpath -r .'), 4)
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 1)

	def test_badskip_okthenbadhost(self):
		self.vfailUnlessExitstatus(self.nget.run('-g test -r foo -h badhost -r .'), 4)
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 1)

	def test_badskip_emptyexpression(self):
		self.vfailUnlessExitstatus(self.nget.run('-g test -R "" -r joy'), 4)
		self.verifyoutput(['0002'])
	
	def test_badskip_retrievebadregex(self):
		self.vfailUnlessExitstatus(self.nget.run('-g test -r "*" -r joy'), 4)
		self.verifyoutput(['0002'])
	
	def test_badskip_retrievenogroup(self):
		self.vfailUnlessExitstatus(self.nget.run('-r . -g test -r joy'), 4)
		self.verifyoutput(['0002'])
	
	def test_cache_reloading_after_host(self):
		self.vfailIf(self.nget.run('-g test -r foo -h host0 -D -r foo'))
		self.verifyoutput(['0001'])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 2)

	def test_flush_nogroup(self):
		self.vfailUnlessExitstatus(self.nget.run('-F host0'), 4)
	
	def test_flush_badserver(self):
		self.vfailUnlessExitstatus(self.nget.run('-g test -F badserv'), 4)

	def test_flush_available_badserver(self):
		self.vfailUnlessExitstatus(self.nget.run('-A -F badserv'), 4)

	def test_bad_arg(self):
		self.vfailUnlessExitstatus(self.nget.run('-g test badarg -r .'), 4)
		self.verifyoutput([])

	def test_bad_argnotcomment(self):
		self.vfailUnlessExitstatus(self.nget.run('-g test "#badarg" -r .'), 4) #comments should not work on command line, only in listfile
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
		self.vfailUnlessExitstatus(self.nget.run('-@ %s'%lpath), 4)
		self.verifyoutput([])
	
	def test_available(self):
		self.vfailIf(self.nget.run('-a'))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 1)
		self.vfailIf(self.nget.run('-a'))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 2)
		apath = os.path.join(self.nget.rcdir, 'avail.out')
		self.vfailIf(self.nget.run('-A -T -r . > %s'%apath))
		print open(apath).read()
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 2)
		self.vfailUnlessEqual(open(apath).readlines()[-1].strip(), "h0\ttest")
	
	def test_available2(self):
		apath = os.path.join(self.nget.rcdir, 'avail.out')
		self.vfailIf(self.nget.run('-a -T -r . > %s'%apath))
		print open(apath).read()
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 1)
		self.failUnless(re.search("^h0\ttest$",open(apath).read(), re.M))
		
	def test_available_newgroups(self):
		self.vfailIf(self.nget.run('-a'))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 1)
		self.vfailUnlessEqual(self.servers.servers[0].count("list_newsgroups"), 1)
		self.vfailUnlessEqual(self.servers.servers[0].count("list_"), 1)
		time.sleep(1.1)
		self.servers.servers[0].addgroup("a.new.group","whee")
		apath = os.path.join(self.nget.rcdir, 'avail.out')
		self.vfailIf(self.nget.run('-a -T -r . > %s'%apath))
		print open(apath).read()
		self.failUnless(re.search(r"^h0\ta.new.group\twhee \[h0\][\n\r]+h0\ttest$",open(apath).read(), re.M))
		self.vfailUnlessEqual(self.servers.servers[0].count("list_newsgroups"), 2)
		self.vfailUnlessEqual(self.servers.servers[0].count("newgroups"), 1)
		self.vfailUnlessEqual(self.servers.servers[0].count("list_"), 1)

	def test_available_r(self):
		self.servers.servers[0].addgroup("aa.group")
		self.servers.servers[0].addgroup("bb.group")
		self.servers.servers[0].addgroup("group.one", "aa.desc")
		self.servers.servers[0].addgroup("group.two", "bb.desc")
		apath = os.path.join(self.nget.rcdir, 'avail.out')
		self.vfailIf(self.nget.run('-a -T -r aa > %s'%apath))
		output = open(apath).read()
		print output
		self.failUnless(output.find("aa.group")>=0)
		self.failUnless(output.find("aa.desc")>=0)
		self.failIf(output.find("bb.")>=0)

	def test_xavailable_r(self):
		self.servers.servers[0].addgroup("aa.group")
		self.servers.servers[0].addgroup("bb.group")
		self.servers.servers[0].addgroup("group.one", "aa.desc")
		self.servers.servers[0].addgroup("group.two", "bb.desc")
		self.servers.servers[0].addgroup("group.aa.two", "foo.desc")
		apath = os.path.join(self.nget.rcdir, 'avail.out')
		self.vfailIf(self.nget.run('-X -T -r aa > %s'%apath))
		output = open(apath).read()
		print output
		self.failUnless(output.find("aa.group")>=0)
		self.failUnless(output.find("group.aa.two")>=0)
		self.failUnless(output.find("foo.desc")>=0)
		self.failIf(output.find("bb.")>=0)
		self.failIf(output.find("aa.desc")>=0) #LIST NEWSGROUPS wildmat doesn't search the description, so this shouldn't be found.

	def test_available_R_desc(self):
		self.servers.servers[0].addgroup("group.bbb")
		self.servers.servers[0].addgroup("group.one", "aaa")
		self.servers.servers[0].addgroup("group.two", "bbb")
		apath = os.path.join(self.nget.rcdir, 'avail.out')
		self.vfailIf(self.nget.run('-a -T -R "desc bbb ==" > %s'%apath))
		output = open(apath).read()
		print output
		self.failUnless(re.search(r"^h0\tgroup.two\tbbb \[h0\]$",output, re.M))
		self.failIf(output.find(".one")>=0)
		self.failIf(output.find("group.bbb")>=0)

	def test_available_R_not_desc(self):
		self.servers.servers[0].addgroup("group.bbb")
		self.servers.servers[0].addgroup("group.one", "aaa")
		self.servers.servers[0].addgroup("group.two", "bbb")
		apath = os.path.join(self.nget.rcdir, 'avail.out')
		self.vfailIf(self.nget.run('-a -T -R "desc bbb !=" > %s'%apath))
		output = open(apath).read()
		print output
		self.failUnless(re.search(r"^h0\tgroup.bbb[\r\n]+h0\tgroup.one\taaa \[h0\][\r\n]+h0\ttest$",output, re.M))
		self.failIf(output.find(".two")>=0)
	
	def test_lite_tempfileexists(self):
		litelist = os.path.join(self.nget.rcdir, 'lite.lst')
		self.vfailIf(self.nget.run("-w %s -g test -r joy"%litelist))
		self.vfailIf(self.nget.run("-G test -K -r joy"))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 2)
		self.vfailIf(self.nget.runlite(litelist))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 2)
		self.vfailIf(self.nget.run("-N -G test -r ."))
		self.verifyoutput('0002')


class MetaGrouping_RetrieveTest_base(DecodeTest_base):
	def setUp(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		
	def tearDown(self):
		self.servers.stop()
		self.nget.clean_all()

	def test_r(self):
		self.addarticles('0002', 'uuencode_multi3', groups=["test"])
		self.addarticles('0001', 'uuencode_single', groups=["test2"])
		self.addarticles('0004', 'input', groups=["test3"])
		self.addarticles('refs01', 'input', groups=["test"])
		self.addarticles('refs02', 'input', groups=["test2"])
		self.vfailIf(self.nget_run('-g test,test2,test3 -ir .'))
		self.verifyoutput(['0002','0001','0004','refs01','refs02'])

	def test_r_dupemerge(self):
		self.addarticles('0002', 'uuencode_multi3', groups=["test","test2"])
		self.addarticles('0001', 'uuencode_single', groups=["test2","test3"])
		self.addarticles('0004', 'input', groups=["test3","test"])
		self.addarticles('refs01', 'input', groups=["test","test2"])
		self.addarticles('refs02', 'input', groups=["test2","test3"])
		self.vfailIf(self.nget_run('-D -g test,test2,test3 -ir .'))
		self.verifyoutput(['0002','0001','0004','refs01','refs02'])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 7)

	def test_r_partmerge(self):
		self.addarticle_toserver('0001', 'yenc_multi', '001', self.servers.servers[0], groups=["test2"])
		self.addarticle_toserver('0001', 'yenc_multi', '002', self.servers.servers[0], groups=["test3"])
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0], groups=["test"])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0], groups=["test2"])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0], groups=["test3"])
		self.addarticles('0004', 'input', groups=["test3"])
		self.addarticles('refs01', 'input', groups=["test"])
		self.addarticles('refs02', 'input', groups=["test2"])
		self.vfailIf(self.nget_run('-g test,test2,test3 -i -r joy -r testfile'))
		self.verifyoutput(['0002','0001'])
	
	def test_r_dupepartmerge(self):
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0], groups=["test","test2"])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0], groups=["test2","test3"])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0], groups=["test3","test"])
		self.addarticles('0001', 'uuencode_single', groups=["test2","test3"])
		self.addarticles('0004', 'input', groups=["test3","test"])
		self.addarticles('refs01', 'input', groups=["test","test2"])
		self.addarticles('refs02', 'input', groups=["test2","test3"])
		self.vfailIf(self.nget_run('-D -g test,test2,test3 -ir joy'))
		self.verifyoutput(['0002'])
	
	def test_r_samerge(self): #note this func is copied in MetaGrouping_LiteRetrieveTestCase
		self.addarticle_toserver('mergesa01', 'input', '001', self.servers.servers[0], groups=["test"])
		self.addarticle_toserver('mergesa01', 'input', '002', self.servers.servers[0], groups=["test2"])
		self.vfailIf(self.nget_run('-g test,test2 -ir .'))
		self.verifyoutput(['mergesa01'])

	
class MetaGrouping_NoCacheRetrieveTestCase(TestCase, MetaGrouping_RetrieveTest_base):
	def setUp(self):
		MetaGrouping_RetrieveTest_base.setUp(self)
	def tearDown(self):
		if self.servers.servers[0].count("group")>0:
			assert self.servers.servers[0].count("xpat")>0
		MetaGrouping_RetrieveTest_base.tearDown(self)
	def nget_run(self, cmd):
		newcmd = re.sub('-[Gg] ', '-x ', cmd)
		return self.nget.run(newcmd)

class MetaGrouping_CacheRetrieveTestCase(TestCase, MetaGrouping_RetrieveTest_base):
	def setUp(self):
		MetaGrouping_RetrieveTest_base.setUp(self)
	def tearDown(self):
		assert self.servers.servers[0].count("xpat")==0
		MetaGrouping_RetrieveTest_base.tearDown(self)
	def nget_run(self, cmd):
		x = re.match("^(.*)-g *([^ ]+) *(.*)$", cmd)
		if x:
			groups = x.group(2).split(',')
			cmd = x.group(1) + '-g ' + ' -g '.join(groups) + ' -G ' + x.group(2) + ' ' + x.group(3)
		return self.nget.run(cmd)
	
	def test_r_allcached(self):
		self.addarticles('0002', 'uuencode_multi3', groups=["test"])
		self.addarticles('0001', 'uuencode_single', groups=["test2"])
		self.addarticles('0004', 'input', groups=["test3"])
		self.addarticles('refs01', 'input', groups=["test"])
		self.addarticles('refs02', 'input', groups=["test2"])
		self.vfailIf(self.nget.run('-g test,test2,test3'))
		self.vfailIf(self.nget.run('-G "*" -ir .'))
		self.verifyoutput(['0002','0001','0004','refs01','refs02'])

class MetaGrouping_RetrieveTestCase(TestCase, MetaGrouping_RetrieveTest_base):
	def setUp(self):
		MetaGrouping_RetrieveTest_base.setUp(self)
	def tearDown(self):
		assert self.servers.servers[0].count("xpat")==0
		MetaGrouping_RetrieveTest_base.tearDown(self)
	def nget_run(self, cmd):
		return self.nget.run(cmd)

class MetaGrouping_LiteRetrieveTestCase(TestCase, MetaGrouping_RetrieveTest_base):
	def setUp(self):
		MetaGrouping_RetrieveTest_base.setUp(self)
	def tearDown(self):
		assert self.servers.servers[0].count("xpat")==0
		MetaGrouping_RetrieveTest_base.tearDown(self)
	def nget_run(self, cmd):
		litelist = os.path.join(self.nget.rcdir, 'lite.lst')
		self.vfailIf(self.nget.run('-w %s %s'%(litelist,cmd)))
		self.vfailIf(self.nget.runlite(litelist))
		return self.nget.run('-N %s'%cmd)

	def test_r_samerge(self): #note this func is copied from MetaGrouping_RetrieveTest_base
		self.addarticle_toserver('mergesa01', 'input', '001', self.servers.servers[0], groups=["test"])
		self.addarticle_toserver('mergesa01', 'input', '002', self.servers.servers[0], groups=["test2"])
		self.vfailIf(self.nget_run('-g test,test2 -r .'))
		#Kind of ugly, but we have to run ngetlite again to get the second article, since they both have the same tempname.  Dunno what should/could really be done about that.
		self.vfailIf(self.nget_run('-G test,test2 -r .'))
		self.verifyoutput(['mergesa01'])


class FatalUserErrorsTestCase(TestCase, DecodeTest_base):
	def setUp(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'fatal_user_errors':1}) 
		self.addarticles('0001', 'uuencode_single')
		self.servers.start()
	
	def tearDown(self):
		self.servers.stop()
		self.nget.clean_all()

	def test_badpath(self):
		self.vfailUnlessExitstatus(self.nget.run('-p badpath -g test'), 4)
		self.vfailUnlessExitstatus(self.nget.run('-P badpath -g test'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
	def test_badhost(self):
		self.vfailUnlessExitstatus(self.nget.run('-h badhost -g test'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
	def test_retrieve_badregex(self):
		self.vfailUnlessExitstatus(self.nget.run('-Gtest -r "*" -g test'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
	def test_retrieve_nogroup(self):
		self.vfailUnlessExitstatus(self.nget.run('-r . -g test'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
	def test_expretrieve_badexp(self):
		self.vfailUnlessExitstatus(self.nget.run('-Gtest -R foo -g test'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
	def test_expretrieve_badexp2(self):
		self.vfailUnlessExitstatus(self.nget.run('-Gtest -R "lines 0" -g test'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
	def test_expretrieve_badexp_join_emptystack(self):
		self.vfailUnlessExitstatus(self.nget.run('-Gtest -R "||" -g test'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
	def test_expretrieve_badexp_join_singleitem(self):
		self.vfailUnlessExitstatus(self.nget.run('-Gtest -R "lines 0 >= ||" -g test'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
	def test_expretrieve_badexp_nojoin(self):
		self.vfailUnlessExitstatus(self.nget.run('-Gtest -R "lines 0 >= lines 10 <" -g test'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
	def test_expretrieve_badregex(self):
		self.vfailUnlessExitstatus(self.nget.run('-Gtest -R "subject * ==" -g test'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
	def test_flush_nogroup(self):
		self.vfailUnlessExitstatus(self.nget.run('-F host0 -g test'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
	def test_flush_badserver(self):
		self.vfailUnlessExitstatus(self.nget.run('-g test -F badserv -r .'), 4)
		self.verifyoutput([])
	def test_flush_available_badserver(self):
		self.vfailUnlessExitstatus(self.nget.run('-A -F badserv -g test'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
	def test_bad_arg(self):
		self.vfailUnlessExitstatus(self.nget.run('badarg -g test'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
	def test_list_nonexistant(self):
		self.vfailUnlessExitstatus(self.nget.run('-@ foo -g test'), 4)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)



class XoverTestCase(TestCase, DecodeTest_base):
	def setUp(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'fullxover':0}) 
		self.fxnget = util.TestNGet(ngetexe, self.servers.servers, options={'fullxover':1}) 
		self.fx2nget = util.TestNGet(ngetexe, self.servers.servers, options={'fullxover':2}) 
		self.servers.start()
		
	def tearDown(self):
		self.servers.stop()
		self.nget.clean_all()
		self.fxnget.clean_all()
		self.fx2nget.clean_all()

	def verifynonewfiles(self):
		nf = os.listdir(self.nget.tmpdir)
		fxnf = os.listdir(self.fxnget.tmpdir)
		fx2nf = os.listdir(self.fx2nget.tmpdir)
		self.failIf(nf or fxnf or fx2nf, "new files: %s %s %s"%(nf, fxnf, fx2nf))

	def run_all(self, args):
		self.vfailIf(self.nget.run(args))
		self.vfailIf(self.fxnget.run(args))
		self.vfailIf(self.fx2nget.run(args))
		
	def verifyoutput_all(self, testnum):
		self.verifyoutput(testnum)
		self.verifyoutput(testnum, tmpdir=self.fxnget.tmpdir)
		self.verifyoutput(testnum, tmpdir=self.fx2nget.tmpdir)
		
	def test_newarticle(self):
		self.addarticle_toserver('0002', 'uuencode_multi', '001', self.servers.servers[0])

		self.run_all("-g test -r .")
		self.verifynonewfiles()
		
		self.addarticle_toserver('0002', 'uuencode_multi', '002', self.servers.servers[0])

		self.run_all("-g test -r .")
		self.verifyoutput_all('0002')

	def test_newarticle_reverse(self):
		self.addarticle_toserver('0002', 'uuencode_multi', '002', self.servers.servers[0])

		self.run_all("-g test -r .")
		self.verifynonewfiles()
		
		self.addarticle_toserver('0002', 'uuencode_multi', '001', self.servers.servers[0])

		self.run_all("-g test -r .")
		self.verifyoutput_all('0002')

	def test_oldarticle(self):
		self.addarticle_toserver('0002', 'uuencode_multi', '001', self.servers.servers[0], anum=2)

		self.run_all("-g test -r .")
		self.verifynonewfiles()
		
		self.addarticle_toserver('0002', 'uuencode_multi', '002', self.servers.servers[0], anum=1)

		self.run_all("-g test -r .")

		self.verifyoutput_all('0002')

	def test_insertedarticle(self):
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0], anum=1)
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0], anum=3)

		self.run_all("-g test -r .")
		self.verifynonewfiles()
		
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0], anum=2)

		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifynonewfiles() #fullxover=0 should not be able to find new article
		self.vfailIf(self.fxnget.run("-g test -r ."))
		self.vfailIf(self.fx2nget.run("-g test -r ."))

		self.verifyoutput('0002', tmpdir=self.fxnget.tmpdir)
		self.verifyoutput('0002', tmpdir=self.fx2nget.tmpdir)
	
	def test_fullxover2_listgroup_appropriateness(self):
		self.addarticles('0001', 'uuencode_single')
		self.vfailIf(self.fx2nget.run("-g test"))
		self.vfailUnlessEqual(self.servers.servers[0].count("listgroup"), 0) #shouldn't do listgroup on first update of group
		self.vfailIf(self.fx2nget.run("-g test"))
		self.vfailUnlessEqual(self.servers.servers[0].count("listgroup"), 1) #should do listgroup on second update of group
		self.addarticles('0002', 'uuencode_multi')
		self.rmarticles('0001', 'uuencode_single')
		self.vfailIf(self.fx2nget.run("-g test -r ."))
		self.vfailUnlessEqual(self.servers.servers[0].count("listgroup"), 1) #shouldn't do listgroup if all cached anums are lower than all available ones
		self.verifyoutput('0002', tmpdir=self.fx2nget.tmpdir)
		self.vfailIf(self.fx2nget.run("-g test"))
		self.vfailUnlessEqual(self.servers.servers[0].count("listgroup"), 2)
	
	def test_removedarticle(self):
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0], anum=1)
		article = self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0], anum=2)
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0], anum=3)

		self.run_all("-g test")
		self.verifynonewfiles()
		
		self.servers.servers[0].rmarticle(article.mid)

		self.vfailIf(self.fx2nget.run("-g test -r ."))
		self.verifynonewfiles()#fullxover=2 should notice the missing article and thus not try to get the now incomplete file
		self.vfailUnlessExitstatus(self.nget.run("-g test -r ."), 8)
		self.vfailUnlessExitstatus(self.fxnget.run("-g test -r ."), 8)
	
	def test_largearticlenumbers(self):
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0], anum=1)
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0], anum=2147483647)
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0], anum=4294967295L)

		self.run_all("-g test -r .")

		self.verifyoutput_all('0002')

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

class OverArticleQuotaShutdowningNNTPRequestHandler(nntpd.NNTPRequestHandler):
	def cmd_article(self, args):
		self.nwrite("502 Over quota. Shutdown.")
		#If how is 0, further receives are disallowed. If how is 1, further sends are disallowed. If how is 2, further sends and receives are disallowed. 
		self.connection.shutdown(1)
		while 1:
			rcmd = self.rfile.readline()
			if not rcmd: raise nntpd.NNTPDisconnect

class OverXOverQuotaDiscoingNNTPRequestHandler(nntpd.NNTPRequestHandler):
	def cmd_xover(self, args):
		raise nntpd.NNTPDisconnect("502 Over quota.  Goodbye.")

class OverArticleQuotaNNTPRequestHandler(nntpd.NNTPRequestHandler):
	def cmd_article(self, args):
		raise nntpd.NNTPError(502, "Over quota.")

class OverXOverQuotaNNTPRequestHandler(nntpd.NNTPRequestHandler):
	def cmd_xover(self, args):
		raise nntpd.NNTPError(502, "Over quota.")

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

class _XOverFinisher(Exception): pass
class XOverStreamingDropsDataNNTPRequestHandler(nntpd.NNTPRequestHandler):
	def cmd_xover(self, args):
		time.sleep(0.2)#ensure the client has time for its streamed commands to get here.
		import select
		rf,wf,ef=select.select([self.rfile], [], [], 0)
		if rf: #if client is streaming, do the screw up stuff.
			self.discocountdown = 2 #allow a single xover result to pass before disconnecting
		elif hasattr(self,'discocountdown'):
			del self.discocountdown
		try:
			nntpd.NNTPRequestHandler.cmd_xover(self, args)
		except _XOverFinisher:
			pass
	def nwrite(self, s):
		if hasattr(self, 'discocountdown'):
			if self.discocountdown==0:
				raise _XOverFinisher
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
		self.vfailUnlessExitstatus(self.nget.run("-g test -r ."), 16, "nget process did not detect connection error")
	
	def test_DeadServerRetr(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.vfailIf(self.nget.run("-g test"))
		self.servers.stop()
		del self.servers
		self.vfailUnlessExitstatus(self.nget.run("-G test -r ."), 8, "nget process did not detect connection error")
	
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
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 3)
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
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 3)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 3)

	def test_OverXOverQuotaDiscoingServerPenalization(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), OverXOverQuotaDiscoingNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'tries':1, 'penaltystrikes':2, 'maxconnections':1})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		self.vfailIf(self.nget.run("-g test -g test -g test -g test -g test"))
		self.vfailUnlessEqual(self.servers.servers[0].count("group"), 2)
		self.vfailUnlessEqual(self.servers.servers[1].count("group"), 5)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 2)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 2)

	def test_OverXOverQuotaServerPenalization(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), OverXOverQuotaNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'tries':1, 'penaltystrikes':2})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		self.vfailIf(self.nget.run("-g test -g test -g test -g test -g test"))
		self.vfailUnlessEqual(self.servers.servers[0].count("group"), 2)
		self.vfailUnlessEqual(self.servers.servers[1].count("group"), 5)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 1)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 1)

	def test_OverArticleQuotaDiscoingServerPenalization(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), OverArticleQuotaDiscoingNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, priorities=[3,1], options={'tries':1, 'penaltystrikes':2, 'maxconnections':1})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		self.vfailIf(self.nget.run("-g test"))
		self.vfailIf(self.nget.run("-G test -r ."))
		self.verifyoutput('0002')
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 2)
		self.vfailUnlessEqual(self.servers.servers[1].count("article"), 3)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 3)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 3)

	def test_OverArticleQuotaShutdowningServerPenalization(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), OverArticleQuotaShutdowningNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, priorities=[3,1], options={'tries':1, 'penaltystrikes':2, 'maxconnections':1})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		self.vfailIf(self.nget.run("-g test"))
		self.vfailIf(self.nget.run("-G test -r ."))
		self.verifyoutput('0002')
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 2)
		self.vfailUnlessEqual(self.servers.servers[1].count("article"), 3)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 3)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 3)

	def test_OverArticleQuotaServerPenalization(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), OverArticleQuotaNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, priorities=[3,1], options={'tries':1, 'penaltystrikes':2})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifyoutput('0002')
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 2)
		self.vfailUnlessEqual(self.servers.servers[1].count("article"), 3)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 1)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 1)

	def test_lite_OverArticleQuotaDiscoingServerHandling(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), OverArticleQuotaDiscoingNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, priorities=[3,1], options={'tries':1, 'penaltystrikes':2, 'maxconnections':1})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		litelist = os.path.join(self.nget.rcdir, 'lite.lst')
		self.vfailIf(self.nget.run("-w %s -g test -r ."%litelist))
		self.vfailIf(self.nget.runlite(litelist))
		self.vfailIf(self.nget.run("-N -G test -r ."))
		self.verifyoutput('0002')
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 3)
		self.vfailUnlessEqual(self.servers.servers[1].count("article"), 3)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 4)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 4)

	def test_lite_OverArticleQuotaDiscoingSingleServerHandling(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), OverArticleQuotaDiscoingNNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'tries':1, 'penaltystrikes':2})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		litelist = os.path.join(self.nget.rcdir, 'lite.lst')
		self.vfailIf(self.nget.run("-w %s -g test -r ."%litelist))
		self.vfailIf(self.nget.runlite(litelist))
		self.vfailIf(self.nget.run("-N -G test -r ."))
		self.verifyoutput([])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 3)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 4)

	def test_lite_OverArticleQuotaShutdowningSingleServerHandling(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), OverArticleQuotaShutdowningNNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'tries':1, 'penaltystrikes':2})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		litelist = os.path.join(self.nget.rcdir, 'lite.lst')
		self.vfailIf(self.nget.run("-w %s -g test -r ."%litelist))
		self.vfailIf(self.nget.runlite(litelist))
		self.vfailIf(self.nget.run("-N -G test -r ."))
		self.verifyoutput([])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 3)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 4)

	def test_lite_OverArticleQuotaServerHandling(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), OverArticleQuotaNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, priorities=[3,1], options={'tries':1, 'penaltystrikes':2})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		litelist = os.path.join(self.nget.rcdir, 'lite.lst')
		self.vfailIf(self.nget.run("-w %s -g test -r ."%litelist))
		self.vfailIf(self.nget.runlite(litelist))
		self.vfailIf(self.nget.run("-N -G test -r ."))
		self.verifyoutput('0002')
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 3)
		self.vfailUnlessEqual(self.servers.servers[1].count("article"), 3)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 4)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 4)

	def test_lite_OverArticleQuotaSingleServerHandling(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), OverArticleQuotaNNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'tries':1, 'penaltystrikes':2})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		litelist = os.path.join(self.nget.rcdir, 'lite.lst')
		self.vfailIf(self.nget.run("-w %s -g test -r ."%litelist))
		self.vfailIf(self.nget.runlite(litelist))
		self.vfailIf(self.nget.run("-N -G test -r ."))
		self.verifyoutput([])
		self.vfailUnlessEqual(self.servers.servers[0].count("article"), 3)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 2)

	def test_NoPenalty_g(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'maxconnections':1, 'penaltystrikes':1})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		self.vfailIf(self.nget.run("-g test -g test -g test"))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 2)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 2)

	def test_NoPenalty_g_NoGroup(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'maxconnections':1, 'penaltystrikes':1})
		self.servers.start()
		self.vfailUnlessExitstatus(self.nget.run("-g test -g test -g test"), 16)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 2)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 2)

	def test_NoPenalty_r(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'maxconnections':1, 'penaltystrikes':1})
		self.servers.start()
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[1])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0])
		self.vfailIf(self.nget.run("-g test"))
		self.vfailIf(self.nget.run("-G test -r ."))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 3)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 2)
		self.verifyoutput('0002')

	def test_NoPenalty_r_Expired(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'maxconnections':1, 'penaltystrikes':1})
		self.servers.start()
		self.addarticles('0001', 'uuencode_single')
		self.addarticles('0002', 'uuencode_multi3')
		self.addarticles('0004', 'input')
		self.vfailIf(self.nget.run("-g test"))
		self.rmarticles('0001', 'uuencode_single')
		self.rmarticles('0002', 'uuencode_multi3')
		self.rmarticles('0004', 'input')
		self.vfailUnlessExitstatus(self.nget.run("-G test -r ."), 8)
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 3)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 3)

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
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 2)
		
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
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 1)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 3)

	def test_TwoXOverDiscoServers(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), XOver1LineDiscoingNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), XOver1LineDiscoingNNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'tries':3})
		self.servers.start()

		self.addarticles('0001', 'yenc_multi')
		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifyoutput('0001')
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 3)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 3)

	def test_ForceXOverDiscoServer(self):
		"Test if errors are handled correctly in header retrieval with force_host"
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), XOver1LineDiscoingNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), XOver1LineDiscoingNNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'tries':3})
		self.servers.start()

		self.addarticles('0001', 'yenc_multi')
		self.vfailIf(self.nget.run("-h host1 -g test"))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 3)
		self.vfailIf(self.nget.run("-G test -r ."))
		self.verifyoutput('0001')

	def test_ForceWrongServer(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.addarticles_toserver('0002', 'uuencode_multi', self.servers.servers[0])
		self.vfailIf(self.nget.run("-g test"))
		self.vfailUnlessExitstatus(self.nget.run("-h host1 -G test -r ."), 8, "nget process did not detect retrieve error")
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 1)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 1)

	def test_ForceServer(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.addarticles('0001', 'yenc_multi')
		self.vfailIf(self.nget.run("-h host1 -g test"))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 1)
		self.vfailIf(self.nget.run("-h host0 -g test"))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 1)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 1)
		self.vfailIf(self.nget.run("-h host1 -G test -r ."))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 1)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 2)
		self.verifyoutput('0001')

	def test_Available_ForceServer(self):
		self.servers = nntpd.NNTPD_Master(3)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.vfailIf(self.nget.run("-h host1 -a"))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 0)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 1)
		self.vfailUnlessEqual(self.servers.servers[2].count("_conns"), 0)

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
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 1)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 2)
	
	def test_Available_FlushServer(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.servers.servers[0].addgroup("test", "aaa")
		self.servers.servers[1].addgroup("test", "bbb")
		self.servers.servers[0].addgroup("foo", "ccc")
		self.servers.servers[1].addgroup("foo", "ccc")
		self.servers.servers[0].addgroup("0only")
		self.servers.servers[1].addgroup("1only")
		self.vfailIf(self.nget.run("-a"))
		self.vfailIf(self.nget.run("-A -Fhost0"))
		apath = os.path.join(self.nget.rcdir, 'avail.out')
		self.vfailIf(self.nget.run('-A -T -r . > %s'%apath))
		output = open(apath).read()
		print output
		self.failUnless(re.search(r"^h1\ttest\tbbb \[h1\]$",output, re.M))
		self.failUnless(re.search(r"^h1\tfoo\tccc \[h1\]$",output, re.M))
		self.failUnless(re.search(r"^h1\t1only$",output, re.M))
		self.failIf(output.find("h0")>=0)
		self.failIf(output.find("0only")>=0)
	
	def test_MetaGrouping_FlushServer(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.addarticle_toserver('0001', 'yenc_multi', '001', self.servers.servers[0], groups=["test2"])
		self.addarticle_toserver('0001', 'yenc_multi', '002', self.servers.servers[0], groups=["test3"])
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[1], groups=["test"])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[1], groups=["test2"])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[1], groups=["test3"])
		self.addarticle_toserver('0004', 'input', '001', self.servers.servers[1], groups=["test2"])
		self.vfailIf(self.nget.run("-g test,test2,test3"))
		self.vfailIf(self.nget.run("-G test,test2,test3 -F host1"))
		self.vfailIf(self.nget.run("-G test,test2,test3 -r ."))
		self.verifyoutput('0001')
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 2)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 1)
	
	def test_MetaGrouping_ismultiserver_no(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.addarticles_toserver('0002', 'uuencode_multi3', self.servers.servers[0], groups=["test"])
		self.addarticles_toserver('0001', 'uuencode_single', self.servers.servers[0], groups=["test2"])
		tpath = os.path.join(self.nget.rcdir, 'test.out')
		self.vfailIf(self.nget.run('-g test,test2 -q -r . > %s'%tpath))
		print open(tpath).read()
		self.failUnless(open(tpath).read().find("h0 1 (-1/0): ")<0)
		self.failUnless(open(tpath).read().find("h0 1 (1/3): ")<0)
	
	def test_MetaGrouping_ismultiserver_yes(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers) 
		self.servers.start()
		self.addarticles_toserver('0002', 'uuencode_multi3', self.servers.servers[0], groups=["test"])
		self.addarticles_toserver('0001', 'uuencode_single', self.servers.servers[1], groups=["test2"])
		tpath = os.path.join(self.nget.rcdir, 'test.out')
		self.vfailIf(self.nget.run('-g test,test2 -q -r . > %s'%tpath))
		print open(tpath).read()
		self.failUnless(open(tpath).read().find("h1 1 (-1/0): ")>=0)
		self.failUnless(open(tpath).read().find("h0 1 (1/3): ")>=0)
	
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
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 1)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 1)
		self.verifyoutput('0002')
	
	def test_XOverStreaming_BuggyServerWhichDropsTheEndsOfReplies(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), XOverStreamingDropsDataNNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'fullxover':1, 'timeout':3, 'tries':2})
		self.servers.start()
		#set up article list with holes in it so next run will use xover streaming
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0], anum=1)
		self.addarticle_toserver('0001', 'yenc_multi', '001', self.servers.servers[0], anum=5)
		self.addarticle_toserver('0003', 'newspost_uue_0', '002', self.servers.servers[0], anum=8)
		self.vfailIf(self.nget.run("-g test"))
		#now fill in holes with articles and try again
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0], anum=2)
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0], anum=3)
		self.addarticle_toserver('0001', 'yenc_multi', '002', self.servers.servers[0], anum=6)
		self.addarticle_toserver('0003', 'newspost_uue_0', '001', self.servers.servers[0], anum=4)
		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifyoutput(['0001','0002','0003'])

	def test_timeout(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), DelayAfterArticle2NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'timeout':1})
		self.servers.start()
		self.addarticles('0002','uuencode_multi3')
		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifyoutput('0002')
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 3)

	def test_idletimeout(self):
		self.servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer(("127.0.0.1",0), DelayBeforeArticleNNTPRequestHandler), nntpd.NNTPTCPServer(("127.0.0.1",0), nntpd.NNTPRequestHandler)])
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'idletimeout':1})
		self.servers.start()
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[1])
		self.vfailIf(self.nget.run("-g test -r ."))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 1)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 2)
		self.verifyoutput('0002')

	def test_maxconnections(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'maxconnections':1})
		self.servers.start()
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[0])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[1])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0])
		self.vfailIf(self.nget.run("-g test -r ."))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 3)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 2)
		self.verifyoutput('0002')

	def test_maxconnections_2(self):
		self.servers = nntpd.NNTPD_Master(3)
		self.nget = util.TestNGet(ngetexe, self.servers.servers, options={'maxconnections':2})
		self.servers.start()
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[2])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[1])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[0])
		self.vfailIf(self.nget.run("-g test -r ."))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 2)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 1)
		self.vfailUnlessEqual(self.servers.servers[2].count("_conns"), 1)
		self.verifyoutput('0002')

	def test_force_host_reset(self):
		self.servers = nntpd.NNTPD_Master(2)
		self.nget = util.TestNGet(ngetexe, self.servers.servers)
		self.servers.start()
		self.addarticle_toserver('0002', 'uuencode_multi3', '001', self.servers.servers[1])
		self.addarticle_toserver('0002', 'uuencode_multi3', '002', self.servers.servers[0])
		self.addarticle_toserver('0002', 'uuencode_multi3', '003', self.servers.servers[1])
		self.vfailIf(self.nget.run('-g test'))
		self.vfailIf(self.nget.run('-G test -h host0 -r nothing -h "" -r .'))
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 2)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 2)
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

	def test_NoSuchGroupAuth(self): #test if the command we were authenticating for failing is handled ok
		self.servers = nntpd.NNTPD_Master(3)
		self.servers.servers[1].adduser('ralph','5') #ralph has full auth
		self.servers.servers[1].adduser('','',{'group':0}) #default can't do GROUP
		self.nget = util.TestNGet(ngetexe, self.servers.servers, hostoptions=[{},{'user':'ralph', 'pass':'5'},{}])
		self.servers.start()
		self.addarticles_toserver('0002', 'uuencode_multi',self.servers.servers[0])
		self.vfailIf(self.nget.run("-g test -r ."))
		self.verifyoutput('0002')
		self.vfailUnlessEqual(self.servers.servers[0].count("_conns"), 1)
		self.vfailUnlessEqual(self.servers.servers[1].count("_conns"), 1)
		self.vfailUnlessEqual(self.servers.servers[2].count("_conns"), 1)

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
		self.vfailUnlessExitstatus(self.nget.run("-g test -r ."), 16, "nget process did not detect auth error")

	def test_NoAuth(self):
		self.servers = nntpd.NNTPD_Master(1)
		self.servers.servers[0].adduser('ralph','5') #ralph has full auth
		self.servers.servers[0].adduser('','',{'group':0}) #default can't do GROUP
		self.nget = util.TestNGet(ngetexe, self.servers.servers)
		self.servers.start()
		self.addarticles('0002', 'uuencode_multi')
		self.vfailUnlessExitstatus(self.nget.run("-g test -r ."), 16, "nget process did not detect auth error")

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
	#little hack to allow to run only tests matching a certain prefix
	#run with ./test_nget.py [unittest args] [TestCases...] -X<testMethodPrefix>
	if len(sys.argv)>1 and sys.argv[-1].startswith('-X'):
		myTestLoader=unittest.TestLoader()
		myTestLoader.testMethodPrefix=sys.argv[-1][2:]
		unittest.main(argv=sys.argv[:-1], testLoader=myTestLoader)
	else:
		unittest.main()
