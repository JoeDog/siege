/**
 * Client Header
 *
 * Copyright (C) 2013-2014 by
 * Jeffrey Fulmer, et al - <jeff@joedog.org>
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
#ifndef CLIENT_H
#define CLIENT_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <setup.h>
#include <joedog/joedog.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef HAVE_SSL
# include <openssl/ssl.h>
# include <openssl/err.h> 
#endif /* HAVE_SSL */

#include <auth.h>
#include <hash.h>

struct trans
{
  int   bytes;
  int   code;
  float time;
};

/**
 * client data
 */
typedef struct
{
  int      id;
  size_t   tid;
  unsigned long  hits;
  unsigned long  bytes;
  unsigned int   code;
  unsigned int   fail;
  unsigned int   ok200;
  ARRAY  urls;
  HASH   cookies;
  struct {
    DCHLG *wchlg;
    DCRED *wcred;
    int    www;
    DCHLG *pchlg;
    DCRED *pcred;
    int  proxy;
    struct {
      int  www;
      int  proxy;
    } bids;
    struct {
      TYPE www;
      TYPE proxy;
    } type;
  } auth;
  int      status;
  float    time;
  float    himark;
  float    lomark;
  unsigned int rand_r_SEED;
} CLIENT;

void * start_routine(CLIENT *client);

#endif/*CLIENT_H */
