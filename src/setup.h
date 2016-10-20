/**
 * Package header
 *
 * Copyright (C) 2000-2013 by
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
 * 
 */
#ifndef SETUP_H
#define SETUP_H

#ifdef  HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <ctype.h>
#include <stdarg.h>

#ifdef  HAVE_LIMITS_H
# include <limits.h>
#endif/*HAVE_LIMITS_H*/

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif

#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#ifdef  HAVE_UNISTD_H
# include <unistd.h>
#endif/*HAVE_UNISTD_H*/

#ifdef STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#ifdef  HAVE_SYS_TIMES_H
# include <sys/times.h>
#endif/*HAVE_SYS_TIMES_H*/

#if  TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif/*TIME_WITH_SYS_TIME*/

#if HAVE_ERRNO_H
# include <errno.h>
#endif /* HAVE_ERRNO_H */

#ifndef  PTHREAD_CREATE_JOINABLE
# define PTHREAD_CREATE_JOINABLE  0
#endif /*PTHREAD_CREATE_JOINABLE*/ 
#ifndef  PTHREAD_CREATE_DETACHED
# define PTHREAD_CREATE_DETACHED  1
#endif /*PTHREAD_CREATE_DETACHED*/


#ifndef HAVE_STRCASECMP
int strcasecmp();
#endif
#ifndef HAVE_STRNCASECMP
int strncasecmp();
#endif
#ifndef HAVE_STRNCMP
int strncmp();
#endif
#ifndef HAVE_STRLEN
int strlen();
#endif

#include <url.h>
#include <auth.h>
#include <array.h>
#include <cookies.h>
#include <joedog/boolean.h>

#define  MXCHLD         1024
#define  MSG_NOERROR    010000

#define F_CONNECTING  1
#define F_READING     2
#define F_DONE        4

#define MAXREPS       10301062

#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif

#ifndef INT_MIN
# define INT_MIN (~0 << (sizeof (int) * CHAR_BIT - 1))
#endif
#ifndef INT_MAX
# define INT_MAX (~0 - INT_MIN)
#endif

typedef struct {
  int  index;
  char **line;
} LINES;

void display_help();
void display_version(BOOLEAN b);

/** 
 * configuration struct;
 * NOTE: this data is writeable ONLY during 
 * the configuration step before any threads
 * are spawned.
 */
struct CONFIG
{
  BOOLEAN logging;      /* boolean, log transactions to log file   */
  BOOLEAN shlog;        /* show log file configuration directive.  */
  int     limit;        /* Limits the thread count to int          */
  char    *url;         /* URL for the single hit invocation.      */
  char    logfile[128]; /* alternative user defined simbot.log     */ 
  BOOLEAN verbose;      /* boolean, verbose output to screen       */
  BOOLEAN quiet;        /* boolean, turn off all output to screen  */
  BOOLEAN parser;       /* boolean, turn on/off the HTML parser    */
  BOOLEAN csv;          /* boolean, display verbose output in CSV  */
  BOOLEAN fullurl;      /* boolean, display full url in verbose    */
  BOOLEAN display;      /* boolean, display the thread id verbose  */
  BOOLEAN config;       /* boolean, prints the configuration       */
  BOOLEAN color;        /* boolean, true for color, false for not  */
  int     cusers;       /* default concurrent users value.         */
  float   delay;        /* range for random time delay, see -d     */
  int     timeout;      /* socket connection timeout value, def:10 */
  BOOLEAN bench;        /* signifies a benchmarking run, no delay  */
  BOOLEAN internet;     /* use random URL selection if TRUE        */
  BOOLEAN timestamp;    /* timestamp the output                    */
  int     time;         /* length of the siege in hrs, mins, secs  */
  int     secs;         /* time value for the lenght of the siege  */
  int     reps;         /* reps to run the test, default infinite  */ 
  char    file[128];    /* urls.txt file, default in joepath.h     */
  int     length;       /* length of the urls array, made global   */
  LINES * nomap;        /* list of hosts to not follow             */
  BOOLEAN debug;        /* boolean, undocumented debug command     */
  BOOLEAN chunked;      /* boolean, accept chunked encoding        */
  BOOLEAN unique;       /* create unique files for upload          */
  BOOLEAN get;          /* get header information for debugging    */ 
  BOOLEAN print;        /* get header and page for debugging       */ 
  BOOLEAN mark;         /* signifies a log file mark req.          */ 
  char    *markstr;     /* user defined string value to mark file  */
  int     protocol;     /* 0=HTTP/1.0; 1=HTTP/1.1                  */
  COOKIES cookies;      /* cookies    */
  char uagent[256];     /* user defined User-Agent string.         */
  char encoding[256];   /* user defined Accept-Encoding string.    */
  char conttype[256];   /* user defined default content type.      */
  int  bids;            /* W & P authorization bids before failure */
  AUTH auth;
  BOOLEAN keepalive;    /* boolean, connection keep-alive value    */
  int     signaled;     /* timed based testing notification bool.  */
  char    extra[2048];  /* extra http request headers              */ 
  #if 0
  struct {
    BOOLEAN required;   /* boolean, TRUE == use a proxy server.    */
    char *hostname;     /* hostname for the proxy server.          */
    int  port;          /* port number for proxysrv                */ 
    char *encode;       /* base64 encoded username and password    */
  } proxy;
  #endif
  BOOLEAN login;        /* boolean, client must login first.       */
  char    *loginurl;    /* XXX: deprecated the initial login URL   */
  ARRAY   lurl;
  int     failures;     /* number of failed attempts before abort. */
  int     failed;       /* total number of socket failures.        */
  BOOLEAN escape;       /* boolean, TRUE == url-escaping           */
  BOOLEAN expire;       /* boolean, TRUE == expire cookies ea. run */
  BOOLEAN follow;       /* boolean, TRUE == follow 302             */
  BOOLEAN zero_ok;      /* boolean, TRUE == zero bytes data is OK. */ 
  BOOLEAN spinner;      /* boolean, TRUE == spin, FALSE not so much*/
  BOOLEAN cache;        /* boolean, TRUE == cache revalidate       */
  char    rc[256];      /* filename of SIEGERC file                */  
  int     ssl_timeout;  /* SSL session timeout                     */
  char    *ssl_cert;    /* PEM certificate file for client auth    */
  char    *ssl_key;     /* PEM private key file for client auth    */
  char    *ssl_ciphers; /* SSL chiphers to use : delimited         */ 
  METHOD  method;       /* HTTP method for --get requests          */
  pthread_cond_t  cond;
  pthread_mutex_t lock;
};


#if INTERN
# define EXTERN /* */
#else
# define EXTERN extern
#endif
 
EXTERN struct CONFIG my;

#endif  /* SETUP_H */
