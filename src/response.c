/**
 * HTTP Response Headers
 *
 * Copyright (C) 2016
 * Jeffrey Fulmer - <jeff@joedog.org>, et al.
 * Copyright (C) 1999 by
 * Jeffrey Fulmer - <jeff@joedog.org>.
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
#include <string.h>
#include <setup.h>
#include <url.h>
#include <auth.h>
#include <date.h>
#include <util.h>
#include <hash.h>
#include <memory.h>
#include <perl.h>
#include <response.h>
#include <joedog/boolean.h>
#include <joedog/defs.h>

struct RESPONSE_T
{
  HASH  headers;
  struct {
    int   www; 
    int   proxy; 
    struct {
      char  *www;
      char  *proxy;
    } realm;
    struct {
      char  *www; 
      char  *proxy;
    } challenge;   
    struct {
      TYPE  www;
      TYPE  proxy;
    } type;
  } auth;
  BOOLEAN  cached;
};

size_t RESPONSESIZE = sizeof(struct RESPONSE_T);

private char *  __parse_pair(char **str);
private char *  __dequote(char *str);
private int     __int_value(RESPONSE this, char *key, int def); 
private BOOLEAN __boolean_value(RESPONSE this, char *key, BOOLEAN def);

RESPONSE
new_response()
{
  RESPONSE this;
  this = xcalloc(RESPONSESIZE, 1);
  this->headers            = new_hash();
  this->auth.realm.www     = NULL;
  this->auth.challenge.www = NULL;
  this->auth.realm.proxy   = NULL;
  this->auth.challenge.www = NULL;
  return this;
}

RESPONSE
response_destroy(RESPONSE this) 
{
  if (this!=NULL) {
    this->headers = hash_destroy(this->headers);
    xfree(this->auth.realm.www);
    xfree(this->auth.challenge.www);
    xfree(this->auth.realm.proxy);
    xfree(this->auth.challenge.proxy);
    xfree(this);
    this = NULL;
  }
  return this;
}

BOOLEAN
response_set_code(RESPONSE this, char *line)
{ // we expect the start line: HTTP/1.0 200 OK
  char *tmp = line;
  char  arr[32]; 

  if (strncasecmp(line, "http", 4) == 0) {
    int num = atoi(tmp+9);
    if (num > 1) {
      memset(arr, '\0', sizeof(arr));
      strncpy(arr, line, 8);
      hash_add(this->headers, PROTOCOL, arr);
      hash_add(this->headers, RESPONSE_CODE, line+9);
      return TRUE;
    }
  }
  return FALSE;
}

int
response_get_code(RESPONSE this)
{ 
  if (this == NULL || this->headers == NULL || (char *)hash_get(this->headers, RESPONSE_CODE) == NULL) {
    return 418; //  I'm a teapot (RFC 2324)
  }
  return atoi((char *)hash_get(this->headers, RESPONSE_CODE));
}

char *
response_get_protocol(RESPONSE this)
{
  return ((char*)hash_get(this->headers, PROTOCOL) == NULL) ? "HTTP/1.1" :
          (char*)hash_get(this->headers, PROTOCOL);
}

int
response_success(RESPONSE this)
{
  if ((char *)hash_get(this->headers, RESPONSE_CODE) == NULL) {
    return 0;
  }
  int code = atoi((char *)hash_get(this->headers, RESPONSE_CODE));
  return (code <  400 || code == 401 || code == 407) ? 1 : 0;
}

int
response_failure(RESPONSE this)
{
  if ((char *)hash_get(this->headers, RESPONSE_CODE) == NULL) {
    return 1;
  }
  int code = atoi((char *)hash_get(this->headers, RESPONSE_CODE));
  return (code >= 400 && code != 401 && code != 407) ? 1 : 0;
}

void
response_set_from_cache(RESPONSE this, BOOLEAN cached)
{
  this->cached = cached;
}

BOOLEAN
response_get_from_cache(RESPONSE this)
{
  return this->cached;
}

BOOLEAN
response_set_content_type(RESPONSE this, char *line) {
  char   *type  = NULL;
  char   *set   = NULL;
  char   *aid   = NULL;
  char   *ptr   = NULL;
  BOOLEAN res   = FALSE;

  if (strstr(line, ";") != NULL) {
    ptr  = line+(strlen(CONTENT_TYPE)+2);
    type = strtok_r(ptr, ";", &aid);
    hash_add(this->headers, CONTENT_TYPE, type);
    res = TRUE;

    /** 
     * We found a ';', do we have a charset?
     */
    set  = stristr(aid, "charset=");
    if (set && strlen(set) > 8) {
      hash_add(this->headers, CHARSET, set+8);
    }
  } else {
    hash_add(this->headers, CONTENT_TYPE, line+(strlen(CONTENT_TYPE)+2));
    res = TRUE;
  }
  return res;
}

