/**
 * HTTP Cache
 *
 * Copyright (C) 2013-2014 by
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
#ifndef __CACHE_H
#define __CACHE_H

#include <stdlib.h>
#include <date.h>
#include <url.h>
#include <joedog/defs.h>
#include <joedog/boolean.h>

typedef enum {
  C_ETAG    = 0,
  C_LAST    = 1,
  C_EXPIRES = 2 
} CTYPE;


typedef struct CACHE_T *CACHE;
extern size_t  CACHESIZE;

CACHE   new_cache();
CACHE   cache_destroy(CACHE this);
BOOLEAN cache_contains(CACHE this, CTYPE type, URL U);
void    cache_add(CACHE this, CTYPE type, URL U, char *date);
DATE    cache_get(CACHE this, CTYPE type, URL U);
char *  cache_get_header(CACHE this, CTYPE type, URL U);
BOOLEAN is_cached(CACHE this, URL U);


#endif/*__CACHE_H*/
