/*
    prot_nntp.* - nntp protocol handler
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "prot_nntp.h"
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef PROTOTYPES
#define PROTOTYPES //required for uudeview.h to prototype function args.
#endif
#ifdef HAVE_UUDEVIEW_H
#include <uudeview.h>
#else
#include "uudeview.h"
#endif

#include "misc.h"
#include "termstuff.h"
#include "strreps.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "log.h"
#include "file.h"
#include "nget.h"
#include "strtoker.h"
#include "dupe_file.h"

extern SockPool sockpool;

int c_prot_nntp::putline(int echo,const char * str,...){
	va_list ap;
	va_start(ap,str);
	int i=connection->doputline(echo,str,ap);
	va_end(ap);
	return i;
}
int c_prot_nntp::stdputline(int echo,const char * str,...){
	va_list ap;
	int i;
	va_start(ap,str);
	connection->doputline(echo,str,ap);
	va_end(ap);
	i=getreply(echo);
	if (i==450 || i==480) {
		nntp_auth();
		va_start(ap,str);
		connection->doputline(echo,str,ap);
		va_end(ap);
		i=getreply(echo);
	}
	return i;
}

int c_prot_nntp::chkreply(int reply){
//	int i=getreply(echo);
	if (reply/100!=2)
		throw ProtocolExFatal(Ex_INIT,"bad reply %i: %s",reply,cbuf);
	return reply;
}

int c_prot_nntp::chkreply_setok(int reply){
	//only set the server_ok flag if the command was successful, or if it had a "normal" error status (like group not found, article expired, etc)
	if (reply/100==2 || reply/100==4)
		connection->server_ok = true;
	return chkreply(reply);
}

int c_prot_nntp::getline(int echo){
	int r = connection->getline(echo);
	cbuf = connection->sock.rbufp();
	return r;
}

int c_prot_nntp::getreply(int echo){
	int code;
	if ((code=getline(echo))>=0)
		code=atoi(cbuf);
	return code;
}

class XoverProgress {
	public:
		time_t lasttime, starttime, curt;
		void print_retrieving_headers(ulong low,ulong high,ulong done,ulong realtotal,ulong total,ulong bytes,int doneranges,int streamed,int totalranges){
			time(&lasttime);
			time_t dtime=lasttime-starttime;
			long Bps=(dtime>0)?bytes/dtime:0;
			long Bph=(done>0)?bytes/done:3;//if no headers have been retrieved yet, set the bytes per header to 3 just to get some sort of timeleft display.  (3=strlen(".\r\n"))
			if (!quiet) clear_line_and_return();
			printf("Retrieving headers %lu-%lu : %lu/%lu/%lu %3li%% %liB/s %s",low,high,done,realtotal,total,(realtotal!=0)?((done+(total-realtotal))*100/total):0,Bps,durationstr(realtotal==done?dtime:(Bps>0)?((realtotal-done)*Bph)/(Bps):0).c_str());
			if (totalranges>1)
				printf(" (%i/%i/%i)",doneranges,doneranges+streamed,totalranges);
			fflush(stdout);//@@@@
		};
		bool needupdate(void) {
			time(&curt);
			return !quiet && curt>lasttime;
		};
		XoverProgress(void) {
			lasttime = 0;
			starttime = time(NULL);
		};
};
/*
2019    Re: question    Katya Moon <MoonAngel@shadowrealm.com>  Wed, 17 Nov 1999 09:42:53 -0600 <xMwyOI+pJ=mVLqMociXeHexHGW92@4ax.com>      <3830CF31.D41E1511@tampabay.rr.com>     1145    9       Xref: rQdQ alt.chocobo:2019
2020    Re: well then!  Katya Moon <MoonAngel@shadowrealm.com>  Wed, 17 Nov 1999 09:44:37 -0600 <Fs0yOEVKaIamwKGBgE=82Fk21OcM@4ax.com>      <3831B98A.72815A01@tampabay.rr.com>     1209    10      Xref: rQdQ alt.chocobo:2020
2021    free me from this hideous thing!        Selah <sslanka@tampabay.rr.com> Wed, 17 Nov 1999 20:17:35 GMT   <38330E93.C8AC2671@tampabay.rr.com>         1142    8       Xref: rQdQ alt.chocobo:2021
.
0=articlenum
1=subject
2=author
3=date
4=message id
5=references (aka in-reply-to) (used for threading)
6=bytes
7=lines
... following are optional (and possibly different):
8=crossreferences

The sequence of fields must be in this order:
subject, author, date, message-id, references, byte count, and line count.
Other optional fields may follow line count. These fields are specified by
examining the response to the LIST OVERVIEW.FMT command. Where no data
exists, a null field must be provided
*/
void c_prot_nntp::doxover(c_nrange *r){
	if (r->empty())
		return;
	XoverProgress progress;
	ulong lowest=r->rlist.begin()->second, highest=r->rlist.rbegin()->first;
	ulong bytes=0, realnum=0, realtotal=r->get_total(), last;
	ulong total=realtotal;
	assert(connection);
	nntp_dogroup(0);

	t_rlist::iterator r_ri;
	t_rlist::iterator w_ri=r->rlist.begin();
	int streamed = 0, totalranges = r->num_ranges(), doneranges = 0;
	for (r_ri=r->rlist.begin();r_ri!=r->rlist.end();++r_ri, ++doneranges){
		if (progress.needupdate())
			progress.print_retrieving_headers(lowest,highest,realnum,realtotal,total,bytes,doneranges,streamed,totalranges);
		char *p;
		char *t[10];
		int i;
		while (w_ri!=r->rlist.end() && streamed<=connection->server->maxstreaming) { 
			ulong low=(*w_ri).second, high=(*w_ri).first;
			if (low==high)
				putline(debug>=DEBUG_MED,"XOVER %lu",low);//save a few bytes
			else
				putline(debug>=DEBUG_MED,"XOVER %lu-%lu",low,high);
			++w_ri;
			++streamed;
		}
		ulong low=(*r_ri).second, high=(*r_ri).first;
		last = low;
		chkreply_setok(getreply(debug>=DEBUG_MED));
		bytes+=strlen(cbuf)+2;//#### ugly.
		--streamed;
		unsigned long an=0;
		c_nntp_header nh;
		char * tp;
		do {
			bytes+=getline(debug>=DEBUG_ALL);
			if (cbuf[0]=='.')break;
			p=cbuf;
			for(i=0;i<10;i++){
				if((t[i]=goodstrtok(&p,'\t'))==NULL){
					break;
				}
				// fields 0, 6, 7 must all be numeric
				// otherwise restart
				if (i==0 || i==6 || i==7){
					tp=t[i];
					while (*tp){
						if (!isdigit(*tp) && !isspace(*tp))
							break;
						tp++;
					}
					// did the test finish, and/or was it a blank field?
					if (*tp && tp!=t[i]){
						// no - get out and read the next line
						// Is this how we want to handle it? SMW
						printf("error retrieving xover (%i non-numeric)\n",i);
						//printf("error retrieving xover (%i non-numeric): ",i);
						//for (int j=0;j<i;j++)
							//printf("%i:%s ",j,t[j]);
						//printf("*:%s\n",p);
						break;
					}
				}
			}
			if (i>7){
				//	c=new c_nntp_cache_item(atol(t[0]),	decode_textdate(t[3]), atol(t[6]), atol(t[7]),t[1],t[2]);
				//gcache->additem(c);
				an=atoul(t[0]);
				nh.set(t[1],t[2],an,decode_textdate(t[3]),atoul(t[6]),atoul(t[7]),t[4],t[5]);
				nh.serverid=connection->server->serverid;
				//gcache->additem(an, decode_textdate(t[3]), atol(t[6]), atol(t[7]),t[1],t[2]);
				gcache->additem(&nh);
				//delete nh;
				realnum++;
#ifndef NDEBUG
				ulong ort=realtotal;
#endif
				realtotal -= an - last;
				assert(ort>=realtotal);
				last = an + 1;
				if (progress.needupdate())
					progress.print_retrieving_headers(lowest,highest,realnum,realtotal,total,bytes,doneranges,streamed,totalranges);
			}else{
				printf("error retrieving xover (%i<=7): ",i);
				for (int j=0;j<i;j++)
					printf("%i:%s ",j,t[j]);
				printf("*:%s\n",p);
				//break;
				continue;
			}
		}while(1);
#ifndef NDEBUG
		ulong ort=realtotal;
#endif
		realtotal -= high - last + 1;
		assert(ort>=realtotal);
	}
	if(quiet<2 /*&& an*/){
		progress.print_retrieving_headers(lowest,highest,realnum,realtotal,total,bytes,doneranges,streamed,totalranges);
		printf("\n");
	}
}
void c_prot_nntp::doxover(ulong low, ulong high){
	c_nrange r;
	r.insert(low, high);
	doxover(&r);
}

