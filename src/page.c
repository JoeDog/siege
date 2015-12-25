#include <page.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct PAGE_T
{
  char * buf;
  size_t len; 
  size_t size;
};

size_t PAGESIZE = sizeof(struct PAGE_T);

void __expand(PAGE this, const int len);

PAGE
new_page(char *str)
{ 
  PAGE this;
  
  this = calloc(1,  PAGESIZE);
  this->len  = strlen(str);
  this->size = this->len + 24576;
  this->buf = calloc(1,   this->size);
  memset(this->buf, '\0', this->size);
  memcpy(this->buf,  str, this->len);
 
  return this;
}

PAGE
page_destroy(PAGE this) 
{
  if (this != NULL) {
    this->len  = 0;
    this->size = 0;
    free(this->buf);
    free(this);
  }
  return this;
}  

size_t
page_length(PAGE this)
{
  return this->len;
}

size_t
page_size(PAGE this)
{
  return this->size;
}

void
page_concat(PAGE this, const char *str, const int len)
{
  if (!this || !str || strlen(str) < 1 || len < 0) 
    return;

  if ((this->len + len) > this->size) {
    __expand(this, len+1);
  }
  memcpy(this->buf+this->len, str, len);
  this->len += len;
  this->buf[this->len+1] = '\0';
  return;
}

void 
page_clear(PAGE this)
{
  if (!this) return;
  this->len = 0;
  memset(this->buf, '\0', this->size);
  return;
}

char *
page_value(PAGE this) 
{
  return this->buf;
}

void
__expand(PAGE this, const int len)
{
  if (!this || len < 0) return;

  this->size += len;
  this->buf   = realloc(this->buf, this->size);
  memset(this->buf + (this->size - len), '\0', len); 
  return;
}

#if 0
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char *argv[])
{
  int     i;
  FILE    *fp;
  char    buf[1024];
  PAGE    page = new_page("");
  
  if (argv[1] == NULL || strlen(argv[1]) < 1) {
    puts("usage: haha <filename>");
    exit(1);
  }
  while (i < 1000) {
    fp = fopen(argv[1], "r");
    if (fp == NULL) exit(EXIT_FAILURE);

    while(fgets(buf, sizeof(buf), fp)){
      page_concat(page,buf,strlen(buf));
    }
    printf("%s", page_value(page));
    page_clear(page);
    fclose(fp);
    i++;
  }
  exit(EXIT_SUCCESS);
}

#endif
