#!/usr/bin/env python
#
#    dumbnntpd.py - simple threaded nntp server daemon for testing nntp client posting.
#    Saves all POSTed articles in sequentially numbered files in the current dir.
#
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

import nntpd, threading

def chomp(line):
	if line[-2:] == '\r\n': return line[:-2]
	elif line[-1:] in '\r\n': return line[:-1]
	return line


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
		while 1:
			l = self.rfile.readline()
			if not l: return -1
			l = chomp(l)
			if l=='.':
				f.close()
				self.nwrite("240 article posted ok")
				return
			f.write(l+'\n')

def main():
	server = nntpd.NNTPTCPServer(("127.0.0.1",119), NNTPRequestHandler)
	thread = threading.Thread(target=server.serve_forever)
	thread.start()
	
	print 'press enter to stop'
	raw_input()

	server.stop_serving()
	thread.join()

if __name__=="__main__":
	main()