void c_prot_nntp::nntp_dogroup(int getheaders){
	assert(connection);
	if (connection->curgroup!=group || getheaders){	
		int reply = stdputline(quiet<2,"GROUP %s",group->group.c_str());
		if (reply/100==4) // if group doesn't exist, set ok flag.  Otherwise let XOVER/ARTICLE reply set it. (If we always set it here, failure of xover/article would never result in penalization.  But if we only set it after xover/article, then a host could be incorrectly penalized just because it didn't have the group (eg, if maxconnections=1 so it closed connection before any other commands could succeed and setok))
			chkreply_setok(reply);
		else
			chkreply(reply);
		connection->curgroup=group;
	}
	if (getheaders){
		assert(gcache);
		char *p;
		ulong num,low,high;
		p=strchr(cbuf,' ')+1;
		num=atoul(p);
		p=strchr(p,' ')+1;
		low=atoul(p);
		p=strchr(p,' ')+1;
		high=atoul(p);
		//printf("%i, %i, %i\n",num,low,high);

		c_nntp_server_info* servinfo=gcache->getserverinfo(connection->server->serverid);
		assert(servinfo);
		if (low>servinfo->low)
			gcache->flushlow(servinfo,low,midinfo);
		if (connection->server->fullxover){
			c_nrange r;
			gcache->getxrange(servinfo,low,high,&r);
			doxover(&r);
		}else{
			c_nrange r;

			if (servinfo->high==0)
				r.insert(low, high);
			else {
				if (servinfo->high<high)
					r.insert(servinfo->high+1, high);
				if (servinfo->low>low)
					r.insert(low, servinfo->low-1);
			}
			doxover(&r);
		}
	}
};

