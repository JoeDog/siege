/**
 * HTTP Authentication
 *
 * Copyright (C) 2002-2014 by
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
#ifndef __AUTH_H
#define __AUTH_H

#include <creds.h>
#include <url.h>
#include <joedog/boolean.h>

typedef struct AUTH_T *AUTH;
extern  size_t AUTHSIZE;

typedef struct DIGEST_CRED DCRED;
typedef struct DIGEST_CHLG DCHLG;
typedef enum { BASIC, DIGEST, NTLM } TYPE;

AUTH    new_auth();
AUTH    auth_destroy(AUTH this);
void    auth_add(AUTH this, CREDS creds);
void    auth_display(AUTH this, SCHEME scheme);
char *  auth_get_basic_header(AUTH this, SCHEME scheme);
BOOLEAN auth_set_basic_header(AUTH this, SCHEME scheme, char *realm);
char *  auth_get_ntlm_header(AUTH this, SCHEME scheme);
BOOLEAN auth_set_ntlm_header(AUTH this, SCHEME scheme, char *header, char *realm);
char *  auth_get_digest_header(AUTH this, SCHEME scheme, DCHLG *chlg, DCRED *cred, const char *meth, const char *uri);
BOOLEAN auth_set_digest_header(AUTH this, DCHLG **ch, DCRED **cr, unsigned int *rand, char *realm, char *str);
BOOLEAN auth_get_proxy_required(AUTH this);
void    auth_set_proxy_required(AUTH this, BOOLEAN required);
char *  auth_get_proxy_host(AUTH this);
void    auth_set_proxy_host(AUTH this, char *host);
int     auth_get_proxy_port(AUTH this);
void    auth_set_proxy_port(AUTH this, int port);
char *  auth_get_ftp_username(AUTH this, char *realm);
char *  auth_get_ftp_password(AUTH this, char *realm);


#endif/*__AUTH_H*/
