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
#include <memory.h>
#include <perl.h>
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

private const char* keys[] = { "ET_", "LM_", "EX_", NULL };
private char * __build_key(CTYPE type, URL U);

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
cache_contains(CACHE this, CTYPE type, URL U)
{
  char   *key;
  BOOLEAN found = FALSE;

  if (!my.cache) return FALSE;

  key = __build_key(type, U);
  if (key == NULL) {
    return FALSE;
  }
  found = hash_contains(this->cache, key);

  xfree(key);
  return found;
}

BOOLEAN
is_cached(CACHE this, URL U)
{
  DATE  day = NULL;
  char *key = __build_key(C_EXPIRES, U);

  if (hash_contains(this->cache, key)) {
    day = (DATE)hash_get(this->cache, key);
    if (date_expired(day) == FALSE) {
      return TRUE;
    } else {
      hash_remove(this->cache, key);
      return FALSE;
    }
  }
  return FALSE;
}

void
cache_add(CACHE this, CTYPE type, URL U, char *date)
{
  char *key = __build_key(type, U);
  
  if (key == NULL) return;

  if (type != C_EXPIRES && hash_contains(this->cache, key)) {
    // NOTE: hash destroyer was set in the constructor
    hash_remove(this->cache, key);
  }

  switch (type) {
    case C_ETAG:
      hash_nadd(this->cache, key, new_etag(date), DATESIZE); 
      break; 
    case C_EXPIRES:
      if (hash_contains(this->cache, key) == TRUE) {
        break; 
      }
      hash_nadd(this->cache, key, new_date(date), DATESIZE); 
      break;
    default:
      hash_nadd(this->cache, key, new_date(date), DATESIZE); 
      break; 
  }
  xfree(key);
  
  return;
}

DATE
cache_get(CACHE this, CTYPE type, URL U)
{
  DATE  date;
  char  *key = __build_key(type, U);

  if (key == NULL) return NULL;
  
  date = (DATE)hash_get(this->cache, key);
  xfree(key);

  return date;
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
cache_get_header(CACHE this, CTYPE type, URL U)
{
  DATE  d   = NULL;
  DATE  e   = NULL;
  char *key = NULL;
  char *exp = NULL;
  char  tmp[256];

  /**
   * If we don't have it cached, there's no
   * point in continuing...
   */
  if (! cache_contains(this, type, U)) {
    return NULL;
  }

  /**
   * Check it's freshness
   */
  exp = __build_key(C_EXPIRES, U);
  if (exp) {
    e = (DATE)hash_get(this->cache, exp);
    xfree(exp);
    if (date_expired(e)) {
      // remove entries
      return NULL;
    }
  }

  /**
   * If we can't build a key, we better bail...
   */
  key = __build_key(type, U);
  if (key == NULL) {
    return NULL;
  } 

  /**
   * At this point we should be able to grab
   * a date but we'll test and bail on failure
   */
  d = (DATE)hash_get(this->cache, key);
  if (d == NULL) {
    return NULL;
  }

  memset(tmp, '\0', 256);
  switch (type) {
    char *ptr = NULL;
    case C_ETAG:
      ptr = strdup(date_get_etag(d)); // need a local copy 
      if (empty(ptr)) return "";      // should never happen
      snprintf(tmp, 256, "If-None-Match: %s\015\012", ptr);
      this->header = strdup(tmp);
      xfree(ptr);
      return this->header;
    default:
      ptr = strdup(date_get_rfc850(d));
      if (empty(ptr)) return "";
      snprintf(tmp, 256, "If-Modified-Since: %s\015\012", ptr);
      this->header = strdup(tmp);
      xfree(ptr);
      return this->header;
  }
  return NULL; // Unsupported header or a WTF?
}

private char *
__build_key(CTYPE type, URL U)
{
  int   len = 0; 
  char *key = NULL;

  if (U == NULL || url_get_request(U) == NULL || strlen(url_get_request(U)) < 1) {
    return NULL;
  }
  len = strlen(url_get_request(U)) + strlen(keys[type])+1;
  key = (char *)xmalloc(len+1);
  memset(key, '\0', len+1);
  snprintf(key, len, "%s%s", keys[type], url_get_request(U));
  return key;  
}
