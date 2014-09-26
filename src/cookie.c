/**
 * Cookies Support
 *
 * Copyright (C) 2000-2014 by
 * Jeffrey Fulmer - <jeff@joedog.org>, et al. 
 * Copyright (C) 2002 the University of Kansas
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
 */ 
#include <setup.h>
#include <cookie.h>
#include <date.h>
#include <util.h>

typedef struct 
{
  char*  name;
  char*  value;
  char*  domain;
  char*  path;
  time_t expires;
  int    secure;
} PARSED_COOKIE;

void
free_cookie(PARSED_COOKIE* ck)
{
  xfree(ck->name);
  xfree(ck->value);
  xfree(ck->domain);
  xfree(ck->path);
}

COOKIE *cookie;

void
parse_cookie(char *cookiestr, PARSED_COOKIE* ck)
{
  char *lval, *rval;
  if (cookiestr==NULL || ck==NULL) return;

  ck->name = ck->value = ck->domain = ck->path = NULL;
  ck->expires = ck->secure = 0;

  lval = cookiestr;

  while (*cookiestr && *cookiestr != '=')
    cookiestr++;

  if (!*cookiestr) /*WTF?*/ return;

  *cookiestr++ = 0; /* NULL-terminate lval (replace '=') and position at start of rval */

  rval = cookiestr;
  while (*cookiestr && *cookiestr != ';')
    cookiestr++;

  *cookiestr++ = 0;

  if (lval != NULL) debug ("%s:%d accepting cookie name:  %s", __FILE__, __LINE__, lval);
  if (rval != NULL) debug ("%s:%d accepting cookie value: %s", __FILE__, __LINE__, rval);
  ck->name  = (lval != NULL) ? xstrdup(lval) : NULL;
  ck->value = (rval != NULL) ? xstrdup(rval) : NULL; 
  /* get the biggest possible positive value */
  ck->expires = 0;
  ck->expires = ~ck->expires;
  if (ck->expires < 0) {
    ck->expires = ~(1UL << ((sizeof(ck->expires) * 8) - 1));
  }
  if(ck->expires < 0) {
    ck->expires = (ck->expires >> 1) * -1;
  }
  
  if (!*cookiestr) { 
    /** 
     revert to defaults 
     XXX: assumes at least one key value pair
     */ 
    return; 
  }

  while (*cookiestr) {
    while (isspace((unsigned char)*cookiestr))
      cookiestr++;

    if (!*cookiestr) break;

    lval = cookiestr;
    while (*cookiestr && *cookiestr != '=' )
      cookiestr++;

    if (!strcasecmp (lval, "secure")) {
      ck->secure = 1;
      rval = NULL;
    } else {
      if (!*cookiestr) return; 

      *cookiestr++ = 0;

      rval = cookiestr;
      while (*cookiestr && *cookiestr != ';')
        cookiestr++;
      *cookiestr++ = 0;
    }

    if (!strcasecmp(lval, "domain")) {
      ck->domain = (rval != NULL) ? xstrdup( rval ) : NULL; 
    } else if (!strcasecmp (lval, "expires")) {
      ck->expires = strtotime(rval);
    } else if (!strcasecmp (lval, "path")) {
      ck->path = (rval != NULL) ? xstrdup( rval ) : NULL; 
    }
  }
  return;
}

/**
 * insert values into list
 */
int
add_cookie(pthread_t id, char *host, char *cookiestr)
{
  char *name, *value;
  int  found = FALSE;
  CNODE *cur, *pre, *fresh = NULL; 
  PARSED_COOKIE ck;

  parse_cookie(cookiestr, &ck);
  name = ck.name;
  value = ck.value;

  if ((name == NULL || value == NULL)) return -1;

  pthread_mutex_lock(&(cookie->mutex)); 
  for (cur=pre=cookie->first; cur != NULL; pre=cur, cur=cur->next) {
    if ((cur->threadID == id )&&(!strcasecmp(cur->name, name))) {
      xfree(cur->value);
      cur->value = xstrdup(value);
      /**
       XXX: I need to read the RFC in order to 
            understand the required behavior
      xfree(cur->name);
      xfree(cur->domain); 
      cur->name  = xstrdup(name);
      cur->expires = ck.expires;
      if(!ck.domain)
        cur->domain = xstrdup(host);
      else
        cur->domain = xstrdup(ck.domain); 
      */
      found = TRUE;
      break;
    }
  }
  if (!found) {
    fresh = (CNODE*)xmalloc(sizeof(CNODE));
    if (!fresh) NOTIFY(FATAL, "out of memory!"); 
    fresh->threadID = id;
    fresh->name     = xstrdup(name);
    fresh->value    = xstrdup(value);
    fresh->expires  = ck.expires; 
    if (!ck.domain)
      fresh->domain = xstrdup(host);
    else
      fresh->domain = xstrdup(ck.domain);
    fresh->next = cur;
    if (cur==cookie->first)
      cookie->first = fresh;
    else
      pre->next = fresh;    
  }
  if (name  != NULL) xfree(name);
  if (value != NULL) xfree(value);

  pthread_mutex_unlock(&(cookie->mutex));

  return 0;
}

