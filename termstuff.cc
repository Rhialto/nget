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

#ifdef HAVE_NETBSD_TERMCAP
#include <termcap.h>
static struct tinfo *info;
static char *clr_bol;
static void my_putchar(char c, void *d) { putchar((int)c); }
#elif defined(HAVE_WORKING_TERMSTUFF)
#include <term.h>
#endif

typedef void voidfunc(void);

//Fake clr_bol function incase we can't use termcap.  Assumes that we will be
//writing another line of near the same length after clearing, thus we don't
//really need to clear the whole line, only the end of it.
static void generic_clr_bol(void) {
	printf("\b\b\b\b    ");
}

static voidfunc *clr_bol_func = generic_clr_bol;

#if defined(HAVE_WORKING_TERMSTUFF) || defined(HAVE_NETBSD_TERMCAP)
static void tputs_clr_bol(void) {
# ifdef HAVE_NETBSD_TERMCAP
	if (t_puts(info, clr_bol, 1, my_putchar, NULL)<0)
# else
	if (tputs(clr_bol, 1, putchar)<0)
# endif 
	{
		generic_clr_bol();
		PDEBUG(DEBUG_MIN, "tputs_clr_bol: error");
	}
}
#endif

void init_term_stuff(void) {
#if defined(HAVE_WORKING_TERMSTUFF) || defined(HAVE_NETBSD_TERMCAP)
# if defined(HAVE_WORKING_TERMSTUFF)
	char tbuf[1024];
# endif
	char *term = getenv("TERM");
	if (!term){
		PDEBUG(DEBUG_MIN, "init_term_stuff: TERM env not set");
		return;
	}
# ifdef HAVE_NETBSD_TERMCAP
	if (t_getent(&info, term) != 1) {
		PDEBUG(DEBUG_MIN, "init_term_stuff: t_getent failure");
		return;
	}
	if (!(clr_bol = t_agetstr(info, "ce"))) {
		PDEBUG(DEBUG_MIN, "init_term_stuff: t_agetstr failure");
		return;
	}
# else
	if (tgetent(tbuf, term) != 1) {
		PDEBUG(DEBUG_MIN, "init_term_stuff: tgetent failure");
		return;
	}
# endif
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
