#    nntpd.py - simple threaded nntp server classes for testing purposes.
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

from __future__ import nested_scopes

import os, select
import time
import threading
import SocketServer
import socket

#allow testing with ipv6.
if os.environ.get('TEST_NGET_IPv6'):
	serveraddr="::1"
	serveraf=socket.AF_INET6
else:
	serveraddr="127.0.0.1"
	serveraf=socket.AF_INET

def addressstr(address):
	host,port=address[:2]
	if ':' in host:
		return '[%s]:%s'%(host,port)
	return '%s:%s'%(host,port)

def chomp(line):
	if line[-2:] == '\r\n': return line[:-2]
	elif line[-1:] in '\r\n': return line[:-1]
	return line

import fnmatch
class WildMat:
	def __init__(self, pat):
		####pat should really be massaged into a regex since the wildmat semantics are not the same as that of fnmatch
		self.pat = pat
	def __call__(self, arg):
		return fnmatch.fnmatchcase(arg, self.pat)
def MatchAny(arg):
	return 1

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
class NNTPBadCommand(NNTPError):
	def __init__(self, s=''):
		NNTPError.__init__(self, 500, "Bad command" + (s and ' (%s)'%s or ''))
class NNTPSyntaxError(NNTPError):
	def __init__(self, s=''):
		NNTPError.__init__(self, 501, "Syntax error" + (s and ' (%s)'%s or ''))

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

def split_cmd(rcmd):
	rs = rcmd.split(' ',1)
	rs[0] = rs[0].lower()
	if len(rs)==1:
		return rs[0], ''
	else:
		return rs

class NNTPRequestHandler(SocketServer.StreamRequestHandler):
	def nwrite(self, s):
		self.wfile.write(s+"\r\n")
	def call_command(self, cmd, args):
		func = getattr(self, 'cmd_'+cmd, None)
		if func and callable(func):
			if not self.authed.has_auth(cmd):
				raise NNTPAuthRequired
			self.server.incrcount(cmd)
			func(args)
		else:
			raise NNTPBadCommand, cmd
	def handle(self):
		self.server.incrcount("_conns")
		readline = self.rfile.readline
		self.nwrite("200 Hello World, %s"%addressstr(self.client_address))
		self.group = None
		self._tmpuser = None
		self.authed = self.server.auth['']
		while 1:
			rcmd = readline()
			if not rcmd: break
			rcmd = rcmd.strip()
			cmd,args = split_cmd(rcmd)
			try:
				self.call_command(cmd, args)
			except NNTPDisconnect, d:
				if d.err:
					self.nwrite(str(d.err))
				return
			except NNTPError, e:
				self.nwrite(str(e))

	def cmd_authinfo(self, args):
		cmd,arg = split_cmd(args)
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
	
	def cmd_date(self, args):
		self.nwrite("111 "+time.strftime("%Y%m%d%H%M%S",time.gmtime()))
	
	def cmd_list(self, args):
		subcmd, args = split_cmd(args)
		self.call_command('list_'+subcmd, args)
	def cmd_list_newsgroups(self, args):
		self.nwrite("215 information follows")
		if args:
			matcher = WildMat(args)
		else:
			matcher = MatchAny
		for name,group in self.server.groups.items():
			if group.description and matcher(name):
				self.nwrite("%s %s"%(name, group.description))
		self.nwrite(".")
	def cmd_list_(self, args):
		if args:
			matcher = WildMat(args)
		else:
			matcher = MatchAny
		self.nwrite("215 list of newsgroups follows")
		for name,group in self.server.groups.items():
			if matcher(name):
				self.nwrite("%s %i %i %s"%(name, group.low, group.high, "y"))
		self.nwrite(".")
	cmd_list_active = cmd_list_

	def cmd_newgroups(self, args):
		#since = time.mktime(time.strptime(args,"%Y%m%d %H%M%S %Z"))
		since = ''.join(args.split()[0:2])
		if len(since)!=14:
			raise NNTPSyntaxError, args
		self.nwrite("231 list of new newsgroups follows")
		for name,group in self.server.groups.items():
			#if since < group.creationtime:
			if since < time.strftime("%Y%m%d%H%M%S",time.gmtime(group.creationtime)): #just do a lexicographical comparison. stupid c library. blah.
				self.nwrite("%s %i %i %s"%(name, group.low, group.high, "y"))
		self.nwrite(".")
		
	def cmd_listgroup(self, args):
		if args:
			group = self.server.groups.get(args)
			if not group:
				raise NNTPNoSuchGroupError, args
			self.group = group
		if not self.group:
			raise NNTPNoGroupSelectedError
		self.nwrite("211 list follows")
		anums = self.group.articles.keys()
		anums.sort()
		for an in anums:
			self.nwrite(str(an))
		self.nwrite(".")

	def cmd_group(self, args):
		self.group = self.server.groups.get(args)
		if not self.group:
			raise NNTPNoSuchGroupError, args
		self.nwrite("211 %i %i %i group %s selected"%(self.group.high-self.group.low+1, self.group.low, self.group.high, args))
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
		self.nwrite("224 Overview information follows "+str(rng))
		for anum in keys:
			article = self.group.articles[anum]
			self.nwrite(str(anum)+'\t%(subject)s\t%(author)s\t%(date)s\t%(mid)s\t%(references)s\t%(bytes)s\t%(lines)s'%vars(article))
		self.nwrite('.')
	def cmd_xpat(self, args):
		if not self.group:
			raise NNTPNoGroupSelectedError
		field,rng,pat = args.split(' ', 2)
		field = field.lower()
		if field == 'message-id':
			field = 'mid'
		####doesn't handle specifing message-id instead of range (nget doesn't use this, though)
		rng = rng.split('-')
		if len(rng)>1:
			low,high = map(long, rng)
		else:
			low = high = long(rng[0])
		keys = [k for k in self.group.articles.keys() if k>=low and k<=high]
		keys.sort()
		matcher = WildMat(pat)
		self.nwrite("221 %s fields follow"%field)
		for anum in keys:
			article = self.group.articles[anum]
			val = getattr(article, field, "")
			if matcher(val):
				self.nwrite(str(anum)+self.server.xpat_field_sep+val)
		self.nwrite('.')
	def cmd_article(self, args):
		if args[0]=='<':
			try:
				article = self.server.midindex[args]
			except KeyError:
				raise NNTPNoSuchArticleMID, args
			anum=0
		else:
			if not self.group:
				raise NNTPNoGroupSelectedError
			try:
				anum=long(args)
				article = self.group.articles[anum]
			except KeyError:
				raise NNTPNoSuchArticleNum, args
		self.nwrite("220 %i %s Article follows"%(anum,article.mid))
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
		self.address_family=serveraf
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
			if hasattr(self.controlw, 'close'):
				self.controlr.close()
				self.controlw.close()
			else:
				os.close(self.controlr)
				os.close(self.controlw)
			self.server_close() # Clean up before we leave

