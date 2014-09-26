/**
 * Memory handling
 * Library: joedog
 *
 * Copyright (C) 2000-2007 by
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
 */ 
#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>

/**
 * a controlled strdup 
 */
char * xstrdup(const char *str); 

/**
 * a controlled realloc
 */
void * xrealloc(void *, size_t);

/**
 * a controlled malloc
 */
void * xmalloc (size_t);

/**
 * a controlled calloc
 */
void * xcalloc (size_t, size_t);  

/**
 * a controlled free 
 */
void xfree(void *ptr);

#endif /* MEMORY_H */
