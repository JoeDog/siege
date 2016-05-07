/**
 * Thread pool
 *
 * Copyright (C) 2003-2013 by
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
#ifndef __CREW_H
#define __CREW_H

#include <pthread.h>
#include <joedog/boolean.h>

typedef struct work
{
  void          (*routine)();
  void          *arg;
  struct work   *next;
} WORK;

typedef struct CREW_T *CREW;

CREW    new_crew(int size, int maxsize, BOOLEAN block);
BOOLEAN crew_add(CREW this, void (*routine)(), void *arg); 
BOOLEAN crew_cancel(CREW this);
BOOLEAN crew_join(CREW this, BOOLEAN finish, void **payload);
void    crew_destroy(CREW this);

void    crew_set_shutdown(CREW this, BOOLEAN shutdown);

int     crew_get_size(CREW this);
int     crew_get_total(CREW this);
BOOLEAN crew_get_shutdown(CREW this);

#endif/*__CREW_H*/
