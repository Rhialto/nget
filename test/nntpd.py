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

class NNTPError(Exception):
	def __init__(self, code, text):
		self.code=code
		self.text=text
	def __str__(self):
		return '%03i %s'%(self.code, self.text)
class NNTPNoSuchGroupError(NNTPError):
	def __init__(self, g):
		NNTPError.__init__(self, 411, "No such newsgroup %s"%g)
class NNTPNoGroupSelectedError(NNTPError):
	def __init__(self):
		NNTPError.__init__(self, 412, "No newsgroup currently selected")
class NNTPNoSuchArticleNum(NNTPError):
	def __init__(self, anum):
		NNTPError.__init__(self, 423, "No such article %s in this newsgroup"%anum)
class NNTPNoSuchArticleMID(NNTPError):
	def __init__(self, mid):
		NNTPError.__init__(self, 430, "No article found with message-id %s"%mid)
class NNTPSyntaxError(NNTPError):
	def __init__(self, s=''):
		NNTPError.__init__(self, 500, "Syntax error or bad command" + (s and ' (%s)'%s or ''))

class NNTPAuthRequired(NNTPError):
	def __init__(self):
		NNTPError.__init__(self, 480, "Authorization required")
class NNTPAuthPassRequired(NNTPError):
	def __init__(self):
		NNTPError.__init__(self, 381, "PASS required")
class NNTPAuthError(NNTPError):
	def __init__(self):
		NNTPError.__init__(self, 502, "Authentication error")

class NNTPDisconnect(Exception):
	def __init__(self, err=None):
		self.err=err

class AuthInfo:
	def __init__(self, user, password, caps=None):
		self.user=user
		self.password=password
		if caps is None:
			caps = {}
		self.caps=caps
	def has_auth(self, cmd):
		if not self.caps.has_key(cmd):
			if cmd in ('quit', 'authinfo'): #allow QUIT and AUTHINFO even if default has been set to no auth
				return 1
			return self.caps.get('*', 1) #default to full auth
		return self.caps[cmd]


class NNTPRequestHandler(SocketServer.StreamRequestHandler):
	def nwrite(self, s):
		self.wfile.write(s+"\r\n")
	def handle(self):
		self.server.incr_conn()
		readline = self.rfile.readline
		self.nwrite("200 Hello World, %s"%(':'.join(map(str,self.client_address))))
		self.group = None
		self._tmpuser = None
		self.authed = self.server.auth['']
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
			try:
				if func and callable(func):
					if not self.authed.has_auth(cmd):
						raise NNTPAuthRequired
					func(args)
				else:
					raise NNTPSyntaxError, rcmd
			except NNTPDisconnect, d:
				if d.err:
					self.nwrite(str(d.err))
				return
			except NNTPError, e:
				self.nwrite(str(e))

	def cmd_authinfo(self, args):
		cmd,arg = args.split(' ',1)
		if cmd=='user':
			self._tmpuser=arg
			raise NNTPAuthPassRequired
		elif cmd=='pass':
			if not self._tmpuser:
				raise NNTPAuthError
			a = self.server.auth.get(self._tmpuser)
			if not a:
				raise NNTPAuthError
			if arg != a.password:
				raise NNTPAuthError
			self.authed = a
			self.nwrite("281 Authentication accepted")
		else:
			raise NNTPSyntaxError, args
			
	def cmd_group(self, args):
		self.group = self.server.groups.get(args)
		if not self.group:
			raise NNTPNoSuchGroupError, args
		self.nwrite("200 %i %i %i group %s selected"%(self.group.high-self.group.low+1, self.group.low, self.group.high, args))
	def cmd_xover(self, args):
		if not self.group:
			raise NNTPNoGroupSelectedError
		rng = args.split('-')
		if len(rng)>1:
			low,high = map(long, rng)
		else:
			low = high = long(rng[0])
		keys = [k for k in self.group.articles.keys() if k>=low and k<=high]
		keys.sort()
		self.nwrite("200 XOVER "+str(rng))
		for anum in keys:
			article = self.group.articles[anum]
			self.nwrite(str(anum)+'\t%(subject)s\t%(author)s\t%(date)s\t%(mid)s\t%(references)s\t%(bytes)s\t%(lines)s'%vars(article))
		self.nwrite('.')
	def cmd_article(self, args):
		self.server.incr_retr()
		if args[0]=='<':
			try:
				article = self.server.articles[args]
			except KeyError:
				raise NNTPNoSuchArticleMID, args
		else:
			if not self.group:
				raise NNTPNoGroupSelectedError
			try:
				article = self.group.articles[long(args)]
			except KeyError:
				raise NNTPNoSuchArticleNum, args
		self.nwrite("200 Article "+args)
		self.nwrite(article.text)
		self.nwrite('.')
	def cmd_mode(self, args):
		if args=='reader':
			self.nwrite("200 MODE READER enabled")
		else:
			raise NNTPSyntaxError, args
	def cmd_quit(self, args):
		raise NNTPDisconnect("205 Goodbye")


class _TimeToQuit(Exception): pass

class StoppableThreadingTCPServer(SocketServer.ThreadingTCPServer):
	def __init__(self, addr, handler):
		if os.name == "nt":
			import socket
			s1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			s1.bind(('127.0.0.1',0))
			s1.listen(1)
			s2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			s2.connect(s1.getsockname())
			self.controlr = s1.accept()[0]
			self.controlw = s2
			s1.close()
		else:
			self.controlr, self.controlw = os.pipe()
		SocketServer.ThreadingTCPServer.__init__(self, addr, handler)

	def stop_serving(self):
		if hasattr(self.controlw, 'send'):
			self.controlw.send('FOO')
		else:
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
		self.auth = {}
		self.adduser('','')
		self.lock = threading.Lock()
		self.conns = 0
		self.retrs = 0
		
	def incr_conn(self):
		self.lock.acquire()
		self.conns += 1
		self.lock.release()
	def incr_retr(self):
		self.lock.acquire()
		self.retrs += 1
		self.lock.release()

	def adduser(self, user, password, caps=None):
		self.auth[user]=AuthInfo(user, password, caps)

	def addarticle(self, groups, article, anum=None):
		self.midindex[article.mid]=article
		for g in groups:
			#if g not in self.groups:
			if not self.groups.has_key(g):
				self.groups[g]=Group()
			self.groups[g].addarticle(article, anum)
	
	def rmarticle(self, mid):
		article = self.midindex[mid]
		for g in self.groups.values():
			g.rmarticle(article)

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
		self.high = 0
		self.articles = {}
	def addarticle(self, article, anum=None):
		if anum is None:
			anum = self.high + 1
		if self.articles.has_key(anum):
			raise Exception, "already have article %s"%anum
		self.articles[anum] = article
		if anum > self.high:
			self.high = anum
		if anum < self.low:
			self.low = anum
	def rmarticle(self, article):
		for k,v in self.articles.items():
			if v==article:
				del self.articles[k]
				if self.articles:
					self.low = min(self.articles.keys())
				else:
					self.low = self.high + 1
				return

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
