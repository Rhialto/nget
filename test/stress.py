#!/usr/bin/env python
#
#    stress.py - randomized testing of nget
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

import nntpd
import threading

import os, sys, time, random, atexit
from optik import Option, OptionValueError
def check_long (option, opt, value):
	try:
		return long(value)
	except ValueError:
		raise OptionValueError("option %s: invalid long value: %r" % (opt, value))

class MyOption (Option):
	TYPES = Option.TYPES + ("long",)
	TYPE_CHECKER = Option.TYPE_CHECKER.copy()
	TYPE_CHECKER["long"] = check_long

from optik import OptionParser
parser = OptionParser(option_class=MyOption)
#parser.add_option("-t", "--threads",
#		action="store", type="int", dest="threads",
#		help="number of threads of nget to run")
parser.add_option(None, "--nget",
		action="store", type="string", dest="nget", default=os.path.join(os.pardir,'nget'))
parser.add_option("-l", "--minlines",
		action="store", type="int", dest="minlines", default=5)
parser.add_option("-p", "--minparts",
		action="store", type="int", dest="minparts", default=0)
parser.add_option("-P", "--maxparts",
		action="store", type="int", dest="maxparts", default=25)
parser.add_option("-s", "--minsize",
		action="store", type="int", dest="minsize", default=100)
parser.add_option("-S", "--maxsize",
		action="store", type="int", dest="maxsize", default=1024*1024)
parser.add_option("-T", "--totalsize",
		action="store", type="int", dest="totalsize", default=8*1024*1024)
parser.add_option(None, "--servers",
		action="store", type="int", dest="numservers", default=3)
parser.add_option(None, "--groups",
		action="store", type="string", dest="groups", default='foo')
parser.add_option(None, "--seed",
		action="store", type="long", dest="seed", default=long(time.time()*256))

(options, args) = parser.parse_args()

servers = []
serverthreads = []
groups = options.groups.split(',')
files = {}
tot_errs = 0

def cleanup():
	if tot_errs==0:
		import shutil
		#shutil.rmtree(tmpdir)
		#print 'rmtree',tmpdir
		#print 'leaving',rcdir
		print 'rmtree',rcdir
		shutil.rmtree(rcdir)
	else:
		print 'errors, leaving',rcdir
	print 'seed was', options.seed

atexit.register(cleanup)

rcdir = os.path.join(os.environ.get('TMPDIR') or '/tmp', 'nget_stress_'+hex(random.randrange(0,sys.maxint)))
os.environ['NGETHOME'] = rcdir
os.mkdir(rcdir)
tmpdir = os.path.join(rcdir, 'tmp')
os.mkdir(tmpdir)

rand = random.Random(options.seed)

for i in range(0, options.numservers):
	server = nntpd.NNTPTCPServer(("127.0.0.1", 0)) #port 0 selects a port automatically.
	servers.append(server)

files = {}
def randfile(f, size, name):
	l = 0
	str = name + '\n'
	while l<size:
		f.write(name[:min(len(name), size-l)])
		l+=len(str)
	#f.close()

def populate():
	from cStringIO import StringIO
	import uu
	size_left = options.totalsize
	def randservers(allservers=servers[:]):
		rand.shuffle(allservers)
		return allservers[:rand.randrange(1,options.numservers+1)]
	fileno = 0
	while size_left > 0:
		size = rand.randrange(min(size_left, options.minsize), min(size_left, options.maxsize)+1)
		size_left -= size
		numparts = rand.randrange(options.minparts, options.maxparts)
		name = 'f%07x.dat'%fileno
		fileno += 1
		mid = '<'+name+'>'
		uuf = StringIO()
		f = StringIO()
		randfile(f, size, name)
		f.seek(0)
		uu.encode(f, uuf, name, 0664)
		files[name] = f.getvalue()
		lines = uuf.getvalue().splitlines()
		numparts = min((len(lines) - 2)/options.minlines, numparts)
		rnumparts = max(numparts, 1)
		partsize = len(lines) / rnumparts
		while partsize*rnumparts < len(lines):
			partsize+=1
		if partsize*(rnumparts-1) >= len(lines):
			rnumparts-=1
			numparts-=1
		for p in range(0, rnumparts):
			article = nntpd.FakeArticle(mid, name, p+1, numparts, groups, lines[p*partsize:(p+1)*partsize])
			for serv in randservers():
				serv.addarticle(groups, article)


print 'generating test posts...'	
populate()
	
rc = open(os.path.join(rcdir, '_ngetrc'), 'w')
rc.write("tries=1\n")
#rc.write("debug=3\n")
rc.write("debug=0\n")
rc.write("{halias\n")
for i in range(0, options.numservers):
	rc.write("""
 {host%i
  addr=127.0.0.1:%i
  fullxover=1
  id=%i
 }
"""%(i, servers[i].socket.getsockname()[1], i+1))
rc.write("}\n")
rc.close()
print 'starting servers...'
for server in servers:
	s=threading.Thread(target=server.serve_forever)
	#s.setDaemon(1)
	s.start()
	serverthreads.append(s)

print 'starting nget...'
tot_errs += os.system(options.nget+' -gfoo -p '+tmpdir+' -r .')
print 'verifying decoded files...'
fnames = files.keys()
fnames.sort()
err=0
for fk in fnames:
	fn = os.path.join(tmpdir, fk)
	#assert(open(fn).read() == files[fk])
	if not os.path.exists(fn):
		print fn,'does not exist'
		err+=1
		tot_errs+=1
	elif open(fn).read() != files[fk]:
		err+=1
		tot_errs+=1
		print fn,' != ',fk
print err, " errors"
	
	
#s=threading.Thread(target=os.system, args=(options.nget+' -gfoo',))
#s.start()
#s.join()
print 'stopping servers...'
for server in servers:
	server.stop_serving()
for s in serverthreads:
	s.join()
print 'exiting...'

#if __name__=="__main__":
#	main()
