#ifndef JOEDOG_H
#define JOEDOG_H 
/**
 * JOEDOG HEADER 
 *
 * Copyright (C) 2000-2006 Jeffrey Fulmer <jeff@joedog.org>
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
 * -- 
 */
#include <config.h>
#include <time.h>  
#include <stdarg.h>

#define BLACK      0
#define RED        1
#define GREEN      2
#define YELLOW     3
#define BLUE       4
#define MAGENTA    5
#define CYAN       6
#define WHITE      7

/**
 * Error notification
 */
typedef enum {
  DEBUG     = 0,
  WARNING   = 1,
  ERROR     = 2,
  FATAL     = 3
} LEVEL;

void OPENLOG(char *program_name);
void CLOSELOG(void);
void SYSLOG(LEVEL L, const char *fmt, ...);
void NOTIFY(LEVEL L, const char *fmt, ...);
void DISPLAY(int color, const char *fmt, ...);

/**
 * Memory management
 */
void * xrealloc(void *, size_t);
void * xmalloc (size_t);
void * xcalloc (size_t, size_t); 
char * xstrdup(const char *str);
char * xstrcat(const char *arg1, ...);
void xfree(void *ptr); 

/**
 * Utility functions
 */
void  itoa(int, char []);
void  reverse(char []);
int   my_random(int, int);
char  *substring(char *, int, int);
float elapsed_time(clock_t);

/**
 * snprintf functions
 */
#define PORTABLE_SNPRINTF_VERSION_MAJOR 2
#define PORTABLE_SNPRINTF_VERSION_MINOR 2

#ifdef HAVE_SNPRINTF
#include <stdio.h>
#else
extern int snprintf(char *, size_t, const char *, /*args*/ ...);
extern int vsnprintf(char *, size_t, const char *, va_list);
#endif

#if defined(HAVE_SNPRINTF) && defined(PREFER_PORTABLE_SNPRINTF)
extern int portable_snprintf(char *str, size_t str_m, const char *fmt, /*args*/ ...);
extern int portable_vsnprintf(char *str, size_t str_m, const char *fmt, va_list ap);
#define snprintf  portable_snprintf
#define vsnprintf portable_vsnprintf
#endif

#ifndef __CYGWIN__
extern int asprintf  (char **ptr, const char *fmt, /*args*/ ...);
extern int vasprintf (char **ptr, const char *fmt, va_list ap);
extern int asnprintf (char **ptr, size_t str_m, const char *fmt, /*args*/ ...);
extern int vasnprintf(char **ptr, size_t str_m, const char *fmt, va_list ap);
#endif/*__CYGWIN__*/

/**
 * chomps the newline character off
 * the end of a string.
 */
char *chomp(char *str);
 
/**
 * trims the white space from the right
 * of a string.
 */
char *rtrim(char *str);

/**
 * trims the white space from the left
 * of a string.
 */
char *ltrim(char *str);

/**
 * trims the white space from the left
 * and the right sides of a string.
 */
char * trim(char *str); 

/**
 * split string *s on pattern pattern pointer
 * n_words holds the size of **
 */
char **split(char pattern, char *s, int *n_words); 

/**
 * free memory allocated by split
 */
void split_free(char **split, int length); 


/**
 * tests for empty string; warns if invalid
 */
int empty(const char *s);

/**
 * portable strsep
 */
char *xstrsep(char **stringp, const char *delim);

/**
 * string allocation
 */
char *stralloc(char *); 

#endif/* JOEDOG_H */  
