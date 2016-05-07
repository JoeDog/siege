#include <stdlib.h>
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
#include <memory.h>
#include <notify.h>
#include <cookies.h>

typedef struct NODE {
  size_t threadID;
  COOKIE cookie;
  struct NODE *next;
} NODE;

struct COOKIES_T {
  NODE *          head;
  size_t          size;
  char *          file;
  pthread_mutex_t mutex;
};

private NODE *  __delete_node(NODE *node);
private BOOLEAN __exists(char *file);
private BOOLEAN __save_cookies(COOKIES this);

COOKIES
new_cookies() {
  COOKIES this;

  this = calloc(sizeof(struct COOKIES_T), 1);
  this->size = 0;
  if (pthread_mutex_init( &(this->mutex), NULL) != 0) {
    NOTIFY(FATAL, "cookies: pthread_mutex_init");
  } 
  int len    = strlen(getenv("HOME")) + 20;
  this->file = malloc(sizeof (char*) * len);
  memset(this->file, '\0', len);
  snprintf(this->file, len, "%s/.siege/cookies.txt", getenv("HOME"));
  return this;
}

COOKIES
cookies_destroy(COOKIES this) 
{
  NODE *cur     = NULL; 
  pthread_mutex_lock(&(this->mutex));
  __save_cookies(this);
  cur = this->head;
  while (cur) {
    cur = __delete_node(cur);
  }
  xfree(this->file);
  pthread_mutex_unlock(&(this->mutex));
  pthread_mutex_destroy(&(this->mutex));
  free(this);
  return NULL;
}

BOOLEAN
cookies_add(COOKIES this, char *str, char *host)
{
  size_t  id    = pthread_self();
  int     hlen  = 0;
  int     dlen  = 0;
  NODE   *cur   = NULL; 
  NODE   *pre   = NULL; 
  NODE   *new   = NULL;
  BOOLEAN found = FALSE;
  BOOLEAN valid = FALSE;
  COOKIE  oreo  = new_cookie(str, host);

  if (oreo == NULL) return FALSE;
  if (cookie_get_name(oreo) == NULL || cookie_get_value(oreo) == NULL) return FALSE;

  pthread_mutex_lock(&(this->mutex));
  for (cur = pre = this->head; cur != NULL; pre = cur, cur = cur->next) {
    const char *domainptr = cookie_get_domain(cur->cookie);
    if (*domainptr == '.') ++domainptr;
    hlen = host      ? strlen(host)      : 0;
    dlen = domainptr ? strlen(domainptr) : 0;
    if (! strcasecmp(host, domainptr)) {
      valid = TRUE; // host level cookie found
    }
    if (! valid && (dlen < hlen) && (! strcasecmp(host + (hlen - dlen), domainptr))) {
      valid = TRUE; // domain level cookie found
    }
    if (valid && cur->threadID == id && !strcasecmp(cookie_get_name(cur->cookie), cookie_get_name(oreo))) {
      cookie_reset_value(cur->cookie, cookie_get_value(oreo));
      oreo  = cookie_destroy(oreo);
      found = TRUE;
      break;
    }
  }   

  if (!found) {
    new = (NODE*)malloc(sizeof(NODE));
    new->threadID = id;
    new->cookie   = oreo;
    new->next     = cur;
    if (cur == this->head)
      this->head = new;
    else
      pre->next  = new;
  }
  pthread_mutex_unlock(&(this->mutex));

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
        //NOTIFY(DEBUG, "%s:%d cookie deleted: %ld => %s\n",__FILE__, __LINE__, (long)id, str);
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
    if (cur->threadID == id) {
      //char *name    = cookie_get_name(cur->cookie);
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
  int   dlen; 
  int   hlen;
  NODE  *pre;
  NODE  *cur;
  time_t tmp;
  time_t now;
  struct tm tm;
  char   oreo[MAX_COOKIES_SIZE];
  size_t id = pthread_self();

  memset(oreo, '\0', sizeof oreo);
  hlen = strlen(host);

  pthread_mutex_lock(&(this->mutex));
  tmp = time(NULL);
  gmtime_r(&tmp, &tm);
  tm.tm_isdst = -1; // force mktime to figure it out!
  now = mktime(&tm);

  for (cur=pre=this->head; cur != NULL; pre=cur, cur=cur->next) {
    /**
     * for the purpose of matching, we'll ignore the leading '.'
     */
    const char *domainptr = cookie_get_domain(cur->cookie);
    if (*domainptr == '.') ++domainptr;
    dlen = domainptr ? strlen(domainptr) : 0;
    if (cur->threadID == id) {
      if (!strcasecmp(domainptr, host)) {
        if (cookie_get_expires(cur->cookie) <= now && cookie_get_session(cur->cookie) != TRUE) {
          cookies_delete(this, cookie_get_name(cur->cookie));
          continue;
        }
        if (strlen(oreo) > 0)
          strncat(oreo, ";",      sizeof(oreo) - 10 - strlen(oreo));
        strncat(oreo, cookie_get_name(cur->cookie),  sizeof(oreo) - 10 - strlen(oreo));
        strncat(oreo, "=",        sizeof(oreo) - 10 - strlen(oreo));
        strncat(oreo, cookie_get_value(cur->cookie), sizeof(oreo) - 10 - strlen(oreo));
      }
      if ((dlen < hlen) && (!strcasecmp(host + (hlen - dlen), domainptr))) {
        if (cookie_get_expires(cur->cookie) <= now && cookie_get_session(cur->cookie) != TRUE) {
          cookies_delete(this, cookie_get_name(cur->cookie));
          continue;
        }
        if (strlen(oreo) > 0)
          strncat(oreo, ";",      sizeof(oreo) - 10 - strlen(oreo));
        strncat(oreo, cookie_get_name(cur->cookie),  sizeof(oreo) - 10 - strlen(oreo));
        strncat(oreo, "=",        sizeof(oreo) - 10 - strlen(oreo));
        strncat(oreo, cookie_get_value(cur->cookie), sizeof(oreo) - 10 - strlen(oreo));
      }
    }
  }
  if (strlen(oreo) > 0) {
    strncpy(newton, "Cookie: ", 8);
    strncat(newton, oreo,       MAX_COOKIE_SIZE);
    strncat(newton, "\015\012", 2);
  }
  pthread_mutex_unlock(&(this->mutex));

  return newton;
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
      "%lld: NAME: %s\n   VALUE: %s\n   Expires: %s\n",
      (long long)cur->threadID, cookie_get_name(tmp), cookie_get_value(tmp), cookie_expires_string(tmp)
    );
  }
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

  if (! __exists(this->file)) {
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
      char  **pair; 
      pair = split('|', line, &num);
      trim(pair[0]);
      trim(pair[1]);
      if (pair[0] != NULL && pair[1] != NULL) {
        if (hash_get(IDX, pair[0]) == NULL) {
          char tmp[1024];
          n += 1;
          memset(tmp, '\0', 1024);
          snprintf(tmp, 1024, "%d", n);
          hash_add(IDX, pair[0], tmp);
        }
        HASH tmp = (HASH)hash_get(HOH, hash_get(IDX, pair[0]));
        if (tmp == NULL) {
          tmp = new_hash();
          hash_add(tmp, pair[1], pair[1]);
          hash_nadd(HOH, hash_get(IDX, pair[0]), tmp, HASHSIZE);
        } else {
          hash_add(tmp, pair[1], pair[1]);
        }
      } 
      split_free(pair, num); 
    }
    memset(line, '\0', len);
  } 
  fclose(fp);
  hash_destroy(IDX);
  return HOH;
}

