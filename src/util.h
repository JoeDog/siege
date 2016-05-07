/**
 * Utility Functions
 *
 * Copyright (C) 2001-2014 
 * by Jeffrey Fulmer <jeff@joedog.org>, et al.
 * This file is part of Siege
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
 *
 */
#ifndef  UTIL_H
#define  UTIL_H

#include <setup.h>
#include <joedog/boolean.h>

void    parse_time(char *p);
float   elapsed_time(clock_t time);
char *  substring(char *str, int start, int len);
void    pthread_sleep_np(unsigned int seconds); 
void    pthread_usleep_np(unsigned long usec); 
int     pthread_rand_np(unsigned int *ctx); 
int     urandom();
BOOLEAN strmatch(char *str1, char *str2);
BOOLEAN startswith(const char *pre, const char *str);
BOOLEAN endswith(const char *suffix, const char *str);
BOOLEAN okay(int code);
void    echo(const char *fmt, ...);
void    debug (const char *fmt, ...);
char *  uppercase(char *s, size_t len);
char *  lowercase(char *s, size_t len);
char *  stristr(const char *haystack, const char *needle);
#ifndef strnlen
size_t  strnlen(const char *str, size_t len);
#endif
#ifndef strncasestr
const char * strncasestr(const char *str1, const char *str2, size_t len);
#endif

#endif /*UTIL_H*/