void c_prot_nntp::nntp_group(c_group_info::ptr ngroup, int getheaders, const nget_options &options){
	if (group == ngroup && gcache && !getheaders)
		return; // group is already selected, don't waste time reloading it

	assert(ngroup);
	group=ngroup;
//	if (gcache) delete gcache;
	cleanupcache();

	midinfo=new c_mid_info((nghome + group->group + ",midinfo"));
	//gcache=new c_nntp_cache(nghome,group->group + ",cache");
	gcache=new c_nntp_cache(ngcachehome, group, midinfo);
	if (getheaders){
		c_server::ptr s;
		list<c_server::ptr> doservers;
		if (force_host){
			doservers.push_front(force_host);
		} else {
			t_server_list::iterator sli = nconfig.serv.begin();
			for (;sli!=nconfig.serv.end();++sli){
				s=(*sli).second;
				assert(s);
				if (group->priogrouping->getserverpriority(s->serverid) >= group->priogrouping->defglevel) {
					if (sockpool.is_connected(s->serverid)) //put current connected hosts at start of list
						doservers.push_front(s);
					else
						doservers.push_back(s);
				}
			}
		}
		int redone=0, succeeded=0, attempted=doservers.size();
		while (!doservers.empty() && redone<options.maxretry) {
			if (redone){
				printf("nntp_group: trying again. %i\n",redone);
				if (options.retrydelay)
					sleep(options.retrydelay);
			}
			list<c_server::ptr>::iterator dsi = doservers.begin();
			list<c_server::ptr>::iterator ds_erase_i;
			while (dsi != doservers.end()){
				s=(*dsi);
				assert(s);
				PDEBUG(DEBUG_MED,"nntp_group: serv(%lu) %f>=%f",s->serverid,group->priogrouping->getserverpriority(s->serverid),group->priogrouping->defglevel);
				try {
					ConnectionHolder holder(&sockpool, &connection, s->serverid);
					nntp_doopen();
					nntp_dogroup(getheaders);
					succeeded++;
				} catch (baseCommEx &e) {
					printCaughtEx(e);
					if (e.isfatal()) {
						printf("fatal error, won't try %s again\n", s->alias.c_str());
						//fall through to removing server from list below.
					}else{
						//if this is the last retry, don't say that we will try it again.
						if (redone+1 < options.maxretry)
							printf("will try %s again\n", s->alias.c_str());
						++dsi;
						continue;//don't remove server from list
					}
				}
				ds_erase_i = dsi;
				++dsi;
				doservers.erase(ds_erase_i);
			}
			redone++;
		}
		if (succeeded) {
			set_group_warn_status(attempted - succeeded);
			set_group_ok_status();
		}else {
			set_group_error_status();
			printf("no servers queried successfully\n");
		}
	}

	gcache_ismultiserver = gcache->ismultiserver();
}

inline void arinfo::print_retrieving_articles(time_t curtime, quinfo*tot){
	dtime=curtime-starttime;
	Bps=(dtime>0)?bytesdone/dtime:0;
	if (!quiet) clear_line_and_return();
	if (tot && tot->doarticle_show_multi!=NO_SHOW_MULTI)
		printf("%s ",server_name);
	printf("%lu (%i/%i): %li/%liL %li/%liB %3li%% %liB/s %s",
			anum,partnum,partreq,linesdone,linestot,bytesdone,bytestot,
			(linestot!=0)?(linesdone*100/linestot):0,Bps,
			durationstr((linesdone>=linestot)?dtime:((Bps>0)?(bytestot-bytesdone)/(Bps):-1)).c_str());
	if (tot)
		printf(" %li/%li %s",tot->filesdone,tot->filestot,
				//(tot->bytesdone>=tot->bytestot)?curtime-tot->starttime:((Bps>0)?(tot->bytestot-tot->bytesdone)/(Bps):-1));
//				(linesdone>=linestot)?curtime-tot->starttime:((Bps>0)?(tot->bytestot-tot->bytesdone)/(Bps):-1));
				durationstr((linesdone>=linestot)?curtime-tot->starttime:((Bps>0)?(tot->bytesleft-bytesdone)/(Bps):-1)).c_str());
	fflush(stdout);
}

//inline void c_prot_nntp::nntp_print_retrieving_articles(long nnn, long tot,long done,long btot,long bbb,unsigned long obtot,unsigned long obdone,long ototf,long odonef,time_t tstarttime){
//	time_t dtime=lasttime-starttime;
//	long Bps=(dtime>0)?bbb/dtime:0;
//	printf("\rRetrieving article %li: %li/%liL %li/%liB %3li%% %liB/s %lis %li/%li %lis",nnn,done,tot,bbb,btot,(tot!=0)?(done*100/tot):0,Bps,(done>=tot)?dtime:((Bps>0)?(btot-bbb)/(Bps):-1),odonef,ototf,(obdone>=obtot)?lasttime-tstarttime:((Bps>0)?(botot-obdone)/(Bps):-1));
//	fflush(stdout);//@@@@
//}

