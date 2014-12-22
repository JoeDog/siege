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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <array.h>
#include <joedog/joedog.h>

typedef void *array;

struct ARRAY_T
{
  int    index;
  int    length;
  array  *data;
};

size_t ARRAYSIZE = sizeof(struct ARRAY_T);

ARRAY
new_array(){
  ARRAY this;

  this = xcalloc(sizeof(struct ARRAY_T), 1);
  this->index  = -1;
  this->length =  0;
  return this;
}

ARRAY
array_destroy(ARRAY this) 
{
  int i;

  for (i = 0; i < this->length; i++) {
    xfree(this->data[i]);  
  } 
  xfree(this->data);
  xfree(this);
  this = NULL;
  return this; 
}

void 
array_push(ARRAY this, void *thing)
{
  int len = 0;

  if (thing==NULL) return;

  len = strlen(thing)+1;
  array_npush(this, (void*)thing, len);
  return;
}

void
array_npush(ARRAY this, void *thing, size_t len) 
{
  array arr;
  if (thing==NULL) return;
  if (this->length == 0) {
    this->data = xmalloc(sizeof(array));
  } else {
    this->data = realloc(this->data,(this->length+1)*sizeof(array)); 
  }
  arr = xmalloc(len+1); 
  memset(arr, '\0', len+1);
  memcpy(arr, thing, len);
  this->data[this->length] = arr;
  this->length += 1;
  return;
}

void *
array_get(ARRAY this, int index)
{
  if (index > this->length) return NULL;

  return this->data[index];
}

void *
array_next(ARRAY this) 
{
  this->index++;
  return this->data[(this->index) % this->length]; 
}

void *
array_prev(ARRAY this) 
{
  this->index--;
  return this->data[((this->index) + (this->length - 1)) % this->length] ;  
}

size_t
array_length(ARRAY this)
{
  return this->length; 
}

char *
array_to_string(ARRAY this)
{
  size_t i;
  int    len = 0;
  char  *str;

  for (i = 0; i < array_length(this); i++) {
    len += strlen(array_get(this, i))+3;
  }
  str = (char*)malloc(len+1);
  memset(str, '\0', len+1);

  for (i = 0; i < array_length(this); i++) {
    strcat(str, "[");
    strcat(str, array_get(this, i));
    if (i == array_length(this) - 1) {
      strcat(str, "]");
    } else {
      strcat(str, "],");
    }
  }
  return str;
}
