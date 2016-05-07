/**
 * Error notification
 * Library: joedog
 *
 * Copyright (C) 2000-2013 by
 * Jeffrey Fulmer - <jeff@joedog.org>, et. al.
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

#ifdef  HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <notify.h>

#define BUFSIZE 40000

#define RESET      0
#define BRIGHT     1
#define DIM        2
#define UNDERLINE  3
#define BLINK      4
#define REVERSE    7
#define HIDDEN     8

#define UNCOLOR   -1
#define BLACK      0
#define RED        1
#define GREEN      2
#define YELLOW     3
#define BLUE       4
#define MAGENTA    5
#define CYAN       6
#define WHITE      7

typedef enum {
  __LOG = 1,
  __OUT = 2,
} METHOD;

static void __message(METHOD M, LEVEL L, const char *fmt, va_list ap);

void
OPENLOG(char *program)
{
  openlog(program, LOG_PID, LOG_DAEMON); 
  return;
}

void 
CLOSELOG(void)
{
  closelog();
  return;
}

static void
__message(METHOD M, LEVEL L, const char *fmt, va_list ap)
{
  char   buf[BUFSIZE];
  char   msg[BUFSIZE+1024];
  LEVEL  level = WARNING;
  char   pmode[64];
  char   lmode[64];
  memset(lmode, '\0', 64);
  memset(pmode, '\0', 64);

  vsprintf(buf, fmt, ap);
  if (errno == 0 || errno == ENOSYS || L == DEBUG) {
    snprintf(msg, sizeof msg, "%s\n", buf);
  } else {
    snprintf(msg, sizeof msg, "%s: %s\n", buf, strerror(errno));
  }

  switch (L) {
    case DEBUG:
      sprintf(pmode, "[%c[%d;%dmdebug%c[%dm]", 0x1B, BRIGHT, BLUE+30, 0x1B, RESET);
      strcpy(lmode, "[debug]");
      level = LOG_WARNING;
      break;
    case WARNING:
      sprintf(pmode, "[%c[%d;%dmalert%c[%dm]", 0x1B, BRIGHT, GREEN+30, 0x1B, RESET);
      strcpy(lmode, "[alert] ");
      level = LOG_WARNING;
      break;
    case ERROR:
      sprintf(pmode, "[%c[%d;%dmerror%c[%dm]", 0x1B, BRIGHT, YELLOW+30, 0x1B, RESET);
      strcpy(lmode, "[error]");
      level = LOG_ERR;
      break;
    case FATAL:
      sprintf(pmode, "[%c[%d;%dmfatal%c[%dm]", 0x1B, BRIGHT, RED+30, 0x1B, RESET);
      strcpy(lmode, "[fatal]");
      level = LOG_CRIT;
      break;
  }
  
  if (M == __LOG) {
    syslog(level, "%s %s", lmode, msg);
  } else {
    fflush(stdout);
    fprintf(stderr, "%s %s", pmode, msg);
  }
  if (L==FATAL) { exit(1); }
  return;
}

void 
SYSLOG(LEVEL L, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  
  __message(__LOG, L, fmt, ap);
  va_end(ap);
  
  return;
}

void
NOTIFY(LEVEL L, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  __message(__OUT, L, fmt, ap);
  va_end(ap);

  return;
}

void
__display(int color, const char *fmt, va_list ap) 
{
  char   buf[BUFSIZE];
  char   msg[BUFSIZE+1024];

  vsprintf(buf, fmt, ap);
  if (color == UNCOLOR) {
    snprintf(msg, sizeof msg,"%s\n", buf);
  } else {
    snprintf(msg, sizeof msg, "%c[%d;%dm%s%c[%dm\n", 0x1B, RESET, color+30, buf, 0x1B, RESET);
  }
  printf("%s", msg);
  return;
}

void 
DISPLAY(int color, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  __display(color, fmt, ap);
  va_end(ap);
  return;
}

