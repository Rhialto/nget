#    nntpd.py - simple threaded nntp server classes for testing purposes.
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
	def syntax_error(self):
		self.nwrite("500 Syntax error or bad command")
	def nwrite(self, s):
		self.wfile.write(s+"\r\n")
	def handle(self):
		readline = self.rfile.readline
		self.nwrite("200 Hello World, %s"%(':'.join(map(str,self.client_address))))
		self.group = None
		while 1:
			rcmd = readline()
			if not rcmd: break
			rcmd = rcmd.strip()
			rs = rcmd.lower().split(' ',1)
			if len(rs)==1:
				cmd = rs[0]
				args = None
			else:
				cmd,args = rs
			func = getattr(self, 'cmd_'+cmd, None)
			if func and callable(func):
				r=func(args)
				if r==-1:
					break
			else:
				self.nwrite("500 command %r not recognized"%rcmd)

	def cmd_group(self, args):
		self.group = self.server.groups.get(args)
		if self.group:
			self.nwrite("200 %i %i %i group %s selected"%(self.group.high-self.group.low+1, self.group.low, self.group.high, args))
		else:
			self.nwrite("411 no such news group")
	def cmd_xover(self, args):
		rng = args.split('-')
		if len(rng)>1:
			low,high = map(int, rng)
		else:
			low = high = int(rng[0])
		keys = [k for k in self.group.articles.keys() if k>=low and k<=high]
		keys.sort()
		self.nwrite("200 XOVER "+str(rng))
		for anum in keys:
			article = self.group.articles[anum]
			self.nwrite(str(anum)+'\t%(subject)s\t%(author)s\t%(date)s\t%(mid)s\t%(references)s\t%(bytes)s\t%(lines)s'%vars(article))
		self.nwrite('.')
	def cmd_article(self, args):
		if args[0]=='<':
			article = self.server.articles[args]
		else:
			article = self.group.articles[int(args)]
		self.nwrite("200 Article "+args)
		self.nwrite(article.text)
		self.nwrite('.')
	def cmd_mode(self, args):
		if args=='reader':
			self.nwrite("200 MODE READER enabled")
		else:
			self.syntax_error()
	def cmd_quit(self, args):
		self.nwrite("205 Goodbye")
		return -1


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
	def __init__(self, addr, RequestHandlerClass=NNTPRequestHandler):
		StoppableThreadingTCPServer.__init__(self, addr, RequestHandlerClass)
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
	def __init__(self, servers_num):
		self.servers = []
		self.threads = []
		if type(servers_num)==type(1): #servers_num is integer number of servers to start
			for i in range(servers_num):
				self.servers.append(NNTPTCPServer(("127.0.0.1", 0))) #port 0 selects a port automatically.
		else: #servers_num is a list of servers already created
			self.servers.extend(servers_num)
			
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
		self.text = '\r\n'.join(a)
		self.bytes = len(self.text)

import rfc822
class FileArticle:
	def __init__(self, fobj):
		msg = rfc822.Message(fobj)
		self.author = msg.get("From")
		self.subject = msg.get("Subject")
		self.date = msg.get("Date")
		self.mid = msg.get("Message-ID")
		self.references = msg.get("References", '')
		a = [l.rstrip() for l in msg.headers]
		a.append('')
		for l in fobj.xreadlines():
			if l[0]=='.':
				a.append('.'+l.rstrip())
			else:
				a.append(l.rstrip())
		self.text = '\r\n'.join(a)
		self.lines = len(a) - 1 - len(msg.headers)
		self.bytes = len(self.text)
