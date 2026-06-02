#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <array.h>
#include <hash.h>
#include <perl.h>
#include <setup.h>
#include <memory.h>
#include <notify.h>
#include <cookies.h>
#include <cookie_def.h>

typedef struct NODE {
  size_t threadID;
  COOKIE cookie;
  struct NODE *next;
} NODE;

struct COOKIES_T {
  NODE *          head;
  size_t          size;
  char *          file;
};

private NODE *iter = NULL;

private NODE *  __delete_node(NODE *node);
private BOOLEAN __same_cookie_identity(COOKIE a, COOKIE b);

COOKIES
new_cookies() {
  int len;
  COOKIES this;
  char    name[] = "cookies.txt";

  this = calloc(sizeof(struct COOKIES_T), 1);
  this->size = 0;
  char *p = getenv("HOME");
  len = p ? strlen(p) : 60;
  len += strlen(name)+1;
  this->file = xmalloc(sizeof (char*) * len);
  memset(this->file, '\0', len);
  snprintf(this->file, len, "%s%s", getenv("HOME"), name);
  return this;
}

COOKIES
cookies_destroy(COOKIES this) 
{
  NODE *cur     = NULL; 
  cur = this->head;
  while (cur) {
    cur = __delete_node(cur);
  }
  xfree(this->file);
  free(this);
  return NULL;
}

BOOLEAN
cookies_add(COOKIES this, COOKIE cookie, size_t owner, char *host)
{
  size_t id = owner;
  NODE *cur = NULL;
  NODE *pre = NULL;
  NODE *new = NULL;

  if (!this || !cookie || cookie->magic != COOKIE_MAGIC) {
    fprintf(stderr,
      "cookies_add: BAD COOKIE passed in! ptr=%p magic=0x%08x expected=0x%08x\n",
      (void *)cookie,
      cookie ? cookie->magic : 0,
      COOKIE_MAGIC
    );
    return FALSE;
  }

  if (!cookie_get_name(cookie) || !cookie_get_value(cookie)) {
    cookie_destroy(cookie);
    return FALSE;
  }

  /*
   * Reject cookies that do not apply to this host.
   * cookie_matches_host() is the authoritative domain/host test.
   */
  if (host && !cookie_matches_host(cookie, host)) {
    cookie_destroy(cookie);
    return FALSE;
  }

  /*
   * Cookie identity is owner + name + domain + path.
   * Value is NOT part of identity; a new value replaces the old value.
   */
  for (cur = this->head; cur != NULL; cur = cur->next) {
    if (cur->threadID == id && __same_cookie_identity(cur->cookie, cookie)) {
      cookie_reset_value(cur->cookie, cookie_get_value(cookie));
      cookie_set_expires(cur->cookie, cookie_get_expires(cookie));
      cookie_set_persistent(cur->cookie, cookie_get_persistent(cookie));

      cur->cookie->session  = cookie->session;
      cur->cookie->secure   = cookie->secure;
      cur->cookie->httponly = cookie->httponly;
      cur->cookie->hostonly = cookie->hostonly;

      cookie_destroy(cookie);
      return TRUE;
    }
  }

  new = malloc(sizeof(NODE));
  if (!new) {
    cookie_destroy(cookie);
    return FALSE;
  }
  new->threadID = id;
  new->cookie   = cookie;
  new->next     = NULL;

  if (this->head == NULL) {
    this->head = new;
  } else {
    for (pre = this->head; pre->next != NULL; pre = pre->next)
      ;
    pre->next = new;
  }

  this->size++;
  return TRUE;
}

BOOLEAN
cookies_delete(COOKIES this, char *str)
{
  NODE     *cur;
  NODE     *pre;
  BOOLEAN   ret = FALSE;
  pthread_t id  = pthread_self();

  for (cur = pre = this->head; cur != NULL; pre = cur, cur = cur->next) {
    if (cur->threadID == id) {
      char *name    = cookie_get_name(cur->cookie);
      if (!strcasecmp(name, str)) {
        cur->cookie = cookie_destroy(cur->cookie);
        pre->next   = cur->next;
        if (cur == this->head) {
          this->head = cur->next;
          pre = this->head;
        } else {
          pre->next = cur->next;
        }
        ret = TRUE;
        break;
      } 
    }
  }
  return ret;
}

BOOLEAN
cookies_delete_all(COOKIES this) 
{
  NODE     *cur;
  NODE     *pre;
  pthread_t id  = pthread_self();
  // XXX: delete cookies by thread; not every cookie in the list
  for (cur = pre = this->head; cur != NULL; pre = cur, cur = cur->next) {
    if (cur->threadID == id) { /* XXX: Need to rework this for siege-# */
      cur->cookie = cookie_destroy(cur->cookie);
      pre->next   = cur->next;
      if (cur == this->head) {
        this->head = cur->next;
        pre = this->head;
      } else {
        pre->next = cur->next;
      }
    }
  }
  return TRUE;
}

