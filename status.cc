/*
    status.cc - status tracking stuff
    Copyright (C) 2001-2003  Matthew Mueller <donut AT dakotacom.net>

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
#include "status.h"
#include "server.h"
#include <stdlib.h>
#include <stdio.h>

static int errorflags=0, warnflags=0, okflags=0;

int get_exit_status(void) {
	return errorflags;
}

void fatal_exit(void) {
	set_fatal_error_status();
	exit(get_exit_status());
}

#define SET_x_x_STATUS(type, low, up, bit) static const int type ## _ ## up = bit; \
static int low ## _ ## type;\
void set_ ## type ## _ ## low ## _status(int incr){\
	low ## _ ## type += incr;\
	if (incr>0)	low ## flags|=type ## _ ## up;\
}

#define SET_x_ERROR_STATUS(type,bit) SET_x_x_STATUS(type, error, ERROR, bit)
#define SET_x_WARN_STATUS(type,bit) SET_x_x_STATUS(type, warn, WARN, bit)
#define SET_x_OK_STATUS(type,bit) SET_x_x_STATUS(type, ok, OK, bit)
SET_x_ERROR_STATUS(decode, 1);
SET_x_ERROR_STATUS(autopar, 2);
SET_x_ERROR_STATUS(path, 4);
SET_x_ERROR_STATUS(user, 4);
SET_x_ERROR_STATUS(retrieve, 8);
SET_x_ERROR_STATUS(group, 16);
SET_x_ERROR_STATUS(grouplist, 32);
SET_x_ERROR_STATUS(other, 64);
SET_x_ERROR_STATUS(fatal, 128);
SET_x_OK_STATUS(total, 1);
SET_x_OK_STATUS(uu, 2);
SET_x_OK_STATUS(base64, 4);
SET_x_OK_STATUS(xx, 8);
SET_x_OK_STATUS(binhex, 16);
SET_x_OK_STATUS(plaintext, 32);
SET_x_OK_STATUS(qp, 64);
SET_x_OK_STATUS(yenc, 128);
SET_x_OK_STATUS(dupe, 256);
SET_x_OK_STATUS(unknown, 512);
SET_x_OK_STATUS(group, 1024);
SET_x_OK_STATUS(skipped, 2048);
SET_x_OK_STATUS(grouplist, 4096);
SET_x_OK_STATUS(autopar, 8192);
SET_x_WARN_STATUS(retrieve,1);
SET_x_WARN_STATUS(undecoded,2);
SET_x_WARN_STATUS(unequal_line_count,8);
SET_x_WARN_STATUS(dupe, 256);
SET_x_WARN_STATUS(group,1024);
SET_x_WARN_STATUS(cache,2048);
SET_x_WARN_STATUS(grouplist, 4096);
SET_x_WARN_STATUS(xover,8192);
SET_x_WARN_STATUS(autopar, 16384);
#define print_x_x_STATUS(type, low) if (low ## _ ## type) printf("%s %i " #type, cf++?",":"", low ## _ ## type)
#define print_x_ERROR_STATUS(type) print_x_x_STATUS(type, error)
#define print_x_WARN_STATUS(type) print_x_x_STATUS(type, warn)
#define print_x_OK_STATUS(type) print_x_x_STATUS(type, ok)
void print_error_status(void){
	int pf=0;
	if (okflags){
		int cf=0;
		pf++;
		printf("OK:");
		print_x_OK_STATUS(total);
		print_x_OK_STATUS(yenc);
		print_x_OK_STATUS(uu);
		print_x_OK_STATUS(base64);
		print_x_OK_STATUS(xx);
		print_x_OK_STATUS(binhex);
		print_x_OK_STATUS(plaintext);
		print_x_OK_STATUS(qp);
		print_x_OK_STATUS(unknown);
		print_x_OK_STATUS(group);
		print_x_OK_STATUS(grouplist);
		print_x_OK_STATUS(dupe);
		print_x_OK_STATUS(skipped);
		print_x_OK_STATUS(autopar);
	}
	if (warnflags){
		int cf=0;
		if (pf++) printf(" ");
		printf("WARNINGS:");
		print_x_WARN_STATUS(group);
		print_x_WARN_STATUS(xover);
		print_x_WARN_STATUS(retrieve);
		print_x_WARN_STATUS(undecoded);
		print_x_WARN_STATUS(unequal_line_count);
		print_x_WARN_STATUS(dupe);
		print_x_WARN_STATUS(cache);
		print_x_WARN_STATUS(grouplist);
		print_x_WARN_STATUS(autopar);
	}
	if (errorflags){
		int cf=0;
		if (pf++) printf(" ");
		printf("ERRORS:");
		print_x_ERROR_STATUS(decode);
		print_x_ERROR_STATUS(path);
		print_x_ERROR_STATUS(user);
		print_x_ERROR_STATUS(retrieve);
		print_x_ERROR_STATUS(group);
		print_x_ERROR_STATUS(grouplist);
		print_x_ERROR_STATUS(autopar);
		print_x_ERROR_STATUS(other);
		print_x_ERROR_STATUS(fatal);
	}
	if (pf)
		printf("\n");
}

void set_path_error_status_and_do_fatal_user_error(int incr) {
	set_path_error_status();
	if (nconfig.fatal_user_errors)
		throw FatalUserException();
}
void set_user_error_status_and_do_fatal_user_error(int incr) {
	set_user_error_status();
	if (nconfig.fatal_user_errors)
		throw FatalUserException();
}

int get_user_error_status(void){
	return error_user;
}
