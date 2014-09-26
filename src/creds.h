/**
 * HTTP authentication credentials
 *
 * Copyright (C) 2000-2014 by
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
#ifndef __CREDS_H
#define __CREDS_H

#include <url.h>

typedef struct CREDS_T *CREDS;
extern  size_t CREDSIZE;

CREDS  new_creds(SCHEME scheme, char *str);
CREDS  creds_destroy(CREDS this);
SCHEME creds_get_scheme(CREDS this);
char  *creds_get_username(CREDS this);
char  *creds_get_password(CREDS this);
char  *creds_get_realm(CREDS this);
void   creds_set_username(CREDS this, char *username);
void   creds_set_password(CREDS this, char *password);
void   creds_set_realm(CREDS this, char *realm);

#endif/*__CREDS*/