char *
response_get_content_type(RESPONSE this) 
{
 return ((char*)hash_get(this->headers, CONTENT_TYPE) == NULL) ? "unknown" : (char*)hash_get(this->headers, CONTENT_TYPE);
}

char *
response_get_charset(RESPONSE this)
{
  if ((char*)hash_get(this->headers, CHARSET) == NULL) {
    hash_add(this->headers, CHARSET, "iso-8859-1");
  }
  return (char*)hash_get(this->headers, CHARSET); 
}

BOOLEAN
response_set_content_length(RESPONSE this, char *line)
{ // Expect Content-length: NUM
  char *tmp = line;
  if (strncasecmp(line, "content-length", strlen(CONTENT_LENGTH)) == 0) {
    int num = atoi(tmp+(strlen(CONTENT_LENGTH)+2));
    if (num > 1) {
      hash_add(this->headers, CONTENT_LENGTH, line+(strlen(CONTENT_LENGTH)+2));
      return TRUE;
    }
  }
  return FALSE;
}

int
response_get_content_length(RESPONSE this)
{
  return __int_value(this, CONTENT_LENGTH, 0);
}

BOOLEAN
response_set_content_encoding(RESPONSE this, char *line)
{
  char *ptr = NULL;
  char  tmp[128];

  if (strncasecmp(line, CONTENT_ENCODING, strlen(CONTENT_ENCODING)) == 0) {
    // These should only be supported encodings since we have brakes in init.c
    memset(tmp, '\0', sizeof(tmp));
    ptr = line+(strlen(CONTENT_ENCODING)+2);
    if (strmatch(ptr, "gzip")) {
      snprintf(tmp, sizeof(tmp), "%d", GZIP);
      hash_add(this->headers, CONTENT_ENCODING, (void*)tmp);
      return TRUE;
    } 
    if (strmatch(ptr, "deflate")) {
      snprintf(tmp, sizeof(tmp), "%d", DEFLATE);
      hash_add(this->headers, CONTENT_ENCODING, (void*)tmp);
      return TRUE;
    } 
  }
  return FALSE;
}

HTTP_CE
response_get_content_encoding(RESPONSE this)
{
  return (HTTP_CE)__int_value(this, CONTENT_ENCODING, 0);
}

BOOLEAN
response_set_transfer_encoding(RESPONSE this, char *line)
{
  char *ptr = NULL;
  char  tmp[128];

  if (strncasecmp(line, TRANSFER_ENCODING, strlen(TRANSFER_ENCODING)) == 0) {
    memset(tmp, '\0', sizeof(tmp));
    ptr = line+(strlen(TRANSFER_ENCODING)+2);
    ptr = trim(ptr);
    if (strmatch(ptr, "chunked")) {
      snprintf(tmp, sizeof(tmp), "%d", CHUNKED);
    } else if (strmatch(ptr, "trailer")) {
      snprintf(tmp, sizeof(tmp), "%d", TRAILER);
    } else {
      snprintf(tmp, sizeof(tmp), "%d", NONE);
    }
    hash_add(this->headers, TRANSFER_ENCODING, (void*)tmp);
    return TRUE;
  }
  return FALSE;
}

HTTP_TE
response_get_transfer_encoding(RESPONSE this)
{
  return (HTTP_TE)__int_value(this, TRANSFER_ENCODING, NONE);
}

BOOLEAN
response_set_location(RESPONSE this, char *line)
{
  int   len = 0;
  char *tmp = NULL;

  if (strncasecmp(line, LOCATION, strlen(LOCATION)) == 0) {
    len = strlen(line);
    tmp = xmalloc(len);
    memset(tmp, '\0', len);
    memmove(tmp, line+10, len-9);
    tmp[len-10] = '\0'; 
    hash_add(this->headers, LOCATION, (void*)tmp);
    hash_add(this->headers, REDIRECT, "true");
    xfree(tmp);
  }
  return __boolean_value(this, REDIRECT, FALSE);
}

char *
response_get_location(RESPONSE this)
{
  return (char*)hash_get(this->headers, LOCATION);
}

BOOLEAN
response_get_redirect(RESPONSE this)
{
  return __boolean_value(this, REDIRECT, FALSE);
}

