from __future__ import nested_scopes

import Memory
from Trick import Trick

import errno
import os
import re
import socket
import FCNTL

import tricklib
import ptrace

class _info:
	def __init__(self):
		self.failed = 0
		self.bytes = 0
class FDInfo:
	def __init__(self, errs):
		self.w = _info()
		self.r = _info()
		self.errs = errs
		self.rerrs = []
		self.werrs = []
		self.cerrs = []
		self.modes = ''
		for err in errs:
			if err.after=='c':
				self.cerrs.append(err)
			else:
				if 'r' in err.modes:
					self.rerrs.append(err)
				if 'w' in err.modes:
					self.werrs.append(err)
			if 'r' in err.modes and 'r' not in self.modes: self.modes += 'r'
			if 'w' in err.modes and 'w' not in self.modes: self.modes += 'w'

class FError:
	def __init__(self, match, modes, after):
		self.match = re.compile(match)
		self.modes = modes
		self.after = after

class SError:
	def __init__(self, modes, after):
		self.modes = modes
		self.after = after

class RenameError:
	def __init__(self, sfn, dfn):
		self.srcmatch = re.compile(sfn)
		self.dstmatch = re.compile(dfn)

class IOError(Trick): #somewhat unfortunate, you must name <blah>Trick class as <blah>, so IOErrorTrick becomes the possibly confusing IOError.  doh.
	def __init__(self, options):
		self.__do_fail = {}
		self.ferrs = []
		self.serrs = []
		self.renameerrs = []
		self._catchmodes = ''
		def updatemodes(mode, after):
			if after=='c' and 'C' not in self._catchmodes:
				self._catchmodes+='C'
			else:
				if 'r' in mode and 'r' not in self._catchmodes:
					self._catchmodes+='r'
				if 'w' in mode and 'w' not in self._catchmodes:
					self._catchmodes+='w'
		defopts = "rw", 0
		if options.has_key('f'):
			for opts in options['f']:
				if type(opts)==type(""):
					opts = (opts,) + defopts
				else:
					opts = opts + defopts[len(opts)-1:]
				updatemodes(opts[1], opts[2])
				fe = FError(*opts)
				self.ferrs.append(fe)
		if options.has_key('s'):
			for opts in options['s']:
				if type(opts)==type(""):
					opts = (opts,) + defopts[1:]
				else:
					opts = opts + defopts[len(opts):]
				updatemodes(opts[0], opts[1])
				fe = SError(*opts)
				self.serrs.append(fe)
		if options.has_key('r'):
			for opts in options['r']:
				re = RenameError(*opts)
				self.renameerrs.append(re)


	def callbefore(self, pid, call, args):
		def doreadwrite():
			fd = args[0]
			pfd = pid,fd
			fdinfo = self.__do_fail.get(pfd, None)
#			if fdinfo:
#				print fdinfo
			if fdinfo and call[0] in fdinfo.modes:
				if call[0] == 'r':
					#after = self.afterr
					after = min([err.after for err in fdinfo.rerrs])
					failinfo = fdinfo.r
				else:
					#after = self.afterw
					after = min([err.after for err in fdinfo.werrs])
					failinfo = fdinfo.w
				failcount, bytes = failinfo.failed, failinfo.bytes

				if failcount > 5:
					print 'exiting on %ith failed %s of %i'%(failcount, call, pfd)
					import sys
					sys.exit(1)

				if bytes < after:
					size = args[2]
					if bytes + size > after:
						size = after - bytes
					print pfd, call, (fd, args[1], size), 'was', args
					return (pfd, None, None, (fd, args[1], size))
					
				failinfo.failed += 1
				print pid,'failing',call,'#%i'%failcount, 'for fd', fd
				return (pfd, -errno.EIO, None, None)
#			else:
#				print 'allowing',call,'for',args[0]
	
		if call == 'read':
			return doreadwrite()
		elif call == 'write':
			return doreadwrite()
		elif call == 'close':
			fd = args[0]
			pfd = pid,fd
			fdinfo = self.__do_fail.get(pfd, None)
			if fdinfo and fdinfo.cerrs:
				return (None, -errno.EIO, None, None)
			
		elif call == 'dup' or call == 'dup2':
			return (args[0], None, None, None)
		elif call == 'open':
			getarg = Memory.getMemory(pid).get_string
			fn = getarg(args[0])
#			print pid,call,[fn]+args[1:],args[1]&FCNTL.O_ACCMODE
			fes = []
			m = ''
			flags = args[1] & FCNTL.O_ACCMODE
			for fe in self.ferrs:
				if (flags == FCNTL.O_RDWR or
						(flags == FCNTL.O_WRONLY and 'w' in fe.modes) or 
						(flags == FCNTL.O_RDONLY and 'r' in fe.modes)):
					if fe.match.search(fn):
						fes.append(fe)
			if fes:
				fdinfo = FDInfo(fes)
				#if flags == FCNTL.O_WRONLY:
				#	after = min([err.after for err in fdinfo.werrs])
				#elif flags == FCNTL.O_RDONLY:
				#	after = min([err.after for err in fdinfo.rerrs])
				#else: #elif flags == FCNTL.O_RDWR:
				after = min([err.after for err in fdinfo.errs])
				if after<0:
					print pid,'failing',call,[fn]+args[1:]
					return (None, -errno.EIO, None, None)
				return (fdinfo, None, None, None)

		elif call == 'socketcall':
#			print pid, call, args
			subcall = args[0]
			do = 0
			if subcall == 1:            # socket
				# FIX: might fail
				if ptrace.peekdata(pid, args[1]) == socket.AF_INET:
					do = -2
			elif subcall == 3:                # connect
				do = -1
			elif subcall == 4:                # listen
				do = -1
			elif subcall == 5:          # accept
				do = -1

			if do:
				#ses = []
				#for se in self.serrs:
				after = min([err.after for err in self.serrs])
				if after==do:
					print pid,'failing',call,args
					return (None, -errno.EIO, None, None)
				errs = [err for err in self.serrs if err.after>=0]
				if errs:
					fdinfo = FDInfo(errs)
					return (fdinfo, None, None, None)
		
		elif call == 'rename':
			getarg = Memory.getMemory(pid).get_string
			sfn = getarg(args[0])
			dfn = getarg(args[1])
#			print pid,call,sfn,dfn
			for rene in self.renameerrs:
				if rene.srcmatch.search(sfn) and rene.dstmatch.search(dfn):
					print pid,'failing',call,sfn,dfn
					return (None, -errno.EIO, None, None)

	
	def callafter(self, pid, call, result, state):
		if call == 'read':
			if result>0 and state!=None:
				self.__do_fail[state].r.bytes += result
		elif call == 'write':
			if result>0 and state!=None:
				self.__do_fail[state].w.bytes += result
		elif call == 'open' and result != -1:
			self.__do_fail[pid,result] = state
		elif call == 'socketcall' and result != -1:
			self.__do_fail[pid,result] = state
		elif (call == 'dup' or call == 'dup2') and result != -1:
			self.__do_fail[pid,result] = self.__do_fail.get((pid,state), None)
								

	def callmask(self):
		d = {}
		if self.renameerrs:
			d['rename']=1
		if self.serrs:
			d['socketcall']=1
			d['dup']=1
			d['dup2']=1
		if self.ferrs:
			d['open']=1
			d['dup']=1
			d['dup2']=1
		if 'r' in self._catchmodes:
			d['read']=1
		if 'w' in self._catchmodes:
			d['write']=1
		if 'C' in self._catchmodes:
			d['close']=1
		return d