char *
cookies_header(FACTS facts, URL url, char *buf)
{
  NODE    *cur; 
  NODE    *next;
  char    *host = url_get_hostname(url);
  COOKIES this  = facts->jar;
  char    oreo[MAX_COOKIES_SIZE];
  char    seen[MAX_COOKIES_SIZE]; // store as ;name;name; format

  memset(oreo, 0, sizeof oreo);
  memset(seen, 0, sizeof seen);

  time_t now = time(NULL);

  for (cur = this->head; cur != NULL; cur = next) {
    next = cur->next;  // safe iteration even if we delete

    COOKIE c = cur->cookie;

    /* Host match (authoritative) */
    if (!cookie_matches_host(c, host)) {
      continue;
    }

    /* Expiration */
    if (cookie_get_expires(c) <= now && !cookie_get_session(c)) {
      cookies_delete(this, cookie_get_name(c));
      continue;
    }

    const char *name  = cookie_get_name(c);
    const char *value = cookie_get_value(c);

    /* Seen check with delimiter safety: ;name; */
    char needle[256];
    snprintf(needle, sizeof(needle), ";%s;", name);

    if (strstr(seen, needle)) {
      continue;
    }

    /* Mark as seen */
    xstrncat(seen, needle, sizeof(seen) - strlen(seen) - 1);

    /* Append to Cookie header string */
    if (strlen(oreo) > 0) {
      xstrncat(oreo, "; ", sizeof(oreo) - strlen(oreo) - 1);
    }

    xstrncat(oreo, name,  sizeof(oreo) - strlen(oreo) - 1);
    xstrncat(oreo, "=",   sizeof(oreo) - strlen(oreo) - 1);
    xstrncat(oreo, value, sizeof(oreo) - strlen(oreo) - 1);
  }

  if (strlen(oreo) > 0) {
    strncpy(buf, "Cookie: ", MAX_COOKIE_SIZE - 1);
    buf[MAX_COOKIE_SIZE - 1] = '\0';

    xstrncat(buf, oreo, MAX_COOKIE_SIZE - strlen(buf) - 1);
    xstrncat(buf, "\r\n", MAX_COOKIE_SIZE - strlen(buf) - 1);
  }
  
  return buf;
}

char *
cookies_file(COOKIES this)
{
  return this->file;
}

void
cookies_list(COOKIES this) 
{
  NODE *cur     = NULL; 
  NODE *pre     = NULL; 
  
  for (cur = pre = this->head; cur != NULL; pre = cur, cur = cur->next) {
    COOKIE tmp = cur->cookie;
    if (tmp == NULL) 
      ; 
    else printf(
      "%lld: NAME: %s\n   VALUE: %s\n   Expires: %s  Persistent: %s\nDomain: %s\n",
      (long long)cur->threadID, cookie_get_name(tmp), cookie_get_value(tmp), cookie_expires_string(tmp),
      (cookie_get_persistent(tmp)==TRUE) ? "true" : "false", cookie_get_domain(tmp)
    );
  }
}


char *
cookies_next(COOKIES this) 
{
  BOOLEAN okay = FALSE;

  if (this == NULL) return NULL;

  do {
    if (iter == NULL)
      iter = this->head;
    else
      iter = iter->next;

    if (iter == NULL)
      return NULL;

    if (cookie_get_persistent(iter->cookie)) {
      return cookie_to_string(iter->cookie);
    }
  } while (! okay);
  return NULL;
}

void cookies_reset_iterator(void) {
  iter = NULL;
}

private NODE *
__delete_node(NODE *node) 
{
  
  if (node == NULL) return NULL;
  NODE *tmp = node->next;
  node->cookie = cookie_destroy(node->cookie); 
  free(node); 
  node = tmp;
  return node;
}

private BOOLEAN
__same_cookie_identity(COOKIE a, COOKIE b)
{
  const char *an = cookie_get_name(a);
  const char *bn = cookie_get_name(b);
  const char *ad = cookie_get_domain(a);
  const char *bd = cookie_get_domain(b);
  const char *ap = cookie_get_path(a);
  const char *bp = cookie_get_path(b);

  if (!an || !bn || !ad || !bd || !ap || !bp) return FALSE;

  return !strcasecmp(an, bn) &&
         !strcasecmp(ad, bd) &&
         !strcasecmp(ap, bp);
}

