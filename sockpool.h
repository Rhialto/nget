/*
    sockpool.* - socket connection pool
    Copyright (C) 1999-2002  Matthew Mueller <donut@azstarnet.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __SOCKPOOL_H__
#define __SOCKPOOL_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdarg.h>
#include "server.h"
#include "file.h"
#include "log.h"

class Connection {
	protected:
		time_t lastuse;
	public:
		c_file_tcp sock;
		c_server *server;
		c_group_info::ptr curgroup;
		bool freshconnect;


		int putline(int echo,const char * str,...)
			__attribute__ ((format (printf, 3, 4)));
		int doputline(int echo,const char * str,va_list ap);
		int getline(int echo);

		
		time_t age(void) const {
			return time(NULL)-lastuse;
		}
		void touch(void) {
			lastuse=time(NULL);
		}
		int open(void) {
			freshconnect=true;
			curgroup=NULL;
			return sock.open(server->addr.c_str(), "nntp");
		}
		bool isopen(void) const {
			return sock.isopen();
		}
		void close(int fast=0) {
			if (sock.isopen()){
				if (!fast)
					putline(quiet<2,"QUIT");
				else if (quiet<2)
					printf("%s close(fast)\n", server->shortname.c_str());
				sock.close();
			}
		}
		Connection(c_server *serv):lastuse(0), server(serv){
			assert(serv);
			sock.initrbuf();
		}
};


typedef map<ulong, Connection *> t_connection_map;

class SockPool {
	protected:
		//t_connection_map used_connections;
		t_connection_map connections;
	public:
		
		bool is_connected(ulong serverid) {
			return (connections.find(serverid) != connections.end());
		}

//		int total_connections(void) const {
//			return free_connections.size() + used_connections.size();
//		}

		Connection* connect(ulong serverid);
		void release(Connection *connection);
		void expire_old_connection(void);
		void expire_connections(bool closeall=false);

		~SockPool() {
			expire_connections(true);
		}
};


class ConnectionHolder {
	protected:
		SockPool * pool;
		Connection ** connection;
	public:
		ConnectionHolder(SockPool *sockpool, Connection **conn, int serverid):pool(sockpool), connection(conn) {
			*connection = pool->connect(serverid);
			pool->expire_connections();
		}
		~ConnectionHolder() {
			if (*connection) { //*connection can be NULL if pool->connect failed.
				pool->release(*connection);
				*connection=NULL;
			}
		}
};

#endif