BOOLEAN
response_set_connection(RESPONSE this, char *line)
{
  char tmp[128];

  if (strncasecmp(line, CONNECTION, strlen(CONNECTION)) == 0) {
    memset(tmp, '\0', 128);
    if (strncasecmp(line+12, "keep-alive", 10) == 0) {
      snprintf(tmp, sizeof(tmp), "%d", KEEPALIVE);
    } else {
      snprintf(tmp, sizeof(tmp), "%d", CLOSE);
    }
    hash_add(this->headers, CONNECTION, tmp);
    return TRUE;
  }
  return FALSE;
}

HTTP_CONN
response_get_connection(RESPONSE this)
{
  return (HTTP_CONN)__int_value(this, CONNECTION, CLOSE);
}

BOOLEAN
response_set_keepalive(RESPONSE this, char *line)
{
  char *tmp     = "";
  char *option  = "";
  char *value   = "";
  char *newline = (char*)line;
  BOOLEAN res   = FALSE;

  while ((tmp = __parse_pair(&newline)) != NULL) {
    option = tmp;
    while (*tmp && !ISSPACE((int)*tmp) && !ISSEPARATOR(*tmp))
      tmp++;
    *tmp++=0;
    while (ISSPACE((int)*tmp) || ISSEPARATOR(*tmp))
      tmp++;
    value  = tmp;
    while (*tmp)
      tmp++;
    if (!strncasecmp(option, "timeout", 7)) {
      if (value == NULL){
        hash_add(this->headers, KEEPALIVE_TIMEOUT, "15");
      } else {
        int num = atoi(value);
        if (num > 0) {
          hash_add(this->headers, KEEPALIVE_TIMEOUT, value);
        }
      }
      res = TRUE;
    }
    if (!strncasecmp(option, "max", 3)) {
      if (value == NULL){
        hash_add(this->headers, KEEPALIVE_MAX, "15");
      } else {
        int num = atoi(value);
        if (num > 0) {
          hash_add(this->headers, KEEPALIVE_MAX, value);
        }
      }
      res = TRUE;
    }
  }
  return res;
}

int 
response_get_keepalive_timeout(RESPONSE this)
{
  return __int_value(this, KEEPALIVE_TIMEOUT, 15);
}

int
response_get_keepalive_max(RESPONSE this)
{
  return __int_value(this, KEEPALIVE_MAX, 5);
}

BOOLEAN
response_set_last_modified(RESPONSE this, char *line)
{
  int   len = 0;
  char *tmp = NULL;
 
  if (strncasecmp(line, LAST_MODIFIED, strlen(LAST_MODIFIED)) == 0) {
    len = strlen(line);
    tmp = xmalloc(len);
    memset(tmp, '\0', len);
    memcpy(tmp, line+15, len-14);
    tmp[len-15] = '\0';
    hash_add(this->headers, LAST_MODIFIED, (void*)tmp);
    xfree(tmp);
    return TRUE;
  }
  return FALSE;
}

char *
response_get_last_modified(RESPONSE this)
{
  return (char*)hash_get(this->headers, LAST_MODIFIED);
}

BOOLEAN
response_set_etag(RESPONSE this, char *line)
{
  int   len = 0;
  char *tmp = NULL;

  if (strncasecmp(line, ETAG, strlen(ETAG)) == 0) {
    len = strlen(line);
    tmp = xmalloc(len);
    memset(tmp, '\0', len);
    memcpy(tmp, line+6, len-5);
    tmp[len-6] = '\0';
    hash_add(this->headers, ETAG, (void*)__dequote(tmp));
    xfree(tmp);
    return TRUE;
  }
  return FALSE;
}

char *
response_get_etag(RESPONSE this)
{
  return (char*)hash_get(this->headers, ETAG);
}

BOOLEAN
response_set_www_authenticate(RESPONSE this, char *line)
{
  char *tmp     = "";
  char *option  = "";
  char *value   = "";
  char *newline = (char*)line;

  if (strncasecmp(line, WWW_AUTHENTICATE, strlen(WWW_AUTHENTICATE)) == 0) {
    if (strncasecmp(line+18, "digest", 6) == 0) {
      newline += 24;
      this->auth.type.www      = DIGEST;
      this->auth.challenge.www = xstrdup(line+18);
    } else if (strncasecmp(line+18, "ntlm", 4) == 0) {
      newline += 22;
      this->auth.type.www = NTLM;
      this->auth.challenge.www = xstrdup(line+18);
    } else {
      /**
       * XXX: If a server sends more than one www-authenticate header
       *      then we want to use one we've already parsed.
       */
      if (this->auth.type.www != DIGEST && this->auth.type.www != NTLM) {
        newline += 23;
        this->auth.type.www = BASIC;
      }
    }
    while ((tmp = __parse_pair(&newline)) != NULL) {
      option = tmp;
      while (*tmp && !ISSPACE((int)*tmp) && !ISSEPARATOR(*tmp))
        tmp++;
      *tmp++='\0';
      while (ISSPACE((int)*tmp) || ISSEPARATOR(*tmp))
        tmp++;
      value  = tmp;
      while (*tmp)
        tmp++;
      if (!strncasecmp(option, "realm", 5)) {
        if(value != NULL){
          this->auth.realm.www = xstrdup(__dequote(value));
          // XXX: url_set_realm(U, __dequote(value));
        } else {
          this->auth.realm.www = xstrdup("");
        }
      }
    } /* end of parse pairs */
  }
  return TRUE;
}

