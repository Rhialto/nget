/*
    sockpool.* - socket connection pool
    Copyright (C) 1999-2002  Matthew Mueller <donut AT dakotacom.net>

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
		string bindaddr;
		c_file_tcp sock;
		c_server::ptr server;
		c_group_info::ptr curgroup;
		bool freshconnect;
		bool server_ok;


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
		Connection(c_server::ptr serv, const string &force_bindaddr) : lastuse(0), bindaddr(serv->get_bindaddr(force_bindaddr)), sock(serv->addr.c_str(), "nntp", bindaddr.c_str()), server(serv) {
			sock.initrbuf();
			freshconnect=true;
			server_ok=false;
			curgroup=NULL;
		}
};


typedef map<c_server::ptr, Connection *> t_connection_map;

class SockPool {
	protected:
		//t_connection_map used_connections;
		t_connection_map connections;

		void connection_erase(t_connection_map::iterator i);
	public:
		
		bool is_connected(const c_server::ptr &server) const {
			return (connections.find(server) != connections.end());
		}

//		int total_connections(void) const {
//			return free_connections.size() + used_connections.size();
//		}

		Connection* connect(const c_server::ptr &server, const string &bindaddr);
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
		ConnectionHolder(SockPool *sockpool, Connection **conn, const c_server::ptr &server, const string &bindaddr):pool(sockpool), connection(conn) {
			PDEBUG(DEBUG_MED, "aquiring connection to %s", server->alias.c_str());
			*connection = pool->connect(server, bindaddr);
			PDEBUG(DEBUG_MED, "aquired connection to %s (%p)", server->alias.c_str(), *connection);
			pool->expire_connections();
		}
		~ConnectionHolder() {
			PDEBUG(DEBUG_MED, "releasing connection to (%p)", *connection);
			assert(pool);
			assert(connection);
			if (*connection) { //*connection can be NULL if pool->connect failed.
				pool->release(*connection);
				*connection=NULL;
			}
		}
};

#endif