int c_prot_nntp::nntp_doarticle_prioritize(c_nntp_part *part,t_nntp_server_articles_prioritized &sap,bool docurservmult){
	t_nntp_server_articles::iterator sai;
	c_nntp_server_article *sa=NULL;
	float prio;
	for (sai = part->articles.begin(); sai != part->articles.end(); ++sai){
		sa=(*sai).second;
		assert(sa);
		if (force_host && sa->serverid!=force_host->serverid)
			continue;
		prio=group->priogrouping->getserverpriority(sa->serverid);
		if (docurservmult){
			if (sockpool.is_connected(sa->serverid))
				prio*=nconfig.curservmult;
		}
		PDEBUG(DEBUG_MED,"prioritizing server %lu article %lu prio %f",sa->serverid,sa->articlenum,prio);
		sap.insert(t_nntp_server_articles_prioritized::value_type(prio,sa));
	}

	if (docurservmult && !sap.empty()) {
		int connected=0, nonconnected=0;
		t_nntp_server_articles_prioritized::iterator i;
		pair<t_nntp_server_articles_prioritized::iterator,t_nntp_server_articles_prioritized::iterator> firstrange = sap.equal_range(sap.rend()->first);
		for (i=firstrange.first; i!=firstrange.second; ++i)
			if (sockpool.is_connected(i->second->serverid))
				++connected;
			else
				++nonconnected;

		if (connected && nonconnected) { //if both connected and nonconnected servers have the (same) highest priority, reprioritize the connected ones a bit higher to avoid excessive reconnecting.
			t_nntp_server_articles_prioritized::iterator ci;
			for (i=firstrange.first; i!=firstrange.second;){
				ci = i;
				++i;
				if (sockpool.is_connected(ci->second->serverid)) {
					prio=(*ci).first;
					sa=(*ci).second;
					sap.erase(ci);
					prio*=1.001;
					sap.insert(t_nntp_server_articles_prioritized::value_type(prio,sa));
					PDEBUG(DEBUG_MED,"server %lu article %lu reprioritized %f",sa->serverid,sa->articlenum,prio);
				}
			}
		}

	}
	return 0;
}


int c_prot_nntp::nntp_dowritelite_article(c_file &fw,c_nntp_part *part,char *fn){
	fw.putf("0\n%s\n%s\n",group->group.c_str(),fn);

	c_server::ptr whost;
	c_nntp_server_article *sa=NULL;
	t_nntp_server_articles_prioritized sap;
	t_nntp_server_articles_prioritized::iterator sapi;
	nntp_doarticle_prioritize(part,sap,false);
	fw.putf("%lu\n",(ulong)sap.size());
	for (sapi = sap.begin(); sapi != sap.end(); ++sapi){
		sa=(*sapi).second;
		whost=nconfig.getserver(sa->serverid);
		fw.putf("%s\t%s\t%s\n",whost->addr.c_str(),whost->user.c_str(),whost->pass.c_str());
		fw.putf("%lu\n%lu\n%lu\n",sa->articlenum,sa->bytes,sa->lines);
	}
	return 0;
}

void c_prot_nntp::nntp_dogetarticle(arinfo*ari,quinfo*toti,list<string> &buf){
	int header=1;
	time_t curt,lastt=0;
	char *lp;
	time(&ari->starttime);
	curt=starttime;
	long glr;
	do {
		glr=getline(debug>=DEBUG_ALL);
		if (cbuf[0]=='.'){
			if(cbuf[1]==0)
				break;
			lp=cbuf+1;
		}else
			lp=cbuf;
		ari->bytesdone+=glr;
		ari->linesdone++;
		if (header && lp[0]==0){
			//			printf("\ntoasted header statssssssss\n");
			header=0;
			ari->linesdone=0;
			time(&ari->starttime);//bytes=0;
		}
		time(&curt);
		if (!quiet && curt>lastt){
			//		nntp_print_retrieving_articles(num,ltotal,lines,btotal,bytes);
			ari->print_retrieving_articles(curt,toti);
			lastt=curt;
		}
		buf.push_back(lp);
	}while(1);
	if (quiet<2){
		//nntp_print_retrieving_articles(num,ltotal,lines,btotal,bytes);
		ari->print_retrieving_articles(curt,toti);
		printf("\n");
	}

	//some servers report # of bytes a bit off of what we expect.
	if (!((ari->bytesdone <= ari->bytestot+3 && ari->bytesdone >= ari->bytestot-3) ||
			//some servers also report # of bytes counted with LF endings, then send with CRLF
			(ari->bytesdone <= ari->bytestot+3+ari->linesdone || ari->bytesdone >= ari->bytestot-3+ari->linesdone)) ||
			ari->linesdone!=ari->linestot){
		printf("doarticle %lu: %lu!=%lu || %lu!=%lu\n",ari->anum,ari->bytesdone,ari->bytestot,ari->linesdone,ari->linestot);
	}
	c_server::ptr host = connection->server;
	if (!(ari->linesdone>=ari->linestot+host->lineleniencelow && ari->linesdone<=ari->linestot+host->lineleniencehigh)){
		printf("unequal line count %lu should equal %lu",ari->linesdone,ari->linestot);
		if (host->lineleniencelow||host->lineleniencehigh){
			if (host->lineleniencelow==-host->lineleniencehigh)
				printf("(+/- %i)",host->lineleniencehigh);
			else
				printf("(%+i/%+i)",host->lineleniencelow,host->lineleniencehigh);
		}
		printf("\n");
		if (nconfig.unequal_line_error)
			throw TransportExFatal(Ex_INIT, "unequal line count and unequal_line_error is true");
		set_unequal_line_count_warn_status();
	}
}

