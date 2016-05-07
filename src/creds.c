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
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <joedog/defs.h>
#include <joedog/boolean.h>
#include <creds.h>

struct CREDS_T {
  SCHEME  scheme;
  char   *username;
  char   *password;
  char   *realm;
};

size_t CREDSIZE = sizeof(struct CREDS_T);

private void __parse_input(CREDS this, char *str);


CREDS
new_creds(SCHEME scheme, char *str)
{
  CREDS this;

  this = calloc(sizeof(struct CREDS_T), 1);
  this->scheme = scheme;
  __parse_input(this, str);
  return this;
}

CREDS
creds_destroy(CREDS this)
{
  free(this->username);
  free(this->password);
  free(this->realm);
  free(this);
  return NULL;
}

SCHEME
creds_get_scheme(CREDS this)
{
  return this->scheme;
}

char *
creds_get_username(CREDS this)
{
  return this->username; 
}

char *
creds_get_password(CREDS this)
{
  return this->password; 
}

char *
creds_get_realm(CREDS this)
{
  return this->realm; 
}

void
creds_set_username(CREDS this, char *username)
{
  size_t len = strlen(username);

  this->username = malloc(len+1);
  memset(this->username, '\0', len+1);
  memcpy(this->username, username, len);
  return;
}

void
creds_set_password(CREDS this, char *password)
{
  size_t len = strlen(password);

  this->password = malloc(len+1);
  memset(this->password, '\0', len+1);
  memcpy(this->password, password, len);
  return;
}

void
creds_set_realm(CREDS this, char *realm)
{
  size_t len = strlen(realm);

  this->realm = malloc(len+1);
  memset(this->realm, '\0', len+1);
  memcpy(this->realm, realm, len);
  return;
}


private void
__parse_input(CREDS this, char *str) 
{
  char *usr; 
  char *pwd; 
  char *rlm; 
  char *tmp;
  char any[] = "any\0";

  usr = tmp = str;
  while (*tmp && *tmp != ':' && *tmp != '\0')
    tmp++;

  *tmp++=0; 
  pwd = tmp;
  while (*tmp && *tmp != ':' && *tmp != '\0')
    tmp++;

  if ('\0' != *tmp) {
    *tmp++=0;
    rlm = tmp;
  } else {
    rlm = NULL;
  }

  creds_set_username(this, usr);
  creds_set_password(this, pwd);
  creds_set_realm(this, (rlm==NULL)?any:rlm);
}

