/*
    termstuff.* - terminal control functions
    Copyright (C) 2001  Matthew Mueller <donut AT dakotacom.net>

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
#include "termstuff.h"

#include "log.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_WORKING_TERMSTUFF
#include <term.h>
#endif

typedef void voidfunc(void);

//Fake clr_bol function incase we can't use termcap.  Assumes that we will be
//writing another line of near the same length after clearing, thus we don't
//really need to clear the whole line, only the end of it.
void generic_clr_bol(void) {
	printf("\b\b\b\b    ");
}

voidfunc *clr_bol_func = generic_clr_bol;

#ifdef HAVE_WORKING_TERMSTUFF
void tputs_clr_bol(void) {
	if (tputs(clr_bol, 1, putchar)<0) {
		generic_clr_bol();
		PDEBUG(DEBUG_MIN, "tputs_clr_bol: error");
	}
}
#endif

void init_term_stuff(void) {
#ifdef HAVE_WORKING_TERMSTUFF
	char tbuf[1024];
	char *term = getenv("TERM");
	if (!term){
		PDEBUG(DEBUG_MIN, "init_term_stuff: TERM env not set");
		return;
	}
	if (tgetent(tbuf, term) != 1){
		PDEBUG(DEBUG_MIN, "init_term_stuff: tgetent failure");
//		err(2, "tgetent failure");
		return;
	}
	clr_bol_func = tputs_clr_bol;
	PDEBUG(DEBUG_MIN, "init_term_stuff: using tputs_clr_bol");
#else
	PDEBUG(DEBUG_MIN, "init_term_stuff: using generic_clr_bol");
#endif
}

void clear_line_and_return(void) {
	clr_bol_func();
	printf("\r");
}
