/**
 * Memory handling
 *
 * Copyright (C) 2000-2016 by
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

char * xstrdup(const char *str); 
char * xstrcat(const char *arg1, ...);
char * xstrncat(char *dest, const char *src, size_t len);
char * xstrncpy(char* dest, const char *src, size_t len);
void * xrealloc(void *, size_t);
void * xmalloc (size_t);
void * xcalloc (size_t, size_t);  
void xfree(void *ptr);

#endif /* MEMORY_H */
