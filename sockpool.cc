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
#include "sockpool.h"
#include <errno.h>

int Connection::putline(int echo,const char * str,...){
	va_list ap;
	va_start(ap,str);
	int i=doputline(echo,str,ap);
	va_end(ap);
	return i;
}
int Connection::doputline(int echo,const char * str,va_list ap){
	char *fpbuf;
	int i;

	i=vasprintf(&fpbuf,str,ap);
	if (!(fpbuf=(char*)realloc(fpbuf,i+3))){
		free(fpbuf);
		close();
		throw TransportExError(Ex_INIT,"nntp_putline:realloc(%p,%i) %s(%i)",fpbuf,i+3,strerror(errno),errno);
	}
	if (echo)
		printf("%s << %s\n", server->shortname.c_str(), fpbuf);
	fpbuf[i]='\r';fpbuf[i+1]='\n';
	if (((i=sock.write(fpbuf,i+2))<=0)){
		free(fpbuf);
		close(1);
		throw TransportExError(Ex_INIT,"nntp_putline:%i %s(%i)",i,strerror(errno),errno);
	}
	free(fpbuf);
	return i;
}

int Connection::getline(int echo){
	int i=sock.bgets();
	if (i<0){//==0 can be legally achieved since the line terminators are removed
		close(1);
		throw TransportExError(Ex_INIT,"nntp_getline:%i %s(%i)",i,strerror(errno),errno);
	}else {
		if (echo)
			printf("%s >> %s\n", server->shortname.c_str(), sock.rbufp());
	}
	return i;
}


void SockPool::connection_erase(t_connection_map::iterator i) {
	try {
		i->second->close();
	} catch (baseCommEx &e) {//ignore transport errors while closing
		printCaughtEx_nnl(e);printf(" (ignored)\n");
	}
	delete i->second;
	connections.erase(i);
}

Connection* SockPool::connect(ulong serverid) {
	//use existing connection when possible
	t_connection_map::iterator i = connections.find(serverid);
	if (i!=connections.end()){
		if (i->second->isopen()) {
			i->second->touch();
			return i->second;
		} else {
			connection_erase(i);
		}
	}
	
	//check penalization before expire_old_connection
	nconfig.check_penalized(serverid); // (throws exception if penalized)
	
	//if we have to create a new one, don't go over max
	if (nconfig.maxconnections > 0 && (signed)connections.size() >= nconfig.maxconnections)
		expire_old_connection();
	
	//create new connection
	Connection *c = new Connection(nconfig.getserver(serverid));
	if (c->open()<0){
		nconfig.penalize(c->server);
		delete c;
		throw TransportExError(Ex_INIT,"make_connection:%s(%i)",strerror(errno),errno);
	}
	nconfig.unpenalize(c->server);
	connections.insert(t_connection_map::value_type(serverid, c));
	c->touch();
	return c;
}

void SockPool::release(Connection *connection) {
	assert(connection);
	if (connection->isopen())
		connection->touch();
	else
		connection_erase(connections.find(connection->server->serverid));
}

void SockPool::expire_old_connection(void) {
	t_connection_map::iterator i, oldest=connections.end();
	for (i=connections.begin(); i!=connections.end(); ++i) {
		Connection *c = i->second;
		if (!c->isopen()) {//if connection is closed, we can get rid of it right now.
			oldest = i;
			break;
		}
		if (oldest==connections.end() || c->age()>oldest->second->age())
			oldest = i;
	}
	if (oldest!=connections.end()){
		connection_erase(oldest);
	}else
		throw ApplicationExError(Ex_INIT, "no old connections to kill");
}

void SockPool::expire_connections(bool closeall) {
	t_connection_map::iterator i, p;
	i=connections.begin();
	while (i!=connections.end()) {
		Connection *c = i->second;
		p=i;
		++i;
		if (closeall || !c->isopen() || (c->age() >= c->server->idletimeout)) {
			connection_erase(p);
		}
	}
}
