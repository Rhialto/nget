#    util.py - nget test utility stuff
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

import os, random, sys, shutil

class TestNGet:
	def __init__(self, nget, servers):
		self.exe = nget
		self.rcdir = os.path.join(os.environ.get('TMPDIR') or '/tmp', 'nget_test_'+hex(random.randrange(0,sys.maxint)))
		os.mkdir(self.rcdir)
		self.tmpdir = os.path.join(self.rcdir, 'tmp')
		os.mkdir(self.tmpdir)

		rc = open(os.path.join(self.rcdir, '_ngetrc'), 'w')
		rc.write("tries=1\n")
#rc.write("debug=3\n")
		rc.write("debug=0\n")
		rc.write("{halias\n")
		for i in range(0, len(servers)):
			rc.write("""
 {host%i
  addr=%s
  fullxover=1
  id=%i
 }
"""%(i, ':'.join(map(str,servers[i].socket.getsockname())), i+1))
		rc.write("}\n")
		rc.close()
	
	def run(self, args):
		os.environ['NGETHOME'] = self.rcdir
		return os.system(self.exe+" -p "+self.tmpdir+" "+args)
	
	def clean_all(self):
		shutil.rmtree(self.rcdir)
		