int c_prot_nntp::nntp_doarticle(c_nntp_part *part,arinfo*ari,quinfo*toti,char *fn, const nget_options &options){
	c_nntp_server_article *sa=NULL;
	t_nntp_server_articles_prioritized sap;
	t_nntp_server_articles_prioritized::iterator sapi;
	t_nntp_server_articles_prioritized::iterator sap_erase_i;
	nntp_doarticle_prioritize(part,sap,true);
	int redone=0, attempted=sap.size();
	while (!sap.empty() && redone<options.maxretry) {
		if (redone){
			printf("nntp_doarticle: trying again. %i\n",redone);
			if (options.retrydelay)
				sleep(options.retrydelay);
		}
		for (sapi = sap.begin(); sapi != sap.end();){
			sa=(*sapi).second;
			assert(sa);
			ari->partnum=part->partnum;
			ari->anum=sa->articlenum;
			ari->bytestot=sa->bytes;
			ari->linestot=sa->lines;
			ari->linesdone=0;
			ari->bytesdone=0;
			PDEBUG(DEBUG_MED,"trying server %lu article %lu",sa->serverid,sa->articlenum);
			list<string> buf;//use a list of strings instead of char *.  Easier and it cleans up after itself too.
			try {
				ConnectionHolder holder(&sockpool, &connection, sa->serverid);
				nntp_doopen();
				if (toti->doarticle_show_multi==SHOW_MULTI_SHORT)
					ari->server_name=connection->server->shortname.c_str();
				else if (toti->doarticle_show_multi==SHOW_MULTI_LONG)
					ari->server_name=connection->server->alias.c_str();
				nntp_dogroup(0);
				chkreply_setok(stdputline(debug>=DEBUG_MED,"ARTICLE %lu",sa->articlenum));
				nntp_dogetarticle(ari,toti,buf);
			} catch (baseCommEx &e) {
				printCaughtEx(e);
				if (e.isfatal()) {
					printf("fatal error, won't try %s again\n", nconfig.getservername(sa->serverid));
					sap_erase_i = sapi;
					++sapi;
					sap.erase(sap_erase_i);
				}else{
					//if this is the last retry, don't say that we will try it again.
					if (redone+1 < options.maxretry)
						printf("will try %s again\n", nconfig.getservername(sa->serverid));
					++sapi;
				}
				continue;
			}
			c_file_fd f(fn, O_CREAT|O_WRONLY|O_TRUNC, PRIVMODE);
			list<string>::iterator curb;
			try {
				for(curb = buf.begin();curb!=buf.end();++curb){
					f.putf("%s\n",(*curb).c_str());
				}
				f.close();
			}catch(FileEx &e){
				//if the drive is full or other error occurs, then the temp file will be cutoff and useless, so delete it.
				if (unlink(fn))
					perror("unlink:");
				throw;
			}
			set_retrieve_warn_status(attempted - sap.size());
			return 0; //article successfully retrieved, return.
		}
		redone++;
	}
	printf("couldn't get %s from anywhere\n",part->messageid.c_str());
	set_retrieve_error_status();
	return -1;
}

int uu_info_callback(void *v,char *i){
	printf("info:%s",i);
	return 0;
};
struct uu_err_status {
	int derr;
	int last_t;
//	string last_m;
	uu_err_status(void):derr(0),last_t(-1){}
};
void uu_msg_callback(void *v,char *m,int t){
	if (t!=UUMSG_MESSAGE) {//######
		uu_err_status *es = (uu_err_status *)v;
		es->derr++;
		es->last_t = t;
//		es->last_m = m;
		//++(*(int *)v);
	}
	if (quiet>=2 && (t==UUMSG_MESSAGE || t==UUMSG_NOTE))
		return;
	printf("uu_msg(%i):%s\n",t,m);
};

int uu_busy_callback(void *v,uuprogress *p){
//	if (!quiet) printf("uu_busy(%i):%s %i%%\r",p->action,p->curfile,(100*p->partno-p->percent)/p->numparts);
	if (!quiet) {printf(".");fflush(stdout);}
	return 0;
};
int fname_filter(char *buf, const char *path, char *fn){
	char *fnp=fn;
	int slen=strlen(fnp);
	if (*fnp=='"') {
		fnp++;
		slen--;
	}
	if (fnp[slen-1]=='"')
		slen--;
	if (path)
		return sprintf(buf,"%s/%.*s",path,slen,fnp);
	else
		return sprintf(buf,"%.*s",slen,fnp);
}
char * uu_fname_filter(void *v,char *fn){
	const string *s=(const string *)v;
	static char buf[PATH_MAX];
	fname_filter(buf,s->c_str(),fn);
	PDEBUG(DEBUG_MED,"uu_fname_filter: filtered %s to %s",fn,buf);
	return buf;
}

void print_nntp_file_info(c_nntp_file::ptr f, t_show_multiserver show_multi) {
	char tconvbuf[TCONV_DEF_BUF_LEN];
	c_nntp_part *p=(*f->parts.begin()).second;
	tconv(tconvbuf,TCONV_DEF_BUF_LEN,&p->date);
	if (f->iscomplete())
		printf("%i",f->have);
	else
		printf("%i/%i",f->have,f->req);
	if (show_multi!=NO_SHOW_MULTI){
		t_server_have_map have_map;
		f->get_server_have_map(have_map);
		if (show_multi==SHOW_MULTI_SHORT)
			printf(" ");
		
		for (t_server_have_map::iterator i=have_map.begin(); i!=have_map.end(); ++i){
			c_server::ptr s=nconfig.getserver(i->first);
			if (show_multi==SHOW_MULTI_LONG){
				printf(" %s", s->alias.c_str());
				if (i->second<f->have)
					printf(":%i", i->second);
			}
			else{
				if (i->second<f->have){
					for (string::size_type j=0; j<s->shortname.size(); j++)
						printf("%c", toupper(s->shortname[j]));
				}
				else
					printf("%s", s->shortname.c_str());
			}
		}
	}
	printf("\t%lil\t%s\t%s\t%s\t%s\n",f->lines(),tconvbuf,f->subject.c_str(),f->author.c_str(),p->messageid.c_str());
}

