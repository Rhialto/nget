import os, random, sys

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
		import shutil
		#shutil.rmtree(tmpdir)
		#print 'rmtree',tmpdir
		#print 'leaving',rcdir
		#print 'rmtree',self.rcdir
		shutil.rmtree(self.rcdir)
		

