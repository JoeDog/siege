/**
 * HTTP Response Headers
 *
 * Copyright (C) 2016 by
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
#ifndef __RESPONSE_H
#define __RESPONSE_H

#include <stdlib.h>
#include <auth.h>
#include <joedog/defs.h>
#include <joedog/boolean.h>

typedef enum { // HTTP connection
  CLOSE        = 1,
  KEEPALIVE    = 2,
  METER        = 4
} HTTP_CONN;

typedef enum { // transfer-encoding
  NONE         = 1,
  CHUNKED      = 2,
  TRAILER      = 4
} HTTP_TE;

typedef enum { // content-encoding
  COMPRESS     = 1,
  DEFLATE      = 2,
  GZIP         = 4,
  BZIP2        = 8
} HTTP_CE; 

#define ACCEPT_RANGES       "accept-ranges"
#define CACHE_CONTROL       "cache-control"
#define CHARSET             "charset"
#define CONNECTION          "connection"
#define CONTENT_DISPOSITION "content-disposition"
#define CONTENT_ENCODING    "content-encoding"
#define CONTENT_LENGTH      "content-length"
#define CONTENT_TYPE        "content-type"
#define ETAG                "etag"
#define EXPIRES             "expires"
#define KEEPALIVE_MAX       "keepalive-max"
#define KEEPALIVE_TIMEOUT   "keepalive-timeout"
#define LAST_MODIFIED       "last-modified"
#define LOCATION            "location"
#define PRAGMA              "pragma"
#define PROTOCOL            "protocol"
#define PROXY_AUTHENTICATE  "proxy-authenticate"
#define PROXY_CONNECTION    "proxy-connection"
#define REFRESH             "refresh"
#define REDIRECT            "redirect"
#define RESPONSE_CODE       "response-code"
#define SET_COOKIE          "set-cookie"
#define TRANSFER_ENCODING   "transfer-encoding"
#define WWW_AUTHENTICATE    "www-authenticate"

/**
 * Response object
 */
typedef struct RESPONSE_T *RESPONSE;

/**
 * For memory allocation; URLSIZE
 * provides the object size
 */
extern size_t  RESPONSESIZE;


RESPONSE  new_response();
RESPONSE  response_destroy(RESPONSE this);

BOOLEAN   response_set_code(RESPONSE this, char *line);
int       response_get_code(RESPONSE this);
char *    response_get_protocol(RESPONSE this);

void      response_set_from_cache(RESPONSE this, BOOLEAN cached);
BOOLEAN   response_get_from_cache(RESPONSE this);

int       response_success(RESPONSE this);
int       response_failure(RESPONSE this);

BOOLEAN   response_set_content_type(RESPONSE this, char *line);
char *    response_get_content_type(RESPONSE this);
char *    response_get_charset(RESPONSE this);

BOOLEAN   response_set_content_length(RESPONSE this, char *line);
int       response_get_content_length(RESPONSE this);

BOOLEAN   response_set_connection(RESPONSE this, char *line);
HTTP_CONN response_get_connection(RESPONSE this);

BOOLEAN   response_set_keepalive(RESPONSE this, char *line);
int       response_get_keepalive_timeout(RESPONSE this);
int       response_get_keepalive_max(RESPONSE this);

BOOLEAN   response_set_location(RESPONSE this, char *line);
char *    response_get_location(RESPONSE this);
BOOLEAN   response_get_redirect(RESPONSE this);

BOOLEAN   response_set_last_modified(RESPONSE this, char *line);
char *    response_get_last_modified(RESPONSE this);

BOOLEAN   response_set_etag(RESPONSE this, char *line);
char *    response_get_etag(RESPONSE this);

BOOLEAN   response_set_content_encoding(RESPONSE this, char *line);
HTTP_CE   response_get_content_encoding(RESPONSE this);

BOOLEAN   response_set_transfer_encoding(RESPONSE this, char *line);
HTTP_TE   response_get_transfer_encoding(RESPONSE this);

BOOLEAN   response_set_www_authenticate(RESPONSE this, char *line);
TYPE      response_get_www_auth_type(RESPONSE this);
char *    response_get_www_auth_challenge(RESPONSE this);
char *    response_get_www_auth_realm(RESPONSE this);

BOOLEAN   response_set_proxy_authenticate(RESPONSE this, char *line);
TYPE      response_get_proxy_auth_type(RESPONSE this);
char *    response_get_proxy_auth_challenge(RESPONSE this);
char *    response_get_proxy_auth_realm(RESPONSE this);

#endif/*__RESPONSE_H*/
