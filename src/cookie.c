#include <stdint.h>
#include <cookie.h>
#include <cookie_def.h>
#include <util.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <notify.h>
#include <perl.h>
#include <joedog/defs.h>

size_t  COOKIESIZE = sizeof(struct COOKIE_T);

private BOOLEAN __parse_input(COOKIE this, char *str, char *host);
private int     __parse_time(const char *str);
private time_t  __timegm(struct tm *tm);
private int     __mkmonth(char * s, char ** ends);
private char *   months[12] = {
  "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

char *__cookie_none = "none";

/**
 * Creates a cookie with the value of the
 * Set-cookie header. 
 * 
 * Set-Cookie: exes=X; expires=Fri, 01-May-2015 12:51:25 GMT
 * ^ strip   ^ ^_ pass all this to the constructor         ^
 */
static int cookie_id_counter = 0;

COOKIE
new_cookie(char *str, char *host)
{
  COOKIE this = calloc(1, sizeof(struct COOKIE_T));
  if (!this) return NULL;

  this->magic      = COOKIE_MAGIC;
  this->id         = ++cookie_id_counter;

  this->name       = NULL;
  this->value      = NULL;
  this->domain     = NULL;
  this->expires    = 0;
  this->expstr     = NULL;
  this->string     = NULL;
  this->path       = NULL;
  this->session    = TRUE;
  this->persistent = FALSE;
  this->secure     = FALSE;
  this->httponly   = FALSE;
  this->none       = strdup(__cookie_none);

  if (str != NULL) {
    if (__parse_input(this, str, host) == FALSE) {
      return cookie_destroy(this);
    }
  }
  return this;
}

/**
 * Destroys a cookie by freeing all allocated memory
 */ 
COOKIE 
cookie_destroy(COOKIE this) {
  if (!cookie_is_valid(this)) {
    return NULL;
  }
  xfree(this->name);    this->name = NULL;
  xfree(this->value);   this->value = NULL;
  xfree(this->domain);  this->domain = NULL;
  xfree(this->path);    this->path = NULL;
  xfree(this->string);  this->string = NULL;
  xfree(this->expstr);  this->expstr = NULL;
  xfree(this->none);    this->none = NULL;
  this->magic = 0xDEADDEAD;
  xfree(this);

  return NULL;
}


BOOLEAN
cookie_is_valid(COOKIE this) 
{
  return this && this->magic == COOKIE_MAGIC;
}

BOOLEAN
cookie_match(COOKIE this, COOKIE that)
{
  if (! strmatch(this->name, that->name)) {
    return FALSE;
  }  
  if (! strmatch(this->value, that->value)) {
    return FALSE;
  }  
  if (! strmatch(this->path, that->path)) {
    return FALSE;
  }  
  if (! strmatch(this->domain, that->domain)) {
    return FALSE;
  }  
  return TRUE;  
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
cookie_set_domain(COOKIE this, const char *domain) {
  if (!this || !domain) return;
  if (this->domain) free(this->domain);
  this->domain = strdup(domain);
}

void
cookie_set_hostonly(COOKIE this, BOOLEAN hostonly)
{
  this->hostonly = hostonly;
}

void 
cookie_set_expires(COOKIE this, time_t expires)
{
  this->expires = expires;
}

void 
cookie_set_persistent(COOKIE this, BOOLEAN persistent)
{
  this->persistent = persistent;
}

/**
 * Returns the name of the cookie
 * Example: Set-Cookie: exes=X; expires=Fri, 01-May-2015 12:51:25 GMT
 * Returns: exes
 */
char *
cookie_get_name(COOKIE this) 
{
  if (this == NULL || this->name == NULL) 
    return __cookie_none;
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
  if (this == NULL || this->value == NULL) 
    return __cookie_none;
  return this->value;
}

/**
 * Returns the value of the domain
 */
char *
cookie_get_domain(COOKIE this)
{
  if (!this || this->magic != COOKIE_MAGIC) {
    return NULL;
  }

  if (this == NULL || this->domain == NULL)
    return __cookie_none;

  uintptr_t addr = (uintptr_t)this->domain;
  if (addr < 0x1000 || addr > 0x00007FFFFFFFFFFF) {  // quick sanity
    fprintf(stderr, "Invalid domain pointer: %p\n", (void *)this->domain);
    return __cookie_none;
  }

  // Optional: validate string
  size_t maxlen = 256;
  for (size_t i = 0; i < maxlen; ++i) {
    char c = this->domain[i];
    if (c == '\0') break;
    if ((unsigned char)c < 32 || (unsigned char)c > 126) {
      fprintf(stderr, "Domain string looks corrupt\n");
      return __cookie_none;
    }
  }

  return this->domain;
}

BOOLEAN
cookie_get_hostonly(COOKIE this)
{
  return this->hostonly;
}


/**
 * Returns the value of the path
 */
char * 
cookie_get_path(COOKIE this)
{
  if (this == NULL || this->path == NULL)
    return __cookie_none;
  return this->path;
}

time_t 
cookie_get_expires(COOKIE this)
{
  if (this == NULL) 
    return -1;
  return this->expires;
}

BOOLEAN
cookie_get_session(COOKIE this)
{
  if (this == NULL) 
    return TRUE;
  return this->session;
}

BOOLEAN
cookie_get_persistent(COOKIE this)
{
  if (this == NULL) 
    return TRUE;
  if (this->expires > 0) {
    return TRUE;
  }
  return this->persistent;
}

BOOLEAN
cookie_matches_host(COOKIE this, const char *host)
{
  const char *domain = cookie_get_domain(this);
  if (!domain || !host) return FALSE;

  size_t host_len   = strlen(host);
  size_t domain_len = strlen(domain);

  if (domain[0] == '.') {
    domain++;
    domain_len--;
  }

  if (this->hostonly) {
    return strcasecmp(host, domain) == 0;
  }

  if (host_len < domain_len) return FALSE;

  const char *host_suffix = host + (host_len - domain_len);
  if (strcasecmp(host_suffix, domain) != 0)
    return FALSE;

  if (host_len == domain_len) return TRUE;

  if (host[host_len - domain_len - 1] != '.') return FALSE;

  return TRUE;
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
  if (!this || this->magic != COOKIE_MAGIC) {
    fprintf(stderr,
        "cookie_to_string: INVALID COOKIE POINTER! this=%p magic=%08x (expected %08x)\n",
        (void*)this,
        this ? this->magic : 0,
        COOKIE_MAGIC
    );
    abort();  // Trap it right here
  }

  if (!this || !this->name || !this->value || !this->domain)
    return NULL;

  char *s = this->string;

  // memory poison check; platform-specific but should catch free'd memory
  if (s && ((uintptr_t)s & 0xff00000000000000) == 0x3300000000000000) {
    fprintf(stderr, "Warning: suspicious string pointer: %p\n", (void *)s);
  }

  char *new_buf = realloc(this->string, MAX_COOKIE_SIZE);
  if (!new_buf)
    return NULL;

  this->string = new_buf;
  memset(this->string, 0, MAX_COOKIE_SIZE);

  snprintf(
    this->string, MAX_COOKIE_SIZE,
    "%s=%s; domain=%s; path=%s; expires=%lld%s",
    this->name, this->value,
    this->domain ? this->domain : "none",
    this->path ? this->path : "/",
    (long long)this->expires,
    this->persistent ? "; persistent=true" : ""
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
cookie_clone(COOKIE original) {
  if (!cookie_is_valid(original)) {
    fprintf(stderr, "cookie_clone: invalid COOKIE passed in: %p\n", (void*)original);
    return NULL;
  }

  COOKIE clone = new_cookie(NULL, NULL);
  if (!clone) return NULL;

  if (original->name)    clone->name    = strdup(original->name);
  if (original->value)   clone->value   = strdup(original->value);
  if (original->domain)  clone->domain  = strdup(original->domain);
  if (original->path)    clone->path    = strdup(original->path);
  if (original->string)  clone->string  = strdup(original->string);

  clone->expires    = original->expires;
  clone->secure     = original->secure;
  clone->persistent = original->persistent;
  clone->session    = original->session;
  clone->httponly   = original->httponly;

  return clone;
}

private BOOLEAN
__parse_input(COOKIE this, char *hdr, char *host)
{
  if (!hdr || !this) return FALSE;

  BOOLEAN seen_max_age = FALSE;
  time_t now = time(NULL);
  time_t expires_candidate = 0;
  time_t max_age_candidate = 0;

  char *copy = strdup(hdr);
  if (!copy) return FALSE;

  char *token = strtok(copy, ";");

  while (token) {
    token = trim(token);
    if (*token == '\0') {
      token = strtok(NULL, ";");
      continue;
    }

    char *sep = strchr(token, '=');

    if (sep) {
      *sep = '\0';
      char *key_raw = token;
      char *val_raw = sep + 1;

      char *key_trimmed = trim(key_raw);
      char *val_trimmed = trim(val_raw);

      char *key = strdup(key_trimmed);
      char *val = strdup(val_trimmed);

      if (!key || !val) {
        free(key);
        free(val);
        goto error;
      }

      if (!strncasecmp(key, "max-age", 7)) {
        long max = atol(val);
        if (max > 0) {
          seen_max_age = TRUE;
          max_age_candidate = now + max;
        }
      } else if (!strncasecmp(key, "expires", 7)) {
        time_t parsed = __parse_time(val);
        if (parsed > 0) {
          expires_candidate = parsed;
        }
      } else if (!strncasecmp(key, "path", 4)) {
        this->path = strdup(val);
        if (!this->path) goto error;

      } else if (!strncasecmp(key, "domain", 6)) {
        cookie_set_domain(this, val); 
        cookie_set_hostonly(this, FALSE);
      } else if (!strncasecmp(key, "secure", 6)) {
        this->secure = TRUE;

      } else if (!strncasecmp(key, "persistent", 10)) {
        if (!strncasecmp(val, "true", 4)) {
          this->persistent = TRUE;
        }
      } else if (!this->name) {
        this->name = key;
        this->value = val;
        key = NULL;
        val = NULL;
      }

      free(key);
      free(val);
    } else {
      token = trim(token);
      if (!strncasecmp(token, "secure", 6)) {
        this->secure = TRUE;
      }
    }

    token = strtok(NULL, ";");
  }

  // Handle expiration assignment (RFC 6265: Max-Age takes precedence)
  if (seen_max_age && max_age_candidate > 0) {
    this->session = FALSE;
    this->expires = max_age_candidate;
  } else if (expires_candidate > 0) {
    this->session = FALSE;
    this->expires = expires_candidate;
  }

  const char *domain = cookie_get_domain(this);
  if (domain == NULL || domain == __cookie_none) {
    cookie_set_domain(this, host);
    cookie_set_hostonly(this, TRUE);
  }

  free(copy);
  return TRUE;

error:
  free(copy);
  return FALSE;
}

private time_t
__timegm(struct tm *tm) {
  char *old_tz = getenv("TZ");
  char *tz_backup = old_tz ? strdup(old_tz) : NULL;

  setenv("TZ", "UTC", 1);  // explicitly UTC, not ""
  tzset();

  time_t result = mktime(tm);

  // restore previous TZ
  if (tz_backup) {
    setenv("TZ", tz_backup, 1);
    free(tz_backup);
  } else {
    unsetenv("TZ");
  }
  tzset();

  return result;
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
  char *s;
  struct tm tm;
  time_t rv;
  time_t now;

  if (!str) return 0;

  memset(&tm, 0, sizeof(tm));  // Initialize struct tm to zero

  if ((s = strchr(str, ','))) {	/* e.g. "Thu, 10 Jan 1993 01:29:59 GMT" or "Thursday, 10-Jun-93 01:29:59 GMT" */
    s++;                        
    while (*s && *s == ' ') s++;
    if (strchr(s, '-')) {  /* RFC 850 format */
      if ((int)strlen(s) < 18) return 0;
      tm.tm_mday = strtol(s, &s, 10);
      tm.tm_mon  = __mkmonth(++s, &s);
      tm.tm_year = strtol(++s, &s, 10);
      tm.tm_year += (tm.tm_year < 70) ? 100 : 0;  // 2-digit year correction
      tm.tm_hour = strtol(++s, &s, 10);
      tm.tm_min  = strtol(++s, &s, 10);
      tm.tm_sec  = strtol(++s, &s, 10);
    } else {  /* RFC 1123 format */
      if ((int)strlen(s) < 20) return 0;
      tm.tm_mday = strtol(s, &s, 10);
      tm.tm_mon  = __mkmonth(s, &s);
      tm.tm_year = strtol(s, &s, 10) - 1900;
      tm.tm_hour = strtol(s, &s, 10);
      tm.tm_min  = strtol(++s, &s, 10);
      tm.tm_sec  = strtol(++s, &s, 10);
    }
  } else if (isdigit((int) *str)) {
    if (strchr(str, 'T')) { /* ISO format: YYYY-MM-DDTHH:MM:SS */
      s = (char *) str;
      if ((int)strlen(s) < 21) return 0;
      tm.tm_year = strtol(s, &s, 10) - 1900;
      tm.tm_mon  = strtol(++s, &s, 10) - 1;
      tm.tm_mday = strtol(++s, &s, 10);
      tm.tm_hour = strtol(++s, &s, 10);
      tm.tm_min  = strtol(++s, &s, 10);
      tm.tm_sec  = strtol(++s, &s, 10);
    } else {
      return atol(str);  // delta seconds format
    }
  } else {  /* ANSI C asctime() format: "Wkd Mon DD HH:MM:SS YYYY" */
    s = (char *) str;
    while (*s && *s != ' ') s++; // skip weekday
    if ((int)strlen(s) < 20) return 0;
    tm.tm_mon  = __mkmonth(s, &s);
    tm.tm_mday = strtol(s, &s, 10);
    tm.tm_hour = strtol(s, &s, 10);
    tm.tm_min  = strtol(++s, &s, 10);
    tm.tm_sec  = strtol(++s, &s, 10);
    tm.tm_year = strtol(s, &s, 10) - 1900;
  }

  if (tm.tm_sec  < 0 || tm.tm_sec  > 59 ||
      tm.tm_min  < 0 || tm.tm_min  > 59 ||
      tm.tm_hour < 0 || tm.tm_hour > 23 ||
      tm.tm_mday < 1 || tm.tm_mday > 31 ||
      tm.tm_mon  < 0 || tm.tm_mon  > 11) {
    return 0;
  }

  tm.tm_isdst = 0;
  rv = __timegm(&tm);  // Use UTC time conversion
  if (rv == -1) return 0;

  now = time(NULL);
  if (rv - now < 0) return 0;

  return rv;
}

private int 
__mkmonth(char * s, char ** ends)
{
  char * ptr = s;
  while (!isalpha((int) *ptr)) ptr++;

  if (*ptr) {
    int i;
    for (i = 0; i < 12; i++) {
      if (!strncasecmp(months[i], ptr, 3)) {
        ptr += 3;
        while (*ptr && isspace((unsigned char)*ptr)) ptr++;
        *ends = ptr;
        return i;
      }
    }
  }
  return 0;
}


#if 0
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
#endif

