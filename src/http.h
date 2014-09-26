/**
 * SIEGE http header file
 *
 * Copyright (C) 2000-2014 by Jeffrey Fulmer <jeff@joedog.org>, et al
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
#ifndef HTTP_H
#define HTTP_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <sock.h>
#include <cookie.h>
#include <auth.h>
#include <url.h>
#include <joedog/joedog.h>
#include <joedog/boolean.h>

#ifndef HAVE_SNPRINTF
# define portable_snprintf  snprintf
# define portable_vsnprintf vsnprintf
#endif

#define REQBUF  43008
#define POSTBUF 63488

/**
 * HTTP headers structure.
 */
typedef 
struct headers
{
  char                head[64];                /* http response         */
  int                 code;                    /* http return code      */
  unsigned long int   length;
  char                cookie[MAX_COOKIE_SIZE]; /* set-cookie data       */
  char                *redirect;               /* redirection URL       */
  struct{
    int   www;                                 /* www auth challenge    */
    int   proxy;                               /* proxy auth challenge  */
    struct {
      char  *www;                              /* www auth realm        */
      char  *proxy;                            /* proxy auth realm      */
    } realm;
    struct {
      char  *www;                              /* www auth realm        */
      char  *proxy;                            /* proxy auth realm      */
    } challenge;                               /* square peg/round hole */
    struct {
      TYPE  www;
      TYPE  proxy;
    } type;
  } auth;
  int                 keepalive;               /* http keep-alive       */
} HEADERS;

/* http function prototypes */
BOOLEAN  http_get (CONN *C, URL U);
BOOLEAN  http_post(CONN *C, URL U);
void     http_free_headers(HEADERS *h);
HEADERS *http_read_headers(CONN *C, URL U);
ssize_t  http_read(CONN *C);
BOOLEAN  https_tunnel_request(CONN *C, char *host, int port);
int      https_tunnel_response(CONN *C);

#endif /* HTTP_H */

