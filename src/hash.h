/**
 * Hash Table
 *
 * Copyright (C) 2003-2014 by
 * Jeffrey Fulmer - <jeff@joedog.org>
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
#ifndef HASH_H
#define HASH_H

#include <stdio.h>
#if defined (__FreeBSD__)
# include <unistd.h>
#endif

#include <joedog/boolean.h>

typedef struct HASH_T *HASH;

HASH     new_hash();
void     hash_add(HASH this, char *key, char *value);
char *   hash_get(HASH this, char *key);
char **  hash_get_keys(HASH this);
BOOLEAN  hash_lookup(HASH this, char *key);
void     hash_destroy(HASH this);
void     hash_free_keys(HASH this, char **keys);
int      hash_get_entries(HASH this);
void     hoh_add(HASH this, char *key, char *k, char *v);
HASH     hoh_get(HASH this, char *key);

#endif/*HASH_H*/
