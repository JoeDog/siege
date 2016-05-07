#ifndef __COOKIES_H
#define __COOKIES_H

#include <joedog/defs.h>
#include <joedog/boolean.h>
#include <hash.h>
#include <cookie.h>

#define MAX_COOKIES_SIZE 81920

typedef struct COOKIES_T *COOKIES;

COOKIES new_cookies();
COOKIES cookies_destroy(COOKIES this);
BOOLEAN cookies_add(COOKIES this, char *str, char *host);
BOOLEAN cookies_add_id(COOKIES this, char *str, char *host, size_t id);
char *  cookies_header(COOKIES this, char *host, char *newton);
size_t  cookies_length(COOKIES this);
BOOLEAN cookies_delete(COOKIES this, char *str);
BOOLEAN cookies_delete_all(COOKIES this);
void    cookies_list(COOKIES this);
HASH    load_cookies(COOKIES this);

#endif/*__COOKIES_H*/


