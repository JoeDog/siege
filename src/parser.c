#include <url.h>
#include <parser.h>
#include <util.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <array.h>
#include <pthread.h>
#include <joedog/joedog.h>
#include <joedog/defs.h>

#define CONTROL_TOKENS      " ="
#define CONTROL_TOKENS_PLUS " =\"\'"

struct PARSER_T
{
  size_t  length;
  char *  page;
  ARRAY   urls;
  URL     base;
};

size_t PARSERSIZE = sizeof(struct PARSER_T);

private BOOLEAN __parse(PARSER this, char *page);
private void    __parse_control(PARSER this, char *html);
private void    __add_url(PARSER this, URL U);
private char *  __strcasestr(const char *s, const char *find);

pthread_mutex_t parser_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  parser_cond  = PTHREAD_COND_INITIALIZER;

PARSER
new_parser(URL base, char *page){
  PARSER this = NULL;

  if (base == NULL) {
    return NULL;
  }
  pthread_mutex_lock(&parser_mutex);
  this = xcalloc(PARSERSIZE, 1);
  this->length = 0;
  this->base   = base;
  this->urls   = new_array();
  __parse(this, page);
  pthread_mutex_unlock(&parser_mutex);

  return this;
}

PARSER
parser_destroy(PARSER this)
{
  //this->urls = array_destroy(this->urls);
  URL u;
  while ((u = (URL)array_pop(this->urls)) != NULL) {
    u = url_destroy(u);
  }
  xfree(this->page);
  xfree(this);
  this = NULL;
  return this;
}

void
parser_reset(PARSER this, URL base)
{
  this->urls = array_destroy(this->urls);
  this->urls = new_array();
  this->base = url_destroy(this->base);
  this->base = base;
}

ARRAY
parser_get(PARSER this) 
{
  if (this != NULL && array_length(this->urls) > 0) {
    return this->urls;
  } else {
    return NULL;
  }
}

#define BUFSZ 4096

private BOOLEAN
__parse(PARSER this, char *page)
{
  char *ptr;
  int  i;
  char tmp[BUFSZ];
  memset(tmp, '\0', BUFSZ);
  ptr = page; 
  
  if (page == NULL) return FALSE;
  if (strlen(page) < 1) return FALSE;

  while (*ptr != '\0') {
    if (*ptr == '<') {
      ptr++;
      if (startswith("!--", ptr) == TRUE)  {
        ptr += 3;
        while (*ptr!='\0') {
          if (startswith("-->", ptr) == TRUE) {
            ptr += 3;
            break;
          }
          ptr++;
        }
      } else {
        i = 0;
        memset(tmp, '\0', sizeof(tmp));
        while (*ptr != '\0' && *ptr != '>' && i < (BUFSZ-1)) {
          tmp[i] = *ptr;
          i++;
          ptr++;
        }
        __parse_control(this, tmp);
      }
    }
    ptr++;
  }
  return TRUE;
}

private void 
__add_url(PARSER this, URL U)
{
  if (U != NULL) {
    array_npush(this->urls, U, URLSIZE);
  }
  return;
}

