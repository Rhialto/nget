#!/usr/bin/env python
#
#    stressg.py - randomized testing of nget -g
#    Copyright (C) 2002-2005  Matthew Mueller <donut AT dakotacom.net>
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

import nntpd,util

import os, sys, re, time, random, atexit
from optparse import Option, OptionValueError
def check_long (option, opt, value):
	try:
		return long(value)
	except ValueError:
		raise OptionValueError("option %s: invalid long value: %r" % (opt, value))

class MyOption (Option):
	TYPES = Option.TYPES + ("long",)
	TYPE_CHECKER = Option.TYPE_CHECKER.copy()
	TYPE_CHECKER["long"] = check_long

from optparse import OptionParser
parser = OptionParser(option_class=MyOption)
#parser.add_option("-t", "--threads",
#		action="store", type="int", dest="threads",
#		help="number of threads of nget to run")
parser.add_option("--nget",
		action="store", type="string", dest="nget", default=os.path.join(os.pardir,'nget'))
parser.add_option("-p", "--minparts",
		action="store", type="int", dest="minparts", default=1)
parser.add_option("-P", "--maxparts",
		action="store", type="int", dest="maxparts", default=25)
parser.add_option("-T", "--totalparts",
		action="store", type="int", default=100000)
parser.add_option("--dupearticles",
		action="store", type="int", default=1)
parser.add_option("--servers",
		action="store", type="int", dest="numservers", default=3)
parser.add_option("--groups",
		action="store", type="string", dest="groups", default='foo')
parser.add_option("--seed",
		action="store", type="long", dest="seed", default=long(time.time()*256))

(options, args) = parser.parse_args()

groups = options.groups.split(',')
tot_errs = 0

def cleanup():
	print 'stopping servers...'
	servers.stop()
	print 'exiting...'

	if tot_errs==0:
		testnget.clean_all()
	else:
		print 'errors, leaving',testnget.rcdir
	print 'seed was', options.seed

atexit.register(cleanup)

rand = random.Random(options.seed)

servers = nntpd.NNTPD_Master(options.numservers)
file_numparts = {}

def populate():
	parts_left = options.totalparts
	def randservers(allservers=servers.servers[:]):
		rand.shuffle(allservers)
		servs = allservers[:rand.randrange(1,options.numservers+1)]
		assert servs
		return servs
	fileno = 0
	while parts_left > 0:
		numparts = rand.randrange(options.minparts, options.maxparts)
		parts_left -= numparts
		name = 'f%07x.dat'%fileno
		file_numparts[name]=numparts
		fileno += 1
		for p in range(0, numparts):
			mid = '<%s.%s>'%(name,p)
			article = nntpd.FakeArticleHeaderOnly(mid, name, p+1, numparts, groups)
			for serv in randservers():
				for i in range(0,options.dupearticles):
					serv.addarticle(groups, article)


try:
	print 'generating test posts...'
	populate()

	testnget = util.TestNGet(options.nget, servers.servers)
		
	print 'starting servers...'
	servers.start()

	print 'starting nget...'
	start = time.time()
	tot_errs += testnget.run('-gfoo')
	print 'nget -g took',time.time()-start,'sec'

	print 'verifying header cache...'
	start = time.time()
	st,output=testnget.run_getoutput('-Gfoo -l0 -Tr .')
	print 'nget -Tr took',time.time()-start,'sec'
	name_re = re.compile(r'\bf\w+\.dat\b')
	parts_re = re.compile(r'^\d+')
	matched = {}
	for line in output.splitlines():
		x = name_re.search(line)
		if x:
			name = x.group(0)
			y = parts_re.search(line)
			assert y, repr(line)
			assert int(y.group(0))==file_numparts[name], repr((file_numparts[name],line))
			matched[name] = 1
	assert len(matched)==len(file_numparts), repr((len(matched),len(file_numparts)))
	print 'verification complete'

	print 'flushing header cache...'
	for serv in servers.servers:
		serv.rmallarticles()
	start = time.time()
	tot_errs += testnget.run('-gfoo')
	print 'nget -g (flushing) took',time.time()-start,'sec'
except AssertionError:
	tot_errs += 1
	raise

#if __name__=="__main__":
#	main()