char * make_text_file_name(c_nntp_file_retr::ptr fr, bool usepath=0) {
	char *nfn;
	//asprintf(&nfn,"%lu.txt",f->banum());//give it a (somewhat) more sensible name
	//give it a (somewhat) more sensible name //TODO: make it better (rand? blah.. message id or something but it might contain bad chars)
	if (usepath)
		asprintf(&nfn,"%s/%lu.%i.txt",fr->path.c_str(),fr->file->badate(),rand());
	else
		asprintf(&nfn,"%lu.%i.txt",fr->file->badate(),rand());
	return nfn;
}

void c_prot_nntp::nntp_retrieve(c_group_info::ptr rgroup, const t_nntp_getinfo_list &getinfos, const nget_options &options){
	c_nntp_files_u filec;
	assert(rgroup);
	if (gcache) {
		assert(group == rgroup);
		gcache->getfiles(&filec, midinfo, getinfos);
	} else {
		if (rgroup != group) {
			cleanupcache();
			group = rgroup;
		}
		if (!midinfo) {
			midinfo=new c_mid_info((nghome + group->group + ",midinfo"));
		}
		
		nntp_cache_getfiles(&filec, &gcache_ismultiserver, ngcachehome, group, midinfo, getinfos);
	}
	if (filec.files.empty())
		return;

	int optionflags = options.gflags;
	t_nntp_files_u::iterator curf;
	//c_nntp_file *f;
	c_nntp_file::ptr f;
	c_nntp_file_retr::ptr fr;

	//hmm, maybe not now. //well, now we need to keep it around again, so we can set the read flag.  At least with the new cache implementation its not quite such a memory hog.
//	if (gcache){
//		gcache->dec_rcount();/*delete gcache;*/gcache=NULL;//we don't need this anymore, so free up some (a lot) of mem
		gcache=NULL;
//	}

	if (optionflags & GETFILES_UNMARK) {
		ulong nbytes=0;
		unsigned int nfiles=0;
		for(curf = filec.files.begin();curf!=filec.files.end();++curf){
			fr=(*curf).second;
			f=fr->file;
			if (optionflags & GETFILES_TESTMODE) {
				if (midinfo->check(f->bamid())){
					print_nntp_file_info(f,options.test_multi);
					nbytes+=f->bytes();
					nfiles++;
				}
			} else
				midinfo->remove(f->bamid());
		}
		if (optionflags & GETFILES_TESTMODE)
			printf("Would unmark %lu bytes in %u files\n",nbytes,nfiles);
	} else if (optionflags & GETFILES_TESTMODE){
		for(curf = filec.files.begin();curf!=filec.files.end();++curf){
			fr=(*curf).second;
			print_nntp_file_info(fr->file,options.test_multi);
		}
		if (optionflags & GETFILES_MARK)
			printf("Would mark ");
		printf("%"PRIuFAST64" bytes in %lu files\n",filec.bytes,(ulong)filec.files.size());
	} else if (optionflags & GETFILES_MARK) {
		for(curf = filec.files.begin();curf!=filec.files.end();++curf){
			fr=(*curf).second;
			f=fr->file;
			midinfo->insert(f->bamid());
		}
	} else {
		t_id group_id;
# ifdef CHECKSUM
		group_id=CHECKSUM(0L, Z_NULL, 0);
		//group_id=CHECKSUM(group_id,host.c_str(),host.size());
		group_id=CHECKSUM(group_id,(Byte*)group->group.c_str(),group->group.size());
# else
		hash<const char*> H;
		group_id=H(group->group.c_str());
# endif
		quinfo qtotinfo;
		arinfo ainfo;
		time(&qtotinfo.starttime);
		qtotinfo.filesdone=0;
//		qtotinfo.linestot=filec.lines;
		qtotinfo.filestot=filec.files.size();
		qtotinfo.bytesleft=filec.bytes;
		qtotinfo.doarticle_show_multi=gcache_ismultiserver?SHOW_MULTI_SHORT:NO_SHOW_MULTI;
		c_nntp_part *p;
//		s_part_u *bp;
		t_nntp_file_parts::iterator curp;
		t_nntp_files_u::iterator lastf=filec.files.end();
		char *fn;
		if (!options.writelite.empty())
			optionflags |= GETFILES_NODECODE;
		for(curf = filec.files.begin();curf!=filec.files.end();++curf){
			int r;
			if (lastf!=filec.files.end()){
//				delete (*lastf).second;//new cache implementation uses pointers to the same data
				filec.files.erase(lastf);
				qtotinfo.filesdone++;
				filec.bytes=qtotinfo.bytesleft;//update bytes in case we have an exception and need to restart.
			}
			lastf=curf;
			fr=(*curf).second;
			f=fr->file;
			printf("Retrieving: ");
			print_nntp_file_info(f, options.retr_show_multi);
//			bp=f->parts.begin()->second;
			list<char *> fnbuf;
			list<char *>::iterator fncurb;
//			int derr=0;
			uu_err_status uustatus;
			int un=0;
			for(curp = f->parts.begin();curp!=f->parts.end();++curp){
				//asprintf(&fn,"%s/%s-%s-%li-%li-%li",nghome.c_str(),host.c_str(),group.c_str(),fgnum,part,num);
				p=(*curp).second;
				if (uustatus.derr){
					qtotinfo.bytesleft-=p->bytes();
					continue;
				}
				//asprintf(&fn,"./%lx-%lx.%03li-%li",group_id,f->sub_id,(*curp).first,p->anum);
				//asprintf(&fn,"./%lx-%lx.%03li",group_id,H(f->subject.c_str()),(*curp).first);
				{
					const char *usepath;
					if (!options.writelite.empty())
						usepath="";
					else usepath=fr->temppath.c_str();
					if (optionflags & GETFILES_TEMPSHORTNAMES)
						asprintf(&fn,"%s%lx.%03i",usepath,group_id+f->fileid,(*curp).first);
					else
						asprintf(&fn,"%s%lx-%lx.%03i",usepath,group_id,f->fileid,(*curp).first);
				}
				if (!fexists(fn)){
					ainfo.partreq = f->req;
//					ainfo.anum=p->articlenum;//set in doarticle now.
//					ainfo.linesdone=0;
//					ainfo.bytesdone=0;
//					ainfo.linestot=p->lines;
//					ainfo.bytestot=p->bytes;
					if (!options.writelite.empty()){
						c_file_fd fw(options.writelite.c_str(), O_WRONLY|O_CREAT|O_APPEND, PRIVMODE);
						nntp_dowritelite_article(fw,p,fn);
						fw.close();
						free(fn);
						qtotinfo.bytesleft-=p->bytes();
//						uustatus.derr=-1;//skip this file..
						continue;
					}
					if ((optionflags & GETFILES_NOCONNECT) || nntp_doarticle(p,&ainfo,&qtotinfo,fn,options)){
						free(fn);
						fn=NULL;
						if (!(optionflags & GETFILES_GETINCOMPLETE)) {
							qtotinfo.bytesleft-=p->bytes();
							uustatus.derr=-1;//skip this file..
							continue;
						}
					}
				}else{
//					qtotinfo.bytestot-=p->bytes;
					if (quiet<2) printf("already have article %s (%s)\n",p->messageid.c_str(),fn);
				}
				qtotinfo.bytesleft-=p->bytes();
//				UULoadFile(fn,NULL,0);//load once they are all d/l'd
				//				delete fn;
				if (fn)
					fnbuf.push_back(fn);
			}
			if (!uustatus.derr && !(optionflags&GETFILES_NODECODE)){
				if ((r=UUInitialize())!=UURET_OK)
					throw ApplicationExFatal(Ex_INIT,"UUInitialize: %s",UUstrerror(r));
				UUSetOption(UUOPT_DUMBNESS,1,NULL);//we already put the parts in the correct order, so it doesn't need to.
				//UUSetOption(UUOPT_FAST,1,NULL);//we store each message in a seperate file
				//actually, I guess that won't work, since some messages have multiple files in them anyway.
				UUSetOption(UUOPT_OVERWRITE,0,NULL);//no thanks.
				UUSetOption(UUOPT_USETEXT,1,NULL);//######hmmm...
				UUSetMsgCallback(&uustatus,uu_msg_callback);
				UUSetBusyCallback(NULL,uu_busy_callback,1000);
				UUSetFNameFilter(&fr->path,uu_fname_filter);
				for(fncurb = fnbuf.begin();fncurb!=fnbuf.end();++fncurb){
					UULoadFile((*fncurb),NULL,0);
				}
				uulist * uul;
				dupe_file_checker flist;
				for (un=0;;un++){
					if ((uul=UUGetFileListItem(un))==NULL)break;
					if (!(uul->state & UUFILE_OK)){
						printf("%s not ok\n",uul->filename);
						uustatus.derr++;
						continue;
					}
					//				printf("\ns:%x d:%x\n",uul->state,uul->uudet);
					if (/*uul->uudet==PT_ENCODED &&*/ strcmp(uul->filename,"0001.txt")==0){
						char *nfn = make_text_file_name(fr);
						UURenameFile(uul,nfn);
						free(nfn);
					}else
						r=UUInfoFile(uul,NULL,uu_info_callback);
					r=UUDecodeFile(uul,NULL);
					if (r==UURET_EXISTS){
						uustatus.derr--; //HACK since this error will also cause a uu_warning thingy, which will incr derr, but we don't want that.
						//all the following ugliness with fname_filter is due to uulib forgetting that we already filtered the name and giving us the original name instead.
						char nfn[PATH_MAX];
						sprintf(nfn+fname_filter(nfn, NULL, uul->filename), ".%lu.%i", f->badate(),rand());
#ifdef	USE_FILECOMPARE	// We have a duplicate file name
						// memorize the old file name
						char old_fnp[PATH_MAX];
						char *nfnp;
						fname_filter(old_fnp, fr->path.c_str(), uul->filename);
						// Generate a new filename to use
						asprintf(&nfnp,"%s/%s",fr->path.c_str(),nfn);
						UURenameFile(uul,nfn);
						r=UUDecodeFile(uul,NULL);
						// did it decode correctly?
						if (r == UURET_OK){
							// if identical, delete the one we just downloaded
							if (filecompare(old_fnp,nfnp)){
								printf("Duplicate File Removed %s\n", nfn);
								unlink(nfnp);
								set_dupe_ok_status();
							}else
								set_dupe_warn_status();
						}
						// cleanup
						free(nfnp);
#else	/* USE_FILECOMPARE */				// the orginal code
						UURenameFile(uul,nfn);
						r=UUDecodeFile(uul,NULL);
						set_dupe_warn_status();
#endif	/* USE_FILECOMPARE */
					}
					if (r!=UURET_OK){
						uustatus.derr++;
						printf("decode(%s): %s\n",uul->filename,UUstrerror(r));
						continue;
					}else{
						if (!(optionflags&GETFILES_NODUPEFILECHECK))
							flist.addfile(fr->path, uul->filename); //#### is this the right place? what about dupes saved as different names??
						switch (uul->uudet){
							case YENC_ENCODED:set_yenc_ok_status();break;
							case UU_ENCODED:set_uu_ok_status();break;
							case B64ENCODED:set_base64_ok_status();break;
							case XX_ENCODED:set_xx_ok_status();break;
							case BH_ENCODED:set_binhex_ok_status();break;
							case PT_ENCODED:set_plaintext_ok_status();break;
							case QP_ENCODED:set_qp_ok_status();break;
							default:set_unknown_ok_status();
						}
					}
				}
				UUCleanUp();
				//handle posts that uulib says "no encoded data" for as text. (seems to be posts with no body)
				if (uustatus.derr==1 && uustatus.last_t==UUMSG_NOTE && un==0 && f->req<=0 && fnbuf.size()==1) {
						uustatus.derr--; //HACK since this error will also cause a uu_note "No encoded data found", which will incr derr, but we don't want that.
						char *fn = *(fnbuf.begin());
						char *nfn = make_text_file_name(fr,true);
						while (fexists(nfn))  {
							free(nfn);
							nfn = make_text_file_name(fr,true);
						}
						xxrename(fn, nfn);
						free(nfn);
						free(fn);
						fnbuf.clear();
						set_plaintext_ok_status();
						un++;
				}

				//check all remaining files against what we just decoded, and remove any dupes.
				if (!uustatus.derr && !flist.empty()) {
					t_nntp_files_u::iterator dfi = curf; ++dfi; //skip the current one
					t_nntp_files_u::iterator del_fi;
					c_nntp_file_retr::ptr dfr;
					while(dfi!=filec.files.end()){
						dfr = (*dfi).second;
						//only check files that are being downloaded to the same path
						if (dfr->path == fr->path) {
							c_nntp_file::ptr df = dfr->file;
							if (flist.checkhavefile(df->subject.c_str(), df->bamid(), df->bytes())) {
								set_skipped_ok_status();
								del_fi=dfi;
								++dfi;
								filec.files.erase(del_fi);
								qtotinfo.filestot--;
								qtotinfo.bytesleft-=df->bytes();
								continue;
							}
						}
						++dfi;
					}
				}
			}
			if (uustatus.derr>0){
				set_decode_error_status();
				printf(" %i decoding errors occured, keeping temp files.\n",uustatus.derr);
			}else if (uustatus.derr<0) 
				printf("download error occured, keeping temp files.\n");
			else if (un==0){
				set_undecoded_warn_status();
				printf("hm.. nothing decoded.. keeping temp files\n");
				uustatus.derr=-2;
			}else if (uustatus.derr==0){
				set_total_ok_status();
				if (optionflags&GETFILES_NODECODE){
					if (quiet<2)
						printf("not decoding, keeping temp files.\n");
					uustatus.derr=1;
				}else  {
					midinfo->insert(f->bamid());
					if (optionflags&GETFILES_KEEPTEMP){
						if (quiet<2)
							printf("decoded ok, keeping temp files.\n");
						uustatus.derr=1;
					}else if (quiet<2)
						printf("decoded ok, deleting temp files.\n");
				}
			}
			char *p;
			for(fncurb = fnbuf.begin();fncurb!=fnbuf.end();++fncurb){
				p=(*fncurb);
				if (!uustatus.derr)
					unlink(p);
				free(p);
			}
		}
	}
	//delete filec;filec=NULL;
	cleanupcache();
}
void c_prot_nntp::nntp_auth(void){
	nntp_doauth(connection->server->user.c_str(),connection->server->pass.c_str());
}
void c_prot_nntp::nntp_doauth(const char *user, const char *pass){
	int i;

	if(!user || !*user){
		throw TransportExFatal(Ex_INIT,"nntp_doauth: no authorization info known");
	}
	putline(quiet<2,"AUTHINFO USER %s",user);
	i=getreply(quiet<2);
	if (i==350 || i==381){
		if(!pass || !*pass){
			throw TransportExFatal(Ex_INIT,"nntp_doauth: no password known");
		}
		if (quiet<2)
			printf("%s << AUTHINFO PASS *\n", connection->server->shortname.c_str());
		putline(0,"AUTHINFO PASS %s",pass);
		i=getreply(quiet<2);
	}
	chkreply(i);
}

