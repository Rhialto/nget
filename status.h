/*
    status.h - status tracking stuff
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

#ifndef __NGET__STATUS_H__
#define __NGET__STATUS_H__
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

class FatalUserException { };

void set_decode_error_status(int incr=1);
void set_retrieve_error_status(int incr=1);
void set_group_error_status(int incr=1);
void set_grouplist_error_status(int incr=1);
void set_path_error_status(int incr=1);
void set_user_error_status(int incr=1);
void set_other_error_status(int incr=1);
void set_fatal_error_status(int incr=1);
void set_autopar_error_status(int incr=1);

void set_path_error_status_and_do_fatal_user_error(int incr=1);
void set_user_error_status_and_do_fatal_user_error(int incr=1);

void fatal_exit(void);
int get_exit_status(void);
int get_user_error_status(void);
void print_error_status(void);

void set_group_warn_status(int incr=1);
void set_xover_warn_status(int incr=1);
void set_retrieve_warn_status(int incr=1);
void set_undecoded_warn_status(int incr=1);
void set_unequal_line_count_warn_status(int incr=1);
void set_dupe_warn_status(int incr=1);
void set_cache_warn_status(int incr=1);
void set_grouplist_warn_status(int incr=1);
void set_autopar_warn_status(int incr=1);

void set_total_ok_status(int incr=1);
void set_yenc_ok_status(int incr=1);
void set_uu_ok_status(int incr=1);
void set_base64_ok_status(int incr=1);
void set_xx_ok_status(int incr=1);
void set_binhex_ok_status(int incr=1);
void set_plaintext_ok_status(int incr=1);
void set_qp_ok_status(int incr=1);
void set_unknown_ok_status(int incr=1);
void set_group_ok_status(int incr=1);
void set_dupe_ok_status(int incr=1);
void set_skipped_ok_status(int incr=1);
void set_grouplist_ok_status(int incr=1);
void set_autopar_ok_status(int incr=1);

#endif
