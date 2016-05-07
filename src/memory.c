/**
 * Memory handling
 * 
 * Copyright (C) 2000-2016 by
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
 * --
 *
 */
#include <config.h>
#include <memory.h>
#include <notify.h>
#include <string.h>
#include <stdarg.h>

char *
xstrdup(const char *str)
{
  register char *ret;
#ifndef HAVE_STRDUP
  register size_t len; 
#endif/*HAVE_STRDUP*/

  if(!str){
    NOTIFY(ERROR, "string has no value!");
    return NULL;
  }
#ifdef HAVE_STRDUP
  ret = strdup(str);
  if(ret==NULL) NOTIFY(FATAL, "xstrdup: unable to allocate additional memory"); 
#else
  len = strlen(str)+1;
  ret = malloc(len);
  if (ret==NULL) {
    NOTIFY(FATAL, "xstrdup: unable to allocate additional memory");
  }
  memcpy(ret, str, len);
#endif
  return ret; 
}

char *
xstrcat(const char *arg1, ...)
{
  const char *argptr;
  char *resptr, *result;
  size_t  len = 0;
  va_list valist;

  va_start(valist, arg1);

  for(argptr = arg1; argptr != NULL; argptr = va_arg(valist, char *))
    len += strlen(argptr);

  va_end(valist);

  result = xmalloc(len + 1);
  resptr = result;

  va_start(valist, arg1);

  for(argptr = arg1; argptr != NULL; argptr = va_arg(valist, char *)) {
    len = strlen(argptr);
    memcpy(resptr, argptr, len);
    resptr += len;
  }

  va_end(valist);

  *resptr = '\0';

  return result;
}

/**
 * xrealloc: value added realloc
 */
void * 
xrealloc(void *ptr, size_t size)
{
  void *tmp;
  if (ptr) {
    tmp = realloc(ptr, size);
  } else {
    tmp = malloc(size);
  }
  if (tmp==NULL) NOTIFY(FATAL, "Memory exhausted; unable to continue.");
  return tmp;
}

/**
 * xmalloc: value-added malloc
 */
void *
xmalloc(size_t size)
{
  void *tmp = malloc(size);
  if(tmp==NULL) NOTIFY(FATAL, "Unable to allocate additional memory.");
  return tmp;
} 

/**
 * xcalloc     replaces calloc
 */
void *
xcalloc(size_t num, size_t size)
{
  void *tmp  =  xmalloc(num * size);
  memset(tmp, 0, (num * size));
  return tmp;
} 


/** 
 * free() wrapper:
 * free it and NULL it to ensure we
 * don't free it a second time...
 */
void
xfree(void *ptr)
{
  if(ptr!=(void *)NULL){
    free(ptr); ptr=(void *)NULL;
  }
}

