#include <facts.h>
#include <cookie.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <perl.h>
#include <setup.h>
#include <memory.h>
#include <notify.h>
#include <util.h>
#include <cookies.h>

size_t FACTSSIZE = sizeof(struct FACTS_T);

private BOOLEAN __exists(char *file);
private BOOLEAN __load_cookies(FACTS this, char *file);
private BOOLEAN __contains(const char *filename, const char *target);

FACTS
new_facts(int id, char *file)
{
  FACTS this;

  this = calloc(1,  FACTSSIZE);
  this->id  = id;
  this->jar = new_cookies();
  __load_cookies(this, file); 
  return this;
}

FACTS
facts_destroy(FACTS this)
{
  if (this != NULL) {
    get_cookies(this);
    this = NULL;
  }
  return this;
}

BOOLEAN
set_cookie(FACTS this, char *line, char *host)
{
  COOKIE tmp = new_cookie(line, host);
  cookies_add(this->jar, tmp, host);
  return TRUE;
}

public char *
get_cookies(FACTS this)
{
  if (!this || !this->jar) return NULL;

  cookies_reset_iterator();

  size_t bufsize = 1024;
  char *result = malloc(bufsize);
  if (!result) return NULL;

  result[0] = '\0'; // Initialize empty string

  size_t len = 0;
  char *cookie_str;

  while ((cookie_str = cookies_next(this->jar)) != NULL) {
    char line[1024];
    int n = snprintf(line, sizeof(line), "siege-%d | %s\n", this->id, cookie_str);

    if (len + n + 1 >= bufsize) {
      bufsize *= 2;
      char *new_result = realloc(result, bufsize);
      if (!new_result) {
        free(result);
        cookies_reset_iterator();
        return NULL;
      }
      result = new_result;
    }

    strcat(result + len, line);
    len += n;
  }

  cookies_reset_iterator();
  return result;
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

private BOOLEAN
__load_cookies(FACTS this, char * file)
{
  FILE * fp;
  const  size_t len = 4096; // max cookie size
  char   line[len];

  snprintf(this->key, sizeof(this->key), "siege-%d", this->id);
  if (! __contains(file, this->key)) {
    return FALSE;
  }

  if (! __exists(file)) {
    return FALSE;
  }

  fp = fopen(file, "r");
  if (fp == NULL) {
    return FALSE;
  }
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
      int    num = 2;
      char   *val;
      char   **pair; 
      COOKIE tmp;
      pair = split('|', line, &num);
      trim(pair[0]);
      trim(pair[1]);
      if (strmatch(this->key, pair[0])) {
        val = strdup(pair[1]);
        tmp = new_cookie(val, NULL);
        cookies_add(this->jar, tmp, NULL);
      }
      split_free(pair, num); 
    }
    memset(line, '\0', len);
  } 
  fclose(fp);
  return TRUE;
}

private BOOLEAN
__contains(const char *filename, const char *target) 
{
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("Could not open file");
    return 0;
  }

  fseek(file, 0, SEEK_END);
  size_t length = ftell(file);
  rewind(file);

  char *buffer = malloc(length + 1);
  if (!buffer) {
    fclose(file);
    perror("Memory allocation failed");
    return 0;
  }

  if (fread(buffer, 1, length, file) != length) {
    fprintf(stderr, "Warning: could not read entire file (expected %zu bytes)\n", length);
    fclose(file); 
    free(buffer); 
    return FALSE; 
  }

  buffer[length] = '\0';  // Null-terminate for string functions
  fclose(file);

  BOOLEAN found = strstr(buffer, target) != NULL;

  free(buffer);
  return found;
}


/**
 * returns TRUE if the file exists,
 */
private BOOLEAN
__exists(char *file)
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

