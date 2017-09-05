/**
 * Perly functions
 * Library: joedog
 * 
 * Copyright (C) 2000-2007 by
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <perl.h>
#include <ctype.h>

#define SPLITSZ 4096 

/**
 * not quite perl chomp, this function
 * hacks the newline off the end of a 
 * string.
 */
char *
chomp(char *str)
{
  if(*str && str[strlen(str)-1]=='\n') str[strlen(str)-1] = 0;
  return str;
}

/**
 * rtrim
 */
char *
rtrim(char *str)
{
  char *ptr;
  int   len;
 
  if (str == NULL) { 
    return NULL;
  }
 
  for (ptr = str; *ptr && isspace((int)*ptr); ++ptr);
 
  len = strlen(str);
  for (ptr = str + len - 1; ptr >= str && isspace((int)*ptr ); --ptr);
  
  ptr[1] = '\0';
 
  return str;
}

/**
 * ltrim: trim white space off left of str 
 */ 
char *
ltrim(char *str)
{
  char *ptr;
  int  len;
  
  if (str == NULL) { 
    return NULL;
  }
 
  for (ptr = str; *ptr && isspace((int)*ptr); ++ptr);
 
  len = strlen(ptr);
  memmove(str, ptr, len + 1);
 
  return str;
}

/**
 * trim: calls ltrim and rtrim
 */ 
char *
trim(char *str)
{
  char *ptr;
  if (str == NULL) return NULL;
  ptr = rtrim(str);
  str = ltrim(ptr);
  return str;
} 

int
valid(const char *s)
{
 int flag=0;
 int i = 0;

 for(i = 0; i <= 255; i++){
    flag = flag || s[i]=='\0';
 }

  if(flag){
    return 1;
  } else {
    return 0;
  }
}

BOOLEAN 
empty(const char *s)
{
  if(!s) return 1;
  if(strlen(s) < 1) return 1;

  while ((ISSPACE(*s)))
     s++;
  return (*s == '\0');
}


int
word_count(char pattern, char *s)
{
  int in_word_flag = 0;
  int count = 0;
  char *ptr;
 
  ptr = s;
  while(*ptr){
    if((*ptr) != pattern){
      if(in_word_flag == 0)
        count++;
      in_word_flag = 1;
    } else {
      in_word_flag = 0;
    }
    ptr++;
  }
  return count;
} 

char **
split(char pattern, char *s, int *n_words)
{
  char **words;
  char *str0, *str1;
  int i;
  
  *n_words = word_count(pattern, s);
  if( *n_words == 0 )
    return NULL;
 
  words = xmalloc(*n_words * sizeof (*words));
  if(!words)
    return NULL;

  str0 = s;
  i = 0;
  while(*str0){
    size_t len;
    str1 = strchr(str0, pattern);
    if(str1 != NULL){
      len = str1 - str0;
    } else {
      len = strlen(str0);
    }

    /**
     * if len is 0 then str0 and str1 match
     * which means the string begins with a
     * separator. we don't want to allocate 
     * memory for an empty string, we just want 
     * to increment the pointer. on 0 we decrement 
     * i since it will be incremented below...
     */
    if(len == 0){
       i--; 
    } else {
      words[i] = (char*)xmalloc(SPLITSZ);
      memset(words[i], '\0', SPLITSZ); 
      memcpy(words[i], (char*)str0, SPLITSZ); 
      words[i][len] = '\0';
    }

    if(str1 != NULL){
      str0 = ++str1;
    } else {
      break;
    }
    i++;
  }
  return words;
} 

void
split_free(char **split, int length)
{
  int x;
  for(x = 0; x < length; x ++){
    if( split[x] != NULL ){
      char *tmp = split[x];
      xfree(tmp);
    }
  }
  free(split);
 
  return;
} 