class NNTPTCPServer(StoppableThreadingTCPServer):
	def __init__(self, addr, RequestHandlerClass=NNTPRequestHandler):
		StoppableThreadingTCPServer.__init__(self, addr, RequestHandlerClass)
		self.groups = {}
		self.midindex = {}
		self.auth = {}
		self.adduser('','')
		self.lock = threading.Lock()
		self.counts = {}
		self.xpat_field_sep = ' '
	
	def count(self, key):
		return self.counts.get(key, 0)

	def incrcount(self, key):
		self.lock.acquire()
		self.counts[key] = self.count(key) + 1
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
		del self.midindex[mid]
		for g in self.groups.values():
			g.rmarticle(article)

	def rmallarticles(self):
		self.midindex = {}
		for g in self.groups.values():
			g.rmallarticles()

	def addgroup(self, name, desc=None):
		if self.groups.has_key(name):
			self.groups[name].description=desc
		else:
			self.groups[name]=Group(description=desc)

class NNTPD_Master:
	def __init__(self, servers_num):
		self.servers = []
		self.threads = []
		if type(servers_num)==type(1): #servers_num is integer number of servers to start
			for i in range(servers_num):
				self.servers.append(NNTPTCPServer((serveraddr, 0))) #port 0 selects a port automatically.
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
	def __init__(self, description=None):
		self.low = 1
		self.high = 0
		self.articles = {}
		self.creationtime = time.time()
		self.description = description
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
	def rmallarticles(self):
		self.articles = {}
		self.low = self.high + 1

class FakeArticleHeaderOnly:
	def __init__(self, mid, name, partno, totalparts, groups):
		self.mid=mid
		self.references=''
		if totalparts>0:
			self.subject="%(name)s [%(partno)i/%(totalparts)i]"%vars()
		else:
			self.subject="%(name)s"%vars()
		self.author = "<noone@nowhere> (test)"
		self.date=time.ctime()
		self.text = ''
		self.bytes = 0
		self.lines = 0

class FakeArticle(FakeArticleHeaderOnly):
	def __init__(self, mid, name, partno, totalparts, groups, body):
		FakeArticleHeaderOnly.__init__(self, mid, name, partno, totalparts, groups)
		a = []
		def add(foo):
			a.append(foo)
		add("Newsgroups: "+' '.join(groups))
		add("Subject: "+self.subject)
		self.lines = len(body)
		add("From: "+self.author)
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
				a.append('.'+chomp(l))
			else:
				a.append(chomp(l))
		self.text = '\r\n'.join(a)
		self.lines = len(a) - 1 - len(msg.headers)
		self.bytes = len(self.text)
