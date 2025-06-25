#ifndef FACTS_H
#define FACTS_H
#include <stdlib.h>
#include <setup.h>

/**
 * FACTS object
 */
struct FACTS_T {
  int       id;
  char      key[1024];
  COOKIES   jar;
};
typedef struct FACTS_T *FACTS;

FACTS   new_facts(int id, char *file);
FACTS   facts_destroy(FACTS this);
BOOLEAN set_cookie(FACTS this, char *line, char *host);
char *  get_cookies(FACTS this);

#endif/*FACTS_H*/
