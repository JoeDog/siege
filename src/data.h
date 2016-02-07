/**
 * Storage for siege data
 *
 * Copyright (C) 2000-2014 by
 * Jeffrey Fulmer - <jeff@joedog.org>, et al.
 * This file is distributed as part of Siege
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *--
 */
#ifndef __DATA_H
#define __DATA_H

#include <stdio.h>
#include <unistd.h>

#ifdef HAVE_SYS_TIMES_H
# include <sys/times.h>
#endif/*HAVE_SYS_TIMES_H*/

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif/*HAVE_SYS_TIME_H   */
#endif /*TIME_WITH_SYS_TIME*/

typedef struct DATA_T *DATA;

/* constructor */
DATA  new_data();
DATA  data_destroy(DATA this);

/* setters */
void  data_set_start      (DATA this);
void  data_set_stop       (DATA this);
void  data_set_highest    (DATA this, float highest);
void  data_set_lowest     (DATA this, float lowest);
void  data_increment_bytes(DATA this, unsigned long bytes);
void  data_increment_count(DATA this, unsigned long count);
void  data_increment_total(DATA this, float total);
void  data_increment_code (DATA this, int code);
void  data_increment_fail (DATA this, int fail);
void  data_increment_okay (DATA this, int ok200);

/* getters */
float    data_get_total(DATA this);
float    data_get_bytes(DATA this);
float    data_get_megabytes(DATA this);
float    data_get_highest(DATA this);
float    data_get_lowest(DATA this);
float    data_get_elapsed(DATA this);
float    data_get_availability(DATA this);
float    data_get_response_time(DATA this);
float    data_get_transaction_rate(DATA this);
float    data_get_throughput(DATA this);
float    data_get_concurrency(DATA this);
unsigned int data_get_count(DATA this);
unsigned int data_get_code (DATA this);
unsigned int data_get_fail (DATA this);
unsigned int data_get_okay (DATA this);

#endif/*__DATA_H*/
