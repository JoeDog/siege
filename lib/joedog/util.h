/**
 * Utility functions
 * Library: joedog
 *
 * Copyright (C) 2000-2007 by
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
 */
#ifndef UTIL_H
#define UTIL_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */ 

#ifdef STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif /* HAVE_STRCHR */
char *strchr (), *strrchr ();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif /* HAVE_MEMCPY  */
#endif  /* STDC_HEADERS */

#ifdef HAVE_SYS_TIMES_H
# include <sys/times.h>
#endif /* HAVE_SYS_TIMES_H */

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif /* HAVE_SYS_TIME_H    */
#endif  /* TIME_WITH_SYS_TIME */ 

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <unistd.h>
#include "memory.h"

void   itoa(int, char []);
void   reverse(char []);
int    my_random(int, int);
char   *substring(char *, int, int);
float  elapsed_time(clock_t);

#endif  /* UTIL_H */

