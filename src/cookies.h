#ifndef __COOKIES_H
#define __COOKIES_H

#include <joedog/defs.h>
#include <joedog/boolean.h>
#include <hash.h>
#include <facts.h>
#include <cookie.h>

#define MAX_COOKIES_SIZE 81920

typedef struct COOKIES_T *COOKIES;

COOKIES new_cookies();
COOKIES cookies_destroy(COOKIES this);
char *  cookies_next(COOKIES this);
void    cookies_reset_iterator(void);
BOOLEAN cookies_add(COOKIES this, COOKIE cookie, size_t owner, char *host);
char *  cookies_file(COOKIES this);
size_t  cookies_length(COOKIES this);
BOOLEAN cookies_delete(COOKIES this, char *str);
BOOLEAN cookies_delete_all(COOKIES this);
void    cookies_list(COOKIES this);
HASH    load_cookies(COOKIES this);
char *  cookies_header(FACTS facts, URL url, char *buf);

#endif/*__COOKIES_H*/


