/**
 * Cookie support
 *
 * Copyright (C) 2001-2014 Jeffrey Fulmer <jeff@joedog.org>, et al
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *--
 */
#ifndef COOKIES_H
#define COOKIES_H

#ifdef  HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

#include <stdlib.h>
#include <joedog/joedog.h>
#include <joedog/boolean.h>

#define MAX_COOKIE_SIZE 8192

/**
 * cookie node
 */
typedef struct CNODE
{
  int    index;
  pthread_t threadID;
  char   *name;
  char   *value;
  char   *path;
  char   *domain;
  time_t expires;
  char   *expirestr;
  char   *version;
  char   *max;
  int    secure;
  struct CNODE  *next;
} CNODE;

typedef struct
{
  CNODE *first;
  pthread_mutex_t mutex;
} COOKIE;

int      add_cookie(pthread_t id, char *host, char *value);
BOOLEAN  delete_cookie(pthread_t id, char *name); 
int      delete_all_cookies(pthread_t id);
int      check_cookie(char *domain, char *value);
char     *get_cookie_header(pthread_t id, char *domain, char *cookie);
void     display_cookies();

extern COOKIE *cookie;

#endif/*COOKIES_H*/

