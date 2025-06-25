#ifndef COOKIE_DEF_H
#define COOKIE_DEF_H

#include <stdint.h>

#define COOKIE_MAGIC 0x434F4F4B

struct COOKIE_T {
  int       id;
  uint32_t  magic;
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
  BOOLEAN   persistent;
  BOOLEAN   httponly;
  BOOLEAN   hostonly;
};

#endif