BOOLEAN
delete_cookie(pthread_t id, char *name)
{
  CNODE  *cur, *pre;
  BOOLEAN res = FALSE;

  for (cur=pre=cookie->first; cur != NULL; pre=cur, cur=cur->next) {
    if (cur->threadID == id) {
      if (!strcasecmp(cur->name, name)) {
        pre->next = cur->next;
        /* ksjuse: XXX: this breaks when the cookie to remove comes first */
        /* JDF:    XXX: I believe this will fix the problem:  */
        if(cur == cookie->first){
          /* deleting the first */
          cookie->first = cur->next;
          pre = cookie->first;
        } else {
          /* deleting inner */
          pre->next = cur->next;
        }
        res = TRUE;
        echo ("%s:%d cookie deleted: %ld => %s\n",__FILE__, __LINE__, (long)id,name); 
        break;
      }
    } else {
      continue; 
    }
  }
  return res;
}


/* 
  Delete all cookies associated with the given id.
  return 0 (as delete_cookie does)?
*/
int
delete_all_cookies(pthread_t id)
{
  CNODE  *cur, *pre;
  
  pthread_mutex_lock(&(cookie->mutex));
  for (pre=NULL, cur=cookie->first; cur != NULL; pre=cur, cur=cur->next) {
    if (cur->threadID == id) {
      echo ("%s:%d cookie deleted: %ld => %s\n",__FILE__, __LINE__, (long)id,cur->name); 
      /* delete this cookie */
      if (cur == cookie->first) {
        /* deleting the first */
        cookie->first = cur->next;
        pre = cookie->first;
      } else {
        /* deleting inner */
        pre->next = cur->next;
      }
      xfree(cur->name);
      xfree(cur->value);
      xfree(cur->domain); 
      xfree(cur);
      cur = pre;
      /* cur is NULL now when we deleted the last cookie;
       * cur was cookie->first and first->next was NULL.
       * check that before incremeting (cur=cur->next).
       */
      if (cur == NULL)
      break;
    } 
  }
  pthread_mutex_unlock(&(cookie->mutex));
  return 0;
}

/**
 * get_cookie returns a char * with a compete value of the Cookie: header
 * It does NOT return the actual Cookie struct.      
 */
char *
get_cookie_header(pthread_t id, char *host, char *newton)
{
  int    dlen, hlen; 
  CNODE  *pre, *cur;
  time_t now;
  char   oreo[MAX_COOKIE_SIZE];  

  memset(oreo, '\0', sizeof oreo);
  hlen = strlen(host); 

  pthread_mutex_lock(&(cookie->mutex));
  now = time(NULL);

  for (cur=pre=cookie->first; cur != NULL; pre=cur, cur=cur->next) {
    /**
     * for the purpose of matching, we'll ignore the leading '.'
     */
    const char *domainptr = cur->domain;
    if (*domainptr == '.') ++domainptr;
    dlen = domainptr ? strlen(domainptr) : 0;
    if (cur->threadID == id) {
      if (!strcasecmp(domainptr, host)) {
        if (cur->expires <= now) {
          delete_cookie(cur->threadID, cur->name);
          continue;
        }
        if (strlen(oreo) > 0)
          strncat(oreo, ";",      sizeof(oreo) - 10 - strlen(oreo));
        strncat(oreo, cur->name,  sizeof(oreo) - 10 - strlen(oreo));
        strncat(oreo, "=",        sizeof(oreo) - 10 - strlen(oreo));
        strncat(oreo, cur->value, sizeof(oreo) - 10 - strlen(oreo));
      }
      if ((dlen < hlen) && (!strcasecmp(host + (hlen - dlen), domainptr))) {
        if (cur->expires <= now) {
          delete_cookie(cur->threadID, cur->name);
          continue;
        }
        if (strlen(oreo) > 0)
          strncat(oreo, ";",      sizeof(oreo) - 10 - strlen(oreo));
        strncat(oreo, cur->name,  sizeof(oreo) - 10 - strlen(oreo));
        strncat(oreo, "=",        sizeof(oreo) - 10 - strlen(oreo));
        strncat(oreo, cur->value, sizeof(oreo) - 10 - strlen(oreo));
      }
    }
  }
  if (strlen(oreo) > 0) {
    strncpy(newton, "Cookie: ", 8);
    strncat(newton, oreo,       MAX_COOKIE_SIZE);
    strncat(newton, "\015\012", 2);
  }
  pthread_mutex_unlock(&(cookie->mutex));

  return newton;
}

/*
  Helper util, displays the contents of Cookie
*/
void
display_cookies()
{
  CNODE *cur;
 
  pthread_mutex_lock(&(cookie->mutex));
 
  printf ("Linked list contains:\n");
  for (cur=cookie->first; cur != NULL; cur=cur->next) {
    printf ("Index: %ld\tName: %s Value: %s\n", (long)cur->threadID, cur->name, cur->value);
  }
 
  pthread_mutex_unlock(&(cookie->mutex));
 
  return;
}

