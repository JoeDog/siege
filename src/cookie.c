#include <cookie.h>
#include <util.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <notify.h>
#include <perl.h>
#include <joedog/defs.h>

struct COOKIE_T {
  char*     name;
  char*     value;
  char*     domain;
  char*     path;
  time_t    expires;
  char *    expstr;
  char *    none;
  char *    string;
  BOOLEAN   session;
  BOOLEAN   secure;
};

size_t COOKIESIZE = sizeof(struct COOKIE_T);

private BOOLEAN __parse_input(COOKIE this, char *str, char *host);
private char *  __parse_pair(char **str);
private int     __parse_time(const char *str);
private int     __mkmonth(char * s, char ** ends);
private char *   months[12] = {
  "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

/**
 * Creates a cookie with the value of the
 * Set-cookie header. 
 * 
 * Set-Cookie: exes=X; expires=Fri, 01-May-2015 12:51:25 GMT
 * ^ strip   ^ ^_ pass all this to the constructor         ^
 */
COOKIE
new_cookie(char *str, char *host)
{
  COOKIE this;

  this = calloc(sizeof(struct COOKIE_T), 1);
  this->expires = 0;
  this->expstr  = NULL;
  this->string  = NULL;
  this->session = TRUE;  
  this->none    = strdup("none");
  if (__parse_input(this, str, host) == FALSE) {
    return cookie_destroy(this);
  }
  return this;
}

/**
 * Destroys a cookie by freeing all allocated memory
 */ 
COOKIE
cookie_destroy(COOKIE this)
{
  if (this != NULL) {
    free(this->name);
    free(this->value);
    free(this->domain);
    free(this->expstr);
    free(this->path);
    free(this->none);
    free(this->string);
    free(this);
  }
  return NULL;
}

void 
cookie_set_name(COOKIE this, char *str)
{
  size_t len = strlen(str)+1;
  this->name = malloc(sizeof this->name * len);
  memset(this->name, '\0', len);
  memcpy(this->name, str, len);
}

void 
cookie_set_value(COOKIE this, char *str)
{
  size_t len  = strlen(str)+1;
  this->value = malloc(sizeof this->value * len);
  memset(this->value, '\0', len);
  memcpy(this->value, str, len);
}

void 
cookie_set_path(COOKIE this, char *str)
{
  size_t len = strlen(str)+1;
  this->path = malloc(sizeof this->path * len);
  memset(this->path, '\0', len);
  memcpy(this->path, str, len);
}

void 
cookie_set_domain(COOKIE this, char *str)
{
  size_t len   = strlen(str)+1;
  this->domain = malloc(sizeof this->domain * len);
  memset(this->domain, '\0', len);
  memcpy(this->domain, str, len);
}

void 
cookie_set_expires(COOKIE this, time_t expires)
{
  this->expires = expires;
}

/**
 * Returns the name of the cookie
 * Example: Set-Cookie: exes=X; expires=Fri, 01-May-2015 12:51:25 GMT
 * Returns: exes
 */
char *
cookie_get_name(COOKIE this) 
{
  if (this->name == NULL) 
    return this->none;
  return this->name;
}

/**
 * Returns the value of the cookie
 * Example: Set-Cookie: exes=X; expires=Fri, 01-May-2015 12:51:25 GMT
 * Returns: X
 */
char * 
cookie_get_value(COOKIE this) 
{
  if (this->value == NULL) 
    return this->none;
  return this->value;
}

/**
 * Returns the value of the domain
 */
char *
cookie_get_domain(COOKIE this)
{
  if (this->domain == NULL) 
    return this->none;
  return this->domain;
}

/**
 * Returns the value of the path
 */
char * 
cookie_get_path(COOKIE this)
{
  if (this->path == NULL)
    return this->none;
  return this->path;
}

time_t 
cookie_get_expires(COOKIE this)
{
  return this->expires;
}

BOOLEAN
cookie_get_session(COOKIE this)
{
  return this->session;
}

/**
 * Returns the string value of the cookie
 * (Mainly a debugging tool; we want cookie_expires for anything useful)
 * Example: Set-Cookie: exes=X; expires=Fri, 01-May-2015 12:51:25 GMT
 * Returns: Fri, 01 May 2015 12:51:25 -0400
 * 
 * @return this->expstr Encapsulated memory free'd in the deconstuctor
 */
char *
cookie_expires_string(COOKIE this)
{
  /*if (this->expstr == NULL) 
    this->expstr = malloc(sizeof (char*) * 128);
  else */
  this->expstr = realloc(this->expstr, sizeof(this->expstr)*128);
  memset(this->expstr, '\0', 128);
  struct tm * timeinfo;
  timeinfo = localtime (&this->expires);
  strftime(this->expstr, 128, "%a, %d %b %Y %H:%M:%S %z", timeinfo);
  return this->expstr;
}

char * 
cookie_to_string(COOKIE this)
{
  int len = 4096;

  if (this->name == NULL || this->value == NULL || this->domain == NULL) return NULL;

  this->string = realloc(this->string, sizeof(this->string)*len);
  memset(this->string, '\0', len);
 
  snprintf(
    this->string, len, "%s=%s; domain=%s; path=%s; expires=%lld",
    this->name, this->value, (this->domain != NULL) ? this->domain : "none", 
    (this->path != NULL) ? this->path : "/", (long long)this->expires
  ); 
  return this->string;
}

/**
 * XXX: should add this to convenience lib
 */
void *
strealloc(char *old, char *str)
{
  size_t num   = strlen(str) + 1;
  char *newptr = realloc(old, sizeof (char*) * num); 
  if (newptr) {
    memset(newptr, '\0', num+1);
    memcpy(newptr, str, num);
  }
  return newptr;
}

void 
cookie_reset_value(COOKIE this, char *value)
{
  this->value   = strealloc(this->value, value); 
}

COOKIE 
cookie_clone(COOKIE this, COOKIE that) 
{
  this->value   = strealloc(this->value,  cookie_get_value(that)); 
  this->domain  = strealloc(this->domain, cookie_get_domain(that));
  this->path    = strealloc(this->path,   cookie_get_path(that));
  //if ((time_t*)cookie_get_expires(that) != 0) {
  if (this->expires > 0) {
    this->expires = time((time_t*)cookie_get_expires(that));
  }
  if (this->session == TRUE) {
    this->session = cookie_get_session(that);
  }
  return this;
}

private BOOLEAN
__parse_input(COOKIE this, char *str, char *host) 
{
  char   *tmp;
  char   *key;
  char   *val;
  char   *pos;
  int    expires = 0;

  if (str == NULL) {
    printf("Coookie: Unable to parse header string");
    return FALSE;
  }
  while (*str && *str == ' ') str++; // assume nothing...

  char *newline = (char*)str;
  while ((tmp = __parse_pair(&newline)) != NULL) {
    key = tmp;
    while (*tmp && !ISSPACE((int)*tmp) && !ISSEPARATOR(*tmp))
      tmp++;
    *tmp++=0;
    while (ISSPACE((int)*tmp) || ISSEPARATOR(*tmp))
      tmp++;
    val  = tmp;
    while (*tmp)
      tmp++;
    if (!strncasecmp(key, "expires", 7)) {
      expires = __parse_time(val);
      if (expires != -1) {
        this->session = FALSE;
        this->expires = expires;
      } // else this->expires was initialized 0 in the constructor
    } else if (!strncasecmp(key, "path", 4))   {
      this->path = strdup(val);
    } else if (!strncasecmp(key, "domain", 6)) {
      cookie_set_domain(this, val);
    } else if (!strncasecmp(key, "secure", 6)) {
      this->secure = TRUE;
    } else {
      this->name  = strdup(key);
      this->value = strdup(val);
    }
  }
  if (this->expires < 1000) {
    this->session = TRUE;
  }
 
  if (this->domain == NULL) {
    pos = strchr (host, '.');
    if (pos == NULL)
      this->domain = xstrdup(".");
    else
      this->domain = xstrdup(pos);
  }
  return TRUE;
}

private char *
__parse_pair(char **str)
{
  int  okay  = 0;
  char *p    = *str;
  char *pair = NULL;

  if (!str || !*str) return NULL;

  pair = p;
  if (p == NULL) return NULL;

  while (*p != '\0' && *p != ';') {
    if (!*p) {
      *str = p;
      return NULL;
    }
    if (*p == '=') okay = 1;
    p++;
  }
  *p++ = '\0';
  *str = p;
  trim(pair);

  if (okay) {
    return pair;
  } else {
    return NULL;
  }
}


/**
 * We'll travel back in time to Jan 2, 1900 to determine
 * what timezone we're in. With that information, we can
 * return an offset in hours. For example, EST returns -5
 */ 
private int 
__utc_offset() 
{
  int         hrs;
  struct tm * ptr;
  time_t      zip = 24*60*60L;

  ptr = localtime(&zip);
  hrs = ptr->tm_hour;

  if (ptr->tm_mday < 2)
    hrs -= 24;

  return hrs;
}


/**
 *  Mostly copied from the MIT reference library HTWWWStr.c
 *  (c) COPYRIGHT MIT 1995.
 *  Please first read the full copyright statement in the file COPYRIGH
 *  With changes by J. Fulmer for the inclusion to siege.
 *
 *  Wkd, 00 Mon 0000 00:00:00 GMT   (RFC1123)
 *  Weekday, 00-Mon-00 00:00:00 GMT (RFC850)
 *  Wkd Mon 00 00:00:00 0000 GMT    (CTIME)
 *  1*DIGIT (delta-seconds)
 */
private int
__parse_time(const char *str) 
{
  char * s;
  struct tm tm;
  time_t rv;
  time_t now;

  if (!str) return 0;

  if ((s = strchr(str, ','))) {	/* Thursday, 10-Jun-93 01:29:59 GMT */
    s++;                        /* or: Thu, 10 Jan 1993 01:29:59 GMT */
    while (*s && *s==' ') s++;
    if (strchr(s,'-')) {	/* First format */
      if ((int)strlen(s) < 18) {
        return 0;
      }
      tm.tm_mday = strtol(s, &s, 10);
      tm.tm_mon  = __mkmonth(++s, &s);
      tm.tm_year = strtol(++s, &s, 10) - 1900;
      tm.tm_hour = strtol(++s, &s, 10);
      tm.tm_min  = strtol(++s, &s, 10);
      tm.tm_sec  = strtol(++s, &s, 10);
    } else {  /* Second format */
      if ((int)strlen(s) < 20) {
        return 0;
      }
      tm.tm_mday = strtol(s, &s, 10);
      tm.tm_mon  = __mkmonth(s, &s);
      tm.tm_year = strtol(s, &s, 10) - 1900;
      tm.tm_hour = strtol(s, &s, 10);
      tm.tm_min  = strtol(++s, &s, 10);
      tm.tm_sec  = strtol(++s, &s, 10);
    }
  } else if (isdigit((int) *str)) {
    if (strchr(str, 'T')) { /* ISO (limited format) date string */
      s = (char *) str;
      while (*s && *s==' ') s++;
      if ((int)strlen(s) < 21) {
        return 0;
      }
      tm.tm_year = strtol(s, &s, 10) - 1900;
      tm.tm_mon  = strtol(++s, &s, 10);
      tm.tm_mday = strtol(++s, &s, 10);
      tm.tm_hour = strtol(++s, &s, 10);
      tm.tm_min  = strtol(++s, &s, 10);
      tm.tm_sec  = strtol(++s, &s, 10);
    } else { /* delta seconds */
      return atol(str);
    }
  } else { /* Try the other format:  Wed Jun  9 01:29:59 1993 GMT */
    s = (char *) str;
    while (*s && *s != ' ') s++; // trim the weekday
    if ((int)strlen(s) < 20) {
      return 0;
    }
    tm.tm_mon  = __mkmonth(s, &s);
    tm.tm_mday = strtol(s, &s, 10);
    tm.tm_hour = strtol(s, &s, 10);
    tm.tm_min  = strtol(++s, &s, 10);
    tm.tm_sec  = strtol(++s, &s, 10);
    tm.tm_year = strtol(s, &s, 10) - 1900;
  }
  if (tm.tm_sec  < 0  ||  tm.tm_sec  > 59  ||
      tm.tm_min  < 0  ||  tm.tm_min  > 59  ||
      tm.tm_hour < 0  ||  tm.tm_hour > 23  ||
      tm.tm_mday < 1  ||  tm.tm_mday > 31  ||
      tm.tm_mon  < 0  ||  tm.tm_mon  > 11) {
    return 0;
  }
  tm.tm_isdst = -1;
  rv = mktime(&tm);
  if (!strstr(str, " GMT") && !strstr(str, " UTC")) {
  	// It's not zulu time, so assume it's in local time
	rv += __utc_offset() * 3600;
  }

  if (rv == -1) {
    return rv;
  }

  now = time (NULL);

  if (rv - now < 0) {
    return 0;
  }
  return rv;
}

private int 
__mkmonth(char * s, char ** ends)
{
  char * ptr = s;
  while (!isalpha((int) *ptr)) ptr++;
  if (*ptr) {
    int i;
    *ends = ptr+3;		
    for (i=0; i<12; i++)
      if (!strncasecmp(months[i], ptr, 3)) return i;
  }
  return 0;
}

