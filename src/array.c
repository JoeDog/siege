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
#include <memory.h>
#include <setup.h>

typedef void *array;

struct ARRAY_T
{
  int     index;
  int     length;
  array * data;
  method  free;
};

size_t ARRAYSIZE = sizeof(struct ARRAY_T);

ARRAY
new_array()
{
  ARRAY this;

  this = xcalloc(sizeof(struct ARRAY_T), 1);
  this->index  = -1;
  this->length =  0;
  this->free   = NULL;
  return this;
}

ARRAY
array_destroy(ARRAY this) 
{
  int i;
  
  if (this == NULL) return NULL;
  
  if (this->free == NULL) {
    this->free = free;
  }

  for (i = 0; i < this->length; i++) {
    this->free(this->data[i]);  
  } 
  xfree(this->data);
  xfree(this);
  this = NULL;
  return this; 
}

ARRAY
array_destroyer(ARRAY this, method m)
{
  this->free = m;
  return array_destroy(this);
}

void
array_set_destroyer(ARRAY this, method m)
{
  this->free = m;
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
  if (this->data == NULL && this->length == 0) {
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
array_remove (ARRAY this, int index) {
  int   length = 0;
  array arr;

  if (index > this->length) return NULL;

  arr    = this->data[index];
  length = --this->length;

  for (; index < length; index++) {
    this->data[index] = this->data[index+1];
  }

  return arr;
}

void *
array_pop(ARRAY this) 
{
  if (this == NULL) return NULL;
  return this->length ? this->data[--this->length] : NULL; 
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

  if (this->length == 0) return "NULL";

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

void
array_print(ARRAY this)
{
  printf("%s\n", array_to_string(this));
}


#if 0
#include <unistd.h>
int
main (int argc, char *argv[])
{
  int   x;
  int   i;
  int   len;
  ARRAY A = new_array();
  void *v; 
for (i = 0; i < 1000000; i++) {
  array_push(A, "Zero");
  array_push(A, "One");
  array_push(A, "Two");
  array_push(A, "Three");
  array_push(A, "Four");
  array_push(A, "Five");
  array_push(A, "Six-six-six");

  while ((v = array_pop(A)) != NULL) {
    printf("popped: %s\n", (char*)v);
    xfree(v);
  } 

}
  A = array_destroy(A);
  return 0;
}
#endif