TYPE 
response_get_www_auth_type(RESPONSE this)
{
  return this->auth.type.www;
}

char *
response_get_www_auth_challenge(RESPONSE this)
{
  return this->auth.challenge.www;
}

char *
response_get_www_auth_realm(RESPONSE this)
{
  return this->auth.realm.www;
}

BOOLEAN
response_set_proxy_authenticate(RESPONSE this, char *line)
{
  char *tmp     = ""; 
  char *option  = "", *value = "";
  char *newline = (char*)line;

  if (strncasecmp(line, PROXY_AUTHENTICATE, strlen(PROXY_AUTHENTICATE)) == 0) {
    if (strncasecmp(line+20, "digest", 6) == 0){
      newline += 26;
      this->auth.type.proxy      = DIGEST;
      this->auth.challenge.proxy = xstrdup(line+20);
    } else {
      newline += 25;
      this->auth.type.proxy = BASIC;
    }
    while ((tmp = __parse_pair(&newline)) != NULL){
      option = tmp; 
      while(*tmp && !ISSPACE((int)*tmp) && !ISSEPARATOR(*tmp))
        tmp++;
      *tmp++='\0';
      while (ISSPACE((int)*tmp) || ISSEPARATOR(*tmp))
        tmp++; 
      value  = tmp;
      while(*tmp)
        tmp++;
      if (!strncasecmp(option, "realm", 5)) {
        if (value != NULL) {
          this->auth.realm.proxy = xstrdup(__dequote(value));
        } else {
          this->auth.realm.proxy = xstrdup("");
        }
      }
    } /* end of parse pairs */
  }
  return TRUE;
}

TYPE
response_get_proxy_auth_type(RESPONSE this)
{
  return this->auth.type.proxy;
}

char *
response_get_proxy_auth_challenge(RESPONSE this)
{
  return this->auth.challenge.proxy;
}

char *
response_get_proxy_auth_realm(RESPONSE this)
{
  return this->auth.realm.proxy;
}

private int
__int_value(RESPONSE this, char *key, int def)
{
  int num = -1;

  if ((char *)hash_get(this->headers, key) != NULL) {
    num = atoi((char *)hash_get(this->headers, key));
  }
  return (num > 0) ? num : def;
}

private BOOLEAN
__boolean_value(RESPONSE this, char *key, BOOLEAN def)
{
  BOOLEAN res = def;

  char *b = (char *)hash_get(this->headers, key);
  if (b == NULL) {
    res = def;
  } else if (strmatch(b, "true")) {
    res = TRUE;
  } else if (strmatch(b, "false")) {
    res = FALSE;
  } else {
    res = def;
  }
  return res; 
}


private char *
__parse_pair(char **str)
{
  int  okay  = 0;
  char *p    = *str;
  char *pair = NULL;

  if (!str || !*str) return NULL;
  /**
   * strip the header label
   */
  while (*p && *p != ' ')
    p++;

  *p++=0;
  if (!*p) {
    *str   = p;
    return NULL;
  }

  pair = p;
  while (*p && *p != ';' && *p != ',') {
    if (!*p) {
      *str = p;
      return NULL;
    }
    if (*p == '=') okay = 1;
    p++;
  }
  *p++ = 0;
  *str = p;

  return (okay) ? pair : NULL;
}

private char *
__rquote(char *str)
{
  char *ptr;
  int   len;

  len = strlen(str);
  for(ptr = str + len - 1; ptr >= str && ISQUOTE((int)*ptr ); --ptr);

  ptr[1] = '\0';

  return str;
}

private char *
__lquote(char *str)
{
  char *ptr;
  int  len;

  for(ptr = str; *ptr && ISQUOTE((int)*ptr); ++ptr);

  len = strlen(ptr);
  memmove(str, ptr, len + 1);

  return str;
}

private char *
__dequote(char *str)
{
  char *ptr;
  ptr = __rquote(str);
  str = __lquote(ptr);
  return str;
}
