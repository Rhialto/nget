#!/usr/bin/env python
#
#    dumbnntpd.py - simple threaded nntp server daemon for testing nntp client posting.
#    Saves all POSTed articles in sequentially numbered files in the current dir.
#
#    Copyright (C) 2002-2003  Matthew Mueller <donut AT dakotacom.net>
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

import nntpd, threading, time, rfc822

def chomp(line):
	if line[-2:] == '\r\n': return line[:-2]
	elif line[-1:] in '\r\n': return line[:-1]
	return line

def genmid(n):
	return '<%i.%s@dumbnntpd>'%(n,time.time())

n=1
nlock = threading.Lock()

class NNTPRequestHandler(nntpd.NNTPRequestHandler):
	def cmd_post(self, args):
		self.nwrite("340 send article to be posted.")
		nlock.acquire()
		global n
		myn = n
		n += 1
		nlock.release()
		f = open("%03i"%myn, "w")
		inheader = 1
		hasmid = 0
		hasdate = 0
		while 1:
			l = self.rfile.readline()
			if not l: return -1
			l = chomp(l)
			if inheader:
				if l=='':
					inheader=0
					if not hasdate:
						date = rfc822.formatdate()
						print "generated date %s for post %s"%(date, myn)
						f.write('Date: %s\n'%date)
					if not hasmid:
						mid = genmid(myn)
						print "generated mid %s for post %s"%(mid, myn)
						f.write('Message-ID: %s\n'%mid)
				elif l.lower().startswith('message-id: '):
					hasmid = 1
				elif l.lower().startswith('date: '):
					hasdate = 1
			if l=='.':
				f.close()
				self.server.addarticle(["test"], nntpd.FileArticle(open(f.name)))
				self.nwrite("240 article posted ok")
				return
			f.write(l+'\n')

def main():
	import sys,getopt
	port = 119
	opts, args = getopt.getopt(sys.argv[1:], "p:")
	for o,a in opts:
		if o=='-p':
			port = int(a)
	servers = nntpd.NNTPD_Master([nntpd.NNTPTCPServer((nntpd.serveraddr,port), NNTPRequestHandler)])
	for a in args:
		servers.servers[0].addarticle(["test"], nntpd.FileArticle(open(a)))
	servers.start()

	print 'press enter to stop'
	raw_input()

	servers.stop()

if __name__=="__main__":
	main()
