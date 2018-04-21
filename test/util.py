#    util.py - nget test utility stuff
#    Copyright (C) 2002-2004  Matthew Mueller <donut AT dakotacom.net>
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

import os, random, sys, shutil, nntpd

def exitstatus(st):
	if not hasattr(os, 'WEXITSTATUS'): #the os.W* funcs are only on *ix
		return st
	if os.WIFSTOPPED(st):
		return 'stopped',os.WSTOPSIG(st)
	if os.WIFSIGNALED(st):
		return 'signal',os.WTERMSIG(st)
	if os.WIFEXITED(st):
		return os.WEXITSTATUS(st)
	return 'unknown',st

def spawnstatus(st):
	if st<0:
		return 'signal',-st
	return st

def vsystem(cmd):
	print 'running %r'%cmd
	return exitstatus(os.system(cmd))

def vspawn(cmd, args, spawn=getattr(os,"spawnvp",os.spawnv)):
	print 'running %r %r'%(cmd,args)
	return spawnstatus(spawn(os.P_WAIT, cmd, [cmd]+args))

class TestNGet:
	def __init__(self, nget, servers, **kw):
		self.exe = nget
		self.rcdir = os.path.join(os.environ.get('TMPDIR') or '/tmp', 'nget_test_'+hex(random.randrange(0,sys.maxint)))
		os.mkdir(self.rcdir)
		self.tmpdir = os.path.join(self.rcdir, 'tmp')
		os.mkdir(self.tmpdir)
		self.writerc(servers, **kw)
	
	def _dowriterc(self):
		rc = open(self.cur_rcfilename, 'w')
		for k,v in self.cur_options.items():
			rc.write("%s=%s\n"%(k,v))

		rc.write("{halias\n")
		for i in range(0, len(self.cur_servers)):
			rc.write(" {host%i\n"%i)
			for k,v in self.cur_hostoptions[i].items():
				rc.write("%s=%s\n"%(k,v))
			rc.write(" }\n")
		rc.write("}\n")
		
		if self.cur_priorities:
			rc.write("{hpriority\n")
			rc.write(" {default\n")
			for i,p in zip(range(0, len(self.cur_servers)), self.cur_priorities):
				rc.write("  host%i=%s\n"%(i, p))
			rc.write(" }\n")
			rc.write("}\n")
			
		rc.write(self.cur_extratail)
		rc.close()

		#print 'begin ngetrc:\n',open(os.path.join(self.rcdir, '_ngetrc'), 'r').read()
		#print '--end ngetrc'
		

	def writerc(self, servers, priorities=None, options=None, hostoptions=None, extratail="", rcfilename=None):
		self.cur_options = {
#			'tries': 1,
			'tries': 2,
			'delay': 0,
#			'penaltystrikes': 1,
#			'debug': 3,
#			'debug': 2,
#			'fullxover': 1
		}
		self.cur_servers = tuple(servers)

		self.cur_rcfilename = rcfilename or '_ngetrc'
		if os.sep not in self.cur_rcfilename:
			self.cur_rcfilename = os.path.join(self.rcdir, self.cur_rcfilename)
		if options:
			self.cur_options.update(options)
		self.cur_hostoptions = [None]*len(self.cur_servers)
		for i in range(0, len(self.cur_servers)):
			self.cur_hostoptions[i] = {
				'addr': nntpd.addressstr(self.cur_servers[i].socket.getsockname()),
				'shortname': 'h%i'%i,
				'id': str(i+1),
			}
			if hostoptions:
				self.cur_hostoptions[i].update(hostoptions[i])

		self.cur_priorities = priorities and tuple(priorities) or None

		self.cur_extratail = extratail

		self._dowriterc()
	
	def updaterc(self, priorities=None, options=None, hostoptions=None):
		if priorities:
			self.cur_priorities = priorities
		if options:
			self.cur_options.update(options)
		if hostoptions:
			for i in range(0, len(self.cur_servers)):
				self.cur_hostoptions[i].update(hostoptions[i])
		self._dowriterc()
	
	def runv(self, args):
		os.environ['NGETHOME'] = self.rcdir
		#return vspawn(self.exe, ["-p",self.tmpdir]+args)
		return vspawn(self.exe, args)
	
	def run(self, args, pre="", dopath=1):
		os.environ['NGETHOME'] = self.rcdir
		if dopath:
			args = "-p "+self.tmpdir+" "+args
		return vsystem(pre + self.exe+" "+args)
	
	def run_getoutput(self, args, pre="", dopath=1):
		outputpath = os.path.join(self.rcdir,'output.'+hex(random.randrange(0,sys.maxint)))
		status = self.run(args+' > '+outputpath+' 2>&1', pre=pre, dopath=dopath) #the 2>&1 will break win9x, but I'm not sure if the test suite ever actually worked there anyway.
		f = open(outputpath, "r")
		output = f.read()
		f.close()
		return status, output
	
	def runlite(self, args, pre=""):
		olddir = os.getcwd()
		try:
			ngetliteexe = os.path.join(os.path.split(self.exe)[0], 'ngetlite')
			os.chdir(self.tmpdir)
			return vsystem(pre + ngetliteexe+" "+args)
		finally:
			os.chdir(olddir)

	
	def clean_tmp(self):
		shutil.rmtree(self.tmpdir)
		os.mkdir(self.tmpdir)
		
	def clean_all(self):
		if os.environ.get('TEST_NGET_NOCLEAN'):
			return
		shutil.rmtree(self.rcdir)
		

