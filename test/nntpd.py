#    nntpd.py - simple threaded nntp server for testing purposes.
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

import os, select
import threading
import SocketServer

class NNTPRequestHandler(SocketServer.StreamRequestHandler):
	def handle(self):
		readline, writef = self.rfile.readline, self.wfile.write
		def nwrite(s):
			writef(s+"\r\n")
		nwrite("200 Hello World, %s!"%(':'.join(map(str,self.client_address))))
		serv = self.server
		group = None
		while 1:
			rcmd = readline()
			if not rcmd: break
			rcmd = rcmd.strip()
			rs = rcmd.lower().split(' ',1)
			if len(rs)==1:
				cmd = rs[0]
			else:
				cmd,args = rs
			if cmd=='group':
				group = serv.groups[args]
				nwrite("200 %i %i %i group %s selected"%(group.high-group.low+1, group.low, group.high, args))
			elif cmd=='xover':
				rng = args.split('-')
				if len(rng)>1:
					low,high = map(int, rng)
				else:
					low = high = int(rng[0])
				keys = [k for k in group.articles.keys() if k>=low and k<=high]
				keys.sort()
				nwrite("200 XOVER "+str(rng))
				for anum in keys:
					article = group.articles[anum]
					nwrite(str(anum)+'\t%(subject)s\t%(author)s\t%(date)s\t%(mid)s\t%(references)s\t%(bytes)s\t%(lines)s'%vars(article))
				nwrite('.')
			elif cmd=='article':
				if args[0]=='<':
					article = serv.articles[args]
				else:
					article = group.articles[int(args)]
				nwrite("200 Article "+args)
				nwrite(article.text)
				nwrite('.')
			elif cmd=='mode' and args=='reader':
				nwrite("200 MODE READER enabled")
			elif cmd=='quit':
				nwrite("200 Goodbye")
				break
			else:
				nwrite("500 command %r not recognized"%rcmd)


class _TimeToQuit(Exception): pass

class StoppableThreadingTCPServer(SocketServer.ThreadingTCPServer):
	def __init__(self, addr, handler):
		#self.controlsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.controlr, self.controlw = os.pipe()
		SocketServer.ThreadingTCPServer.__init__(self, addr, handler)

	def stop_serving(self):
		os.write(self.controlw, 'FOO')
	
	def get_request(self):
		readfds = [self.socket, self.controlr]
		while 1:
			ready = select.select(readfds, [], [])
			if self.controlr in ready[0]:
				raise _TimeToQuit
			if self.socket in ready[0]:
				return SocketServer.ThreadingTCPServer.get_request(self)
		
	def serve_forever(self):
		try:
			SocketServer.ThreadingTCPServer.serve_forever(self)
		except _TimeToQuit:
			self.server_close() # Clean up before we leave

class NNTPTCPServer(StoppableThreadingTCPServer):
	def __init__(self, addr):
		StoppableThreadingTCPServer.__init__(self, addr, NNTPRequestHandler)
		self.groups = {}
		self.midindex = {}
		
	def addarticle(self, groups, article):
		self.midindex[article.mid]=article
		for g in groups:
			#if g not in self.groups:
			if not self.groups.has_key(g):
				self.groups[g]=Group()
			self.groups[g].addarticle(article)

class NNTPD_Master:
	def __init__(self, num):
		self.servers = []
		self.threads = []
		for i in range(num):
			self.servers.append(NNTPTCPServer(("127.0.0.1", 0))) #port 0 selects a port automatically.
			
	def start(self):
		for server in self.servers:
			s=threading.Thread(target=server.serve_forever)
			#s.setDaemon(1)
			s.start()
			self.threads.append(s)
			
	def stop(self):
		for server in self.servers:
			server.stop_serving()
		for thread in self.threads:
			thread.join()
		self.threads = []


class Group:
	def __init__(self):
		self.low = 1
		self.high = 1
		self.articles = {}
	def addarticle(self, article):
		self.articles[self.high] = article
		self.high += 1

import time
class FakeArticle:
	def __init__(self, mid, name, partno, totalparts, groups, body):
		self.mid=mid
		self.references=''
		a = []
		def add(foo):
			a.append(foo)
		add("Newsgroups: "+' '.join(groups))
		if totalparts>0:
			self.subject="%(name)s [%(partno)i/%(totalparts)i]"%vars()
		else:
			self.subject="Subject: %(name)s"%vars()
		add("Subject: "+self.subject)
		self.author = "<noone@nowhere> (test)"
		self.lines = len(body)
		add("From: "+self.author)
		self.date=time.ctime()
		add("Date: "+self.date)
		add("Lines: %i"%self.lines)
		add("Message-ID: "+mid)
		add("")
		for l in body:
			if l[0]=='.':
				add('.'+l)
			else:
				add(l)
		#a.append("")#ensure last line has crlf
		self.text = '\r\n'.join(a)
		self.bytes = len(self.text)
#Date: Mon, 01 Oct 2001 07:47:13 GMT
#NNTP-Posting-Host: 65.1.111.84
#X-Complaints-To: abuse@home.net
#X-Trace: news2.rdc2.tx.home.com 1001922433 65.1.111.84 (Mon, 01 Oct 2001 00:47:13 PDT)
#NNTP-Posting-Date: Mon, 01 Oct 2001 00:47:13 PDT
#Xref: sn-us alt.binaries.foo:880414
