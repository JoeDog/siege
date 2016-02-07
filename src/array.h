/**
 * Dynamic Array
 *
 * Copyright (C) 2006-2014 by
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
#ifndef ARRAY_H
#define ARRAY_H

#include <stdlib.h>
#include <joedog/defs.h>

/**
 * ARRAY object
 */
typedef struct ARRAY_T *ARRAY;
extern  size_t ARRAYSIZE;

ARRAY  new_array();
ARRAY  array_destroy(ARRAY this);
ARRAY  array_destroyer(ARRAY this, method m);
void   array_set_destroyer(ARRAY this, method m);
void   array_push(ARRAY this, void *thing);
void   array_npush(ARRAY this, void *thing, size_t len);
void * array_get(ARRAY this, int index);
void * array_remove (ARRAY this, int index);
void * array_pop(ARRAY this);
void * array_next(ARRAY this);
void * array_prev(ARRAY this);
size_t array_length(ARRAY this);
char * array_to_string(ARRAY this);

#endif/*ARRAY_H*/

