/**
 * Utility Functions
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
#include <setup.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <notify.h>
#include <memory.h>
#include <util.h>
#include <joedog/boolean.h>
#include <joedog/defs.h>

void pthread_usleep_np(unsigned long usec); 

/**
 * parses the -t/--time option for a timed
 * interval.  The option requires a modifier,
 * H, M, or S, hours, minutes or seconds    
 */
void
parse_time(char *p)
{
  size_t x = 0;
  my.time = my.secs = 0;

  while (ISDIGIT(p[x]))
    x++;
  if (x==0) return;
  my.time = atoi(substring(p, 0, x));

  for (; x < strlen(p); x ++)
    switch (TOLOWER(p[x])) {
      case 's':
        my.secs = my.time;
        my.time = 1;
        return;
      case 'm':
        my.secs = my.time * 60;
        my.time = 1;
        return;
      case 'h':
        my.secs = my.time * 3600;
        my.time = 1;
        return;
      default:
        break;
    }
  if ((my.time > 0) && (my.secs <= 0)) {
    my.secs = my.time * 60;
  }

  return;
}

char *
substring(char *str, int start, int len)
{
  int   i;
  char  *ret;
  char  *res;
  char  *ptr;
  char  *end;

  if ((len < 1) || (start < 0) || (start > (int)strlen (str)))
    return NULL;

  if (start+len > (int)strlen(str))
    len = strlen(str) - start;

  ret = xmalloc(len+1);
  res = ret;
  ptr = str;
  end = str;

  for (i = 0; i < start; i++, ptr++) ;
  for (i = 0; i < start+len; i++, end++) ;
  while (ptr < end)
    *res++ = *ptr++;

  *res = 0;
  return ret;
}

BOOLEAN
okay(int code)
{
  return (code >= 100 && code <= 299);
}


BOOLEAN
strmatch(char *option, char *param)
{
  if (!strncasecmp(option,param,strlen(param))&&strlen(option)==strlen(param))
    return TRUE;
  else
    return FALSE;
}

BOOLEAN
startswith(const char *pre, const char *str)
{
  size_t lenpre = strlen(pre);
  size_t lenstr = strlen(str);
  return lenstr < lenpre ? FALSE : strncmp(pre, str, lenpre) == 0;
}

BOOLEAN
endswith(const char *suffix, const char *str)
{
  if (!str || !suffix)
    return FALSE;
  size_t lenstr    = strlen(str);
  size_t lensuffix = strlen(suffix);
  if (lensuffix >  lenstr)
    return FALSE;
  return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

char *
uppercase(char *s, size_t len){
  unsigned char *c, *e;

  c = (unsigned char*)s;
  e = c+len;

  while (c < e) {
    *c = TOUPPER((unsigned char)(*c));
    c++;
  }
  return s;
}


char *
lowercase(char *s, size_t len){
  unsigned char *c, *e;

  c = (unsigned char*)s;
  e = c+len;

  while (c < e) {
    *c = TOLOWER((unsigned char)(*c));
    c++;
  }
  return s;
}

char * 
stristr(const char* haystack, const char* needle) {
  do {
    const char* h = haystack;
    const char* n = needle;
    while (tolower((unsigned char) *h) == tolower((unsigned char ) *n) && *n) {
      h++;
      n++;
    }
    if (*n == 0) {
      return (char *) haystack;
    }
  } while (*haystack++);
  return NULL;
}

/**
 * sleep and usleep work on all supported
 * platforms except solaris, so we'll use
 * those functions on non-solaris, but we
 * still need to sleep. The macro handles
 * that for us.
 */
void
pthread_sleep_np( unsigned int secs )
{
#if defined( SOLARIS ) || defined( sun )
  /* Theoretically, this could fail for sizeof(int)==sizeof(long) and
   * very large values of secs due to an overflow.
   * NB: for 64-bit int, that would mean waiting until the year 584543.
   */
  pthread_usleep_np (secs*1000000);
#else
  sleep(secs);
#endif/*pthread_sleep_np*/

  return;
}


void
pthread_usleep_np(unsigned long usec)
{
#if defined(SOLARIS) || defined(sun) 
  int err, type;
  struct timeval  now; 
  struct timespec timeout;
  pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t  timer_cond  = PTHREAD_COND_INITIALIZER;
  
  pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, &type ); 
  gettimeofday(&now, NULL);
  timeout.tv_sec  = now.tv_sec + usec/1000000;
  timeout.tv_nsec = (now.tv_usec + usec%1000000)*1000;
  /* At most 1s carry. */ 
  if(timeout.tv_nsec >= 1000000000){
    timeout.tv_nsec -= 1000000000;
    timeout.tv_sec++;
  }

  pthread_mutex_lock(&timer_mutex);
  err = pthread_cond_timedwait(&timer_cond, &timer_mutex, &timeout);
  
  pthread_setcanceltype(type,NULL);
  pthread_testcancel();  
#else
  usleep(usec);
#endif
  return;  
}