private BOOLEAN
__save_cookies(COOKIES this) 
{
  FILE * fp;
  char * line;
  size_t len  = 4096+24; // max cookie size plus ID
  time_t now;

  NODE * cur  = NULL;
  now = time(NULL);
  fp  = fopen(this->file, "w");
  if (fp == NULL) {
    return FALSE;
  }
  fputs("#\n", fp);
  fputs("# Siege cookies file. You may edit this file to add cookies\n",fp);
  fputs("# manually but comments and formatting will be removed.    \n",fp);
  fputs("# All cookies that expire in the future will be preserved. \n",fp);
  fputs("# ---------------------------------------------------------\n",fp);
  line = malloc(sizeof(char *) * len);

  for (cur = this->head; cur != NULL; cur = cur->next) {
    COOKIE tmp = cur->cookie; 
    /**
     * Criteria for saving cookies:
     * 1.) It's not null (obvs)
     * 2.) It's not a session cookie
     * 3.) It's not expired. 
     * All cookies which meet the requirement are stored
     * whether they were used during this session or not.
     */
    if (tmp != NULL && cookie_get_session(tmp) != TRUE && cookie_get_expires(cur->cookie) >= now) {
      memset(line, '\0', len);
      if (cookie_to_string(tmp) != NULL) {
        snprintf(line, len, "%ld | %s\n", cur->threadID, cookie_to_string(tmp));
      }
      fputs(line, fp); 
    }
  }   
  free(line);
  fclose(fp);
  return TRUE;
}

/**
 * returns TRUE if the file exists,
 */
private BOOLEAN
__exists(char *file)
{
  int  fd;

  /* open the file read only  */
  if ((fd = open(file, O_RDONLY)) < 0) {
  /* the file does NOT exist  */
    close(fd);
    return FALSE;
  } else {
  /* party on Garth... */
    close(fd);
    return TRUE;
  }

  return FALSE;
}
