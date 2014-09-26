/**
 * Stralloc
 * Library: joedog
 * 
 * Copyright (C) 2000-2009 by
 * Jeffrey Fulmer - <jeff@joedog.org>
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

#ifdef  HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/ 

#ifdef  HAVE_STRING_H
# include <string.h>
#endif/*HAVE_STRING_H*/

#ifdef  HAVE_STRINGS_H
# include <strings.h>
#endif/*HAVE_STRINGS_H*/

#include <stdlib.h>
#include <notify.h>

char *stralloc(char *str)
{ 
  char *retval;
 
  retval=calloc(strlen(str)+1, 1);
  if(!retval) {
    NOTIFY(FATAL, "Fatal memory allocation error");
  }
  strcpy(retval, str);
  return retval;
} 

