#ifndef PAGE_H
#define PAGE_H
#include <stdlib.h>

/**
 * PAGE object
 */
typedef struct PAGE_T *PAGE;

PAGE   new_page();
PAGE   page_destroy(PAGE this);
void   page_concat(PAGE this, const char *str, const int len);
void   page_clear(PAGE this);
char * page_value(PAGE this);
size_t page_length(PAGE this);

#endif/*PAGE_H*/

