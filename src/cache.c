/**
 * HTTP Logical Local Cache
 * (we don't actually store anything)
 *
 * Copyright (C) 2016
 * Jeffrey Fulmer - <jeff@joedog.org>, et al.
 *
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
#include <stdio.h>
#include <stdlib.h>
#include <setup.h>
#include <util.h>
#include <hash.h>
#include <cache.h>
#include <joedog/joedog.h>
#include <joedog/boolean.h>
#include <joedog/defs.h>

#define xfree(x) free(x)
#define xmalloc(x) malloc(x)
#define xstrdup(x) strdup(x)

struct CACHE_T
{
  HASH   cache;
  int    hlen;
  char * header;
};

size_t CACHESIZE = sizeof(struct CACHE_T);

CACHE
new_cache()
{
  CACHE this   = calloc(CACHESIZE, 1);
  this->cache  = new_hash();
  this->header = NULL;
  hash_set_destroyer(this->cache, (void*)date_destroy);
  return this;
}


CACHE
cache_destroy(CACHE this)
{
  if (this != NULL) {
    this->cache = hash_destroy(this->cache);
    xfree(this);
    this        = NULL;
  }
  return this;
}

BOOLEAN
cache_contains(CACHE this, URL U)
{
  return hash_contains(this->cache, url_get_request(U));
}

void
cache_add(CACHE this, URL U, char *date)
{
  if (hash_contains(this->cache, url_get_request(U))) {
    // NOTE: hash destroyer was set in the constructor
    hash_remove(this->cache, url_get_request(U));
  }
  hash_nadd(this->cache, url_get_request(U), new_date(date), DATESIZE); 
}

DATE
cache_get(CACHE this, URL U)
{
  return (DATE)hash_get(this->cache, url_get_request(U));
}

/**
 * Yeah, this function is kind of kludgy. We have to 
 * localize everything in order to fit it into our OO
 * architecture but the benefits out-weigh the negatives
 *
 * From an API perspective, we don't want to worry about
 * managing memory outside of the object.
 */
char *
cache_get_header(CACHE this, URL U, char *name)
{
  DATE d = NULL;
  char tmp[256];
 
  if (! hash_contains(this->cache, url_get_request(U))) {
    return ""; 
  }

  memset(tmp, '\0', 256);

  d = (DATE)hash_get(this->cache, url_get_request(U));

  if (strmatch(name, "If-None-Match")) {
    char   *etag = strdup(date_get_etag(d)); // strdup for local copy
    if (empty(etag)) return "";

    snprintf(tmp, 256, "If-None-Match: %s\015\012", etag);
    this->header = strdup(tmp);
    xfree(etag);
    return this->header;
  } else if (strmatch(name, "If-Modified-Since")) {
    char *date = strdup(date_get_rfc850(d)); 

    if (empty(date)) return "";
    snprintf(tmp, 256, "If-Modified-Since: %s\015\012", date);
    this->header = strdup(tmp);
    return this->header;
  }
  return ""; // Unsupported header or a WTF?
}
