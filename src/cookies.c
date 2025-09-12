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
private BOOLEAN __validity_test(COOKIE oreo, const char *host);
private BOOLEAN __file_exists(char *file);
private BOOLEAN __endswith(const char *str, const char *suffix);

COOKIES
new_cookies() {
  int len;
  COOKIES this;
  char    name[] = "cookies.txt";

  this = calloc(1, sizeof(struct COOKIES_T));
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
cookies_add(COOKIES this, COOKIE cookie, char *host)
{
  size_t  id    = pthread_self(); // XXX: this should go after we cut-over
  NODE    *cur  = NULL; 
  NODE    *pre  = NULL; 
  NODE    *new  = NULL;
  BOOLEAN found = FALSE;
  BOOLEAN valid = FALSE;

  if (!cookie || cookie->magic != COOKIE_MAGIC) {
    fprintf(stderr, "cookies_add: BAD COOKIE passed in! ptr=%p magic=0x%08x (expected 0x%08x)\n",
            cookie,
            cookie ? cookie->magic : 0,
            COOKIE_MAGIC);
    return FALSE;
  }

  if (cookie_get_name(cookie) == NULL || cookie_get_value(cookie) == NULL) return FALSE;
  for (cur = pre = this->head; cur != NULL; pre = cur, cur = cur->next) {
    valid = __validity_test(cookie, host);
    if (valid &&  !strcasecmp(cookie_get_name(cur->cookie), cookie_get_name(cookie))) {
      if (cookie_match(cur->cookie, cookie)) {
        found = TRUE;
        break; 
      }
      cookie_reset_value(cur->cookie, cookie_get_value(cookie));
      cookie  = cookie_destroy(cookie);
      found = TRUE;
      break;
    }
  }
  if (!found) {
    if (my.get && host != NULL) {
      cookie_set_persistent(cookie, TRUE);
    }
    new = (NODE*)malloc(sizeof(NODE));
    new->threadID = (cookie_get_persistent(cookie) == TRUE) ? 999999999999999 : id;
    new->cookie   = cookie;
    new->next     = cur;
    if (cur == this->head) {
      this->head = new;
    } else {
      if (pre != NULL) pre->next  = new;
    }
  }
  return FALSE;
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
cookies_header(COOKIES this, char *host, char *newton)
{
  NODE  *cur;
  char oreo[MAX_COOKIES_SIZE];
  char seen[MAX_COOKIES_SIZE]; // track names we've added
  memset(oreo, 0, sizeof oreo);
  memset(seen, 0, sizeof seen);

  time_t now = time(NULL);

  for (cur = this->head; cur != NULL; cur = cur->next) {
    COOKIE c = cur->cookie;
    const char *domainptr = cookie_get_domain(c);

    if (!cookie_matches_host(c, host)) {
      continue;
    }

    if (__endswith(host, domainptr)) {
      if (cookie_get_expires(c) <= now && !cookie_get_session(c)) {
        cookies_delete(this, cookie_get_name(c));
        continue;
      }

      const char *name = cookie_get_name(c);
      if (strstr(seen, name)) {
        continue;
      }

      xstrncat(seen, name, sizeof(seen) - strlen(seen) - 1);
      xstrncat(seen, ";", sizeof(seen) - strlen(seen) - 1);

      if (strlen(oreo) > 0) xstrncat(oreo, "; ", sizeof(oreo) - strlen(oreo) - 1);
      xstrncat(oreo, name, sizeof(oreo) - strlen(oreo) - 1);
      xstrncat(oreo, "=", sizeof(oreo) - strlen(oreo) - 1);
      xstrncat(oreo, cookie_get_value(c), sizeof(oreo) - strlen(oreo) - 1);
    }
  }

  if (strlen(oreo) > 0) {
    strncpy(newton, "Cookie: ", MAX_COOKIE_SIZE - 1);
    xstrncat(newton, oreo, MAX_COOKIE_SIZE - strlen(newton) - 1);
    xstrncat(newton, "\r\n", MAX_COOKIE_SIZE - strlen(newton) - 1);
  }

  return newton;
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
  printf("__delete_node; cookies_destroy\n");
  node->cookie = cookie_destroy(node->cookie); 
  free(node); 
  node = tmp;
  return node;
}

private void
__strip(char *str)
{
  char *ch;
  ch = (char *)strstr(str, "#");
  if (ch){*ch = '\0';}
  ch = (char *)strstr(str, "\n");
  if (ch){*ch = '\0';}
}

HASH
load_cookies(COOKIES this)
{
  FILE * fp;
  int    n = -1;
  HASH   HOH; 
  HASH   IDX;
  const  size_t len = 4096; // max cookie size
  char   line[len];
  if (! __file_exists(this->file)) {
    return NULL;
  }

  fp = fopen(this->file, "r");
  if (fp == NULL) {
    return NULL;
  }

  HOH = new_hash();
  IDX = new_hash();

  /**
   * We're going to treat this file like it's editable
   * which means it will permit comments and white space 
   * formatting. Siege users are a savvy bunch, they may
   * want to add cookies by hand...
   */
  memset(line, '\0', len);
  while (fgets(line, len, fp) != NULL){
    char *p = strchr(line, '\n');
    if (p) {
      *p = '\0';
    } else {
      int  i;
      if ((i = fgetc(fp)) != EOF) {
        while ((i = fgetc(fp)) != EOF && i != '\n');
        line[0]='\0';
      }
    }
    __strip(line);
    chomp(line);
    if (strlen(line) > 1) {
      int   num = 2;
      char  *key;
      char  *val;
      char  **pair; 
      pair = split('|', line, &num);
      trim(pair[0]);
      trim(pair[1]);
      if (strstr(pair[1], "persistent=true") != NULL) {
        key = xstrdup("999999999999999");
      } else {
        key = xstrdup(pair[0]);
      }
      val = xstrdup(pair[1]);
      if (key != NULL && val != NULL) {
        if (hash_get(IDX, key) == NULL) {
          char tmp[1024];
          n += 1;
          memset(tmp, '\0', 1024);
          snprintf(tmp, 1024, "%d", n);
          hash_add(IDX, key, tmp);
        }
        HASH tmp = (HASH)hash_get(HOH, hash_get(IDX, key));
        if (tmp == NULL) {
          tmp = new_hash();
          hash_add(tmp, val, val);
          hash_nadd(HOH, hash_get(IDX, key), tmp, HASHSIZE);
        } else {
          hash_add(tmp, val, val);
        }
      } 
      split_free(pair, num); 
      xfree(key);
      xfree(val);
    }
    memset(line, '\0', len);
  } 
  fclose(fp);
  hash_destroy(IDX);
  return HOH;
}

/**
 * returns TRUE if the file exists,
 */
private BOOLEAN
__file_exists(char *file)
{
  int  fd;

  if ((fd = open(file, O_RDONLY)) < 0) {
    /**
     * The file does NOT exist so the descriptor is -1
     * No need to close it.
     */
    return FALSE;
  } else {
    /** 
     * Party on Garth... 
     */
    close(fd);
    return TRUE;
  }
  return FALSE;
}

private BOOLEAN
__validity_test(COOKIE oreo, const char *host)
{
  const char *domainptr = NULL;
  if (cookie_get_domain(oreo) != NULL) {
    return TRUE;
  }
  if (host == NULL || *host == '\0') {
    return TRUE;
  }
  domainptr = cookie_get_domain(oreo);
  if (!domainptr || *domainptr == '\0') {
    fprintf(stderr, "Invalid domain pointer: %p\n", (void *)domainptr);
    return FALSE;
  }
  uintptr_t addr = (uintptr_t)domainptr;
  if (addr < 0x1000 || addr % 8 != 0) {
    fprintf(stderr, "domainptr likely bad: %p\n", (void *)domainptr);
    return FALSE;
  }
  if (domainptr == NULL || *domainptr == '\0') {
    fprintf(stderr, "domainptr is null or empty\n");
    return FALSE;
  }
  if (domainptr && strlen(domainptr) < 128) {  // sanity check, optional
    printf("domain: %s\n", domainptr);
  }
  if (*domainptr == '.') ++domainptr;
  if (__endswith(host, domainptr)){
    return TRUE;
  }
  return FALSE;
}

private BOOLEAN
__endswith(const char *str, const char *suffix)
{
  if (!str) {
    fprintf(stderr, "__endswith: str is NULL\n");
    return FALSE;
  }
  if (!suffix) {
    fprintf(stderr, "__endswith: suffix is NULL\n");
    return FALSE;
  }

  size_t lenstr = strlen(str);
  size_t lensuffix = strlen(suffix);

  if (lensuffix > lenstr)
    return FALSE;

  return (strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0);
}