void c_prot_nntp::nntp_open(c_server::ptr h){
	if (h)
		force_host=h;
	else
		force_host=NULL;
}

void c_prot_nntp::nntp_doopen(void){
	assert(connection);
	if (connection->freshconnect){
		chkreply(getreply(quiet<2));
		putline(debug>=DEBUG_MED,"MODE READER");
		getline(debug>=DEBUG_MED);
		connection->freshconnect=false;
	}
}

void c_prot_nntp::cleanupcache(void){
//	if(gcache){gcache->dec_rcount();/*delete gcache;*/gcache=NULL;}
	gcache=NULL;//ref counted.
	if (midinfo){
		c_mid_info *mi = midinfo; //store midinfo in temp pointer and NULL out real pointer, to prevent a second deletion attempt if the destructor aborts and the atexit calls the cleanup again.
		midinfo=NULL;
		delete mi;
	}
}
void c_prot_nntp::cleanup(void){
	cleanupcache();
}

void c_prot_nntp::initready(void){
//	midinfo=new c_mid_info((nghome + ".midinfo"));
}
c_prot_nntp::c_prot_nntp(void){
//	cbuf=new char[4096];
//	cbuf_size=4096;
	gcache=NULL;
//	ch=-1;
	connection=NULL;
	midinfo=NULL;
	force_host=NULL;
}
c_prot_nntp::~c_prot_nntp(void){
//	printf("nntp destructing\n");
//	if (midinfo)delete midinfo;
	cleanup();
}