/**
 * The following code is based on parse_control from LinkCheck
 * by Ken Jones <kbo@inter7.com>
 *
 * Copyright (C) 2000 Inter7 Internet Technologies, Inc.
 * Copyright (C) 2013 Jeffrey Fulmer, et al
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 */
private void
__parse_control(PARSER this, char *html) 
{
  char * ptr = NULL;
  char * aid;
  char   tmp[BUFSZ];

  ptr = strtok_r(html, CONTROL_TOKENS, &aid);
  while (ptr != NULL) {
    if (strncasecmp(ptr, "href", 4) == 0) {
      ptr = strtok_r(NULL, CONTROL_TOKENS_PLUS, &aid);
      if (ptr != NULL) {
        memset(tmp, '\0', BUFSZ);
        strncpy(tmp, ptr, BUFSZ-1);
      }
    } else if (strncasecmp(ptr, "meta", 4) == 0) {
      for (ptr = strtok_r(NULL, CONTROL_TOKENS, &aid); ptr != NULL; ptr = strtok_r(NULL, CONTROL_TOKENS, &aid)) {
        if (strncasecmp(ptr, "content", 7) == 0) {        
          for (ptr = strtok_r(NULL, CONTROL_TOKENS, &aid); ptr != NULL; ptr = strtok_r(NULL, CONTROL_TOKENS, &aid)) {  
            if (__strcasestr(ptr, "url") != NULL) {
              ptr = strtok_r(NULL, CONTROL_TOKENS_PLUS, &aid);
              if (ptr != NULL) {
                URL U = url_normalize(this->base, ptr);
                __add_url(this, U);
                //url_destroy(U);
              }
            }
          }
        }
      }
    } else if (strncasecmp(ptr, "img", 3) == 0) {
      ptr = strtok_r(NULL, CONTROL_TOKENS, &aid);
      if (ptr != NULL) {
        if (! strncasecmp(ptr, "src", 3)) {
          ptr = strtok_r(NULL, CONTROL_TOKENS_PLUS, &aid);
          if (ptr != NULL) { 
            URL U = url_normalize(this->base, ptr);
            __add_url(this, U);
            //url_destroy(U);
          }
        } else {
          for (ptr = strtok_r(NULL, CONTROL_TOKENS, &aid); ptr != NULL; ptr = strtok_r(NULL, CONTROL_TOKENS, &aid)) {
            if ((ptr != NULL) && (strncasecmp(ptr, "src", 3) == 0)) {        
              ptr = strtok_r(NULL, CONTROL_TOKENS_PLUS, &aid);
              if (ptr != NULL) { 
                URL U = url_normalize(this->base, ptr);
                __add_url(this, U);
                //url_destroy(U);
              }
            } 
          }
        }
      }

    } else if (strncasecmp(ptr, "link", 4) == 0) {
      /*
      <link rel="stylesheet" type="text/css" href="/wp-content/themes/joedog/style.css" />
      <meta name="verify-v1" content="T3mz6whWX6gK4o2ptN99TNTakYMe7InrFRkBqqi/6XI=" />
      <link href="https://plus.google.com/u/0/102619614955071602341" rel="author" />
      */
      BOOLEAN okay = FALSE;
      char buf[2048]; //XXX: TEMP!!!!! make dynamic
      for (ptr = strtok_r(NULL, CONTROL_TOKENS, &aid); ptr != NULL; ptr = strtok_r(NULL, CONTROL_TOKENS, &aid)) {
        if (strncasecmp(ptr, "rel", 3) == 0) {
          ptr = strtok_r(NULL, CONTROL_TOKENS_PLUS, &aid);
          if (strncasecmp(ptr, "stylesheet", 10) ==0) {
            okay = TRUE;
          }  
        } 
        if (strncasecmp(ptr, "href", 4) == 0) {
          ptr = strtok_r(NULL, CONTROL_TOKENS_PLUS, &aid);
          if (ptr != NULL) {
            memset(buf, '\0', sizeof(buf));
            strncpy(buf, ptr, strlen(ptr));
          }
        }
      }
      if (okay) {
        URL U = url_normalize(this->base, buf);
        __add_url(this, U);
      }
    } else if (strncasecmp(ptr, "script", 6) == 0) {
      for (ptr = strtok_r(NULL, CONTROL_TOKENS, &aid); ptr != NULL; ptr = strtok_r(NULL, CONTROL_TOKENS, &aid)) {
        if (strncasecmp(ptr, "src", 3) == 0) {
          ptr = strtok_r(NULL, CONTROL_TOKENS_PLUS, &aid);
          if (ptr != NULL) {
            memset(tmp, 0, BUFSZ);
            strncpy(tmp, ptr, BUFSZ-1);
            URL U = url_normalize(this->base, tmp);
            __add_url(this, U);
          }
        }
      }
    } else if (strncasecmp(ptr, "location.href", 13) == 0) {
      ptr = strtok_r(NULL, CONTROL_TOKENS_PLUS, &aid);
      if (ptr != NULL ) {
        memset(tmp, '\0', BUFSZ);
        strncpy(tmp, ptr, BUFSZ-1);
      }
    } else if (strncasecmp(ptr, "frame", 5) == 0) {
      ptr = strtok_r(NULL, CONTROL_TOKENS, &aid);
      while (ptr != NULL) {
        if (strncasecmp(ptr, "src", 3) == 0) {
          ptr = strtok_r(NULL, CONTROL_TOKENS_PLUS, &aid);
          if (ptr != NULL) {
            memset(tmp, '\0', BUFSZ);
            strncpy(tmp, ptr, BUFSZ-1);
          }
        }
        ptr = strtok_r(NULL, CONTROL_TOKENS, &aid);
      }
    } else if (strncasecmp(ptr, "background", 10) == 0) {
      ptr = strtok_r(NULL, CONTROL_TOKENS_PLUS, &aid);
      if (ptr != NULL) {
        memset(tmp, 0, BUFSZ);
        strncpy(tmp, ptr, BUFSZ-1);
      }
    }
    ptr = strtok_r(NULL, CONTROL_TOKENS, &aid);
  }
}

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * This code was altered by Jeffrey Fulmer. The original is here:
 * http://opensource.apple.com/source/Libc/Libc-391.4.1/string/FreeBSD/strcasestr.c
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

private char *
__strcasestr(const char *s, const char *find)
{
  char c, sc;
  size_t len;

  if ((c = *find++) != 0) {
    c = tolower((unsigned char)c);
    len = strlen(find);
    do {
      do {
        if ((sc = *s++) == 0)
          return (NULL);
      } while ((char)tolower((unsigned char)sc) != c);
    } while (strncasecmp(s, find, len) != 0);
    s--;
  }
  return ((char *)s);
}