float
elapsed_time(clock_t time)
{
  long tps = sysconf( _SC_CLK_TCK );
  return (float)time/tps;
}

void
echo (const char *fmt, ...)
{
  char    buf[256];
  va_list ap;

  if (my.quiet) {
    return;
  }

  if (my.get) {
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    printf("%s", buf); // yes, that's ugly...
    va_end(ap);
    fflush(stdout);
    return;
  }

  if (my.debug) {
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    if (strlen(buf) == 1) {
      printf("%s", buf);
    } else {
      NOTIFY(DEBUG, buf);
    }
    va_end(ap);
  }
  return;
}

void
debug (const char *fmt, ...)
{
  char    buf[256];
  va_list ap;

  if (my.quiet) {
    return;
  }

  if (my.debug) {
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    if (strlen(buf) == 1) {
      printf("%s", buf);
    } else {
      NOTIFY(DEBUG, buf);
    }
    va_end(ap);
  }
  return;
}


/*-
 * Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */ 
#ifndef HAVE_RAND_R 
static int
do_rand(unsigned long *ctx)
{
  return ((*ctx = *ctx * 1103515245 + 12345) % ((u_long)RAND_MAX + 1));
}
 
int
posix_rand_r(unsigned int *ctx)
{
  u_long val = (u_long) *ctx;
  *ctx = do_rand(&val);
  return (int) *ctx;
}
#endif
 
int
pthread_rand_np(unsigned int *ctx)
{
#ifndef HAVE_RAND_R
  return (int)posix_rand_r(ctx);
#else
  return (int)rand_r(ctx);
#endif
}

int
urandom()
{
#ifdef HAVE_DEV_RANDOM
  int  rand = -1;
  int  fd;
  int  len;

  if ((fd = open("/dev/urandom", O_RDONLY)) >= 0) {
    len = read(fd, &rand, sizeof(rand));
    if (len == -1) {
      fprintf(stderr, "ERROR: failed to open /dev/urandom\n"); 
    }
    close(fd);
  }
  return rand;
#else 
  unsigned int randrseed;

  randrseed = time(0);
  return pthread_rand_np(&randrseed);
#endif
}

#ifndef strnlen
size_t
strnlen(const char *str, size_t len)
{
  const char *end = memchr(str, '\0', len);
  return (end != NULL) ? (size_t) (end - str) : len;
}
#endif

#ifndef strncasestr
const char *
strncasestr(const char *str1, const char *str2, size_t len)
{
  size_t str1_len = strnlen(str1, len);
  size_t str2_len = strlen(str2);
  size_t i;

  if (str1_len < 1 || str2_len < 1) {
    return NULL;
  } 

  for (i = 0; i < (str1_len - str2_len + 1); i++) {
    if (strncasecmp(str1, str2, str2_len) == 0) {
        return str1;
    }
    str1++;
  }
  return NULL;
}
#endif
