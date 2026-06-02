#ifndef __FACTS_H
#define __FACTS_H
#include <stdlib.h>
#include <setup.h>

typedef struct COOKIES_T *COOKIES;

/**
 * FACTS object
 */
struct FACTS_T {
  int       id;
  char      key[1024];
  COOKIES   jar;
};
typedef struct FACTS_T   *FACTS;

FACTS   new_facts(int id, char *file);
FACTS   facts_destroy(FACTS this);
BOOLEAN set_cookie(FACTS this, char *line, char *host);
char *  get_cookies(FACTS this);

#endif/*__FACTS_H*/
