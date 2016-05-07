/**
 * Configuration file support
 *
 * Copyright (C) 2000-2015 by
 * Jeffrey Fulmer - <jeff@joedog.org>, et al. 
 * This file is distributed as part of Siege 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * --
 *
 */
#include <setup.h>
#include <hash.h>
#include <eval.h>
#include <perl.h>
#include <memory.h>
#include <notify.h>
#include <joedog/defs.h>
#include <joedog/boolean.h>

BOOLEAN is_variable_line(char *line);

/** 
 * Ignores comment lines beginning with 
 * '#' empty lines beginning with \n 
 * Takes a char* as an argument        
 */
void 
parse(char *str)
{
  char *ch;
  char *sp;
  char *sl;

  /**
   * An indented comment could be problematic.
   * Let's trim the string then see if the first
   * character is a comment.
   */
  str = trim(str);
  if (str[0] == '#') { 
    str[0] = '\0';
  }

  sp = strchr(str, ' ');
  sl = strchr(str, '/');
  if (sl==NULL && sp != NULL) {
    ch = (char *)strstr(str, "#"); 
    if (ch) {*ch = '\0';}
  }
  ch = (char *)strstr(str, "\n"); 
  if (ch) {*ch = '\0';}

  trim(str);
}

/** 
 * Reads filename into memory and populates
 * the config_t struct with the result. Uses
 * parse to ignore comments and empty lines. 
 */ 
int 
read_cfg_file(LINES *l, char *filename)
{
  /* file pointer  */
  FILE *file; 
  HASH H;
  char *line;
  char *option;
  char *value;

  /* char array to hold contents */
  
  /* make sure LINES has been initialized. */
  if (!l) {	
    printf("Structure not initialized!\n");
    return -1;
  }

  if ((file = fopen(filename, "r")) == NULL) {
    /* this is a fatal problem, but we want  
       to enlighten the user before dying   */
    NOTIFY(WARNING, "unable to open file: %s", filename);
    display_help();
    exit(EXIT_FAILURE);
  }
 
  line = xmalloc(BUFSIZE);
  memset(line, '\0', BUFSIZE);

  H = new_hash();

  l->index = 0;
  while (fgets(line, BUFSIZE, file) != NULL) {
    int  num; char *p = strchr(line, '\n');      
    /**
     * if the line is longer than our buffer, we're 
     * just going to chuck it rather then fsck with it.
     */
    if(p) { 
      *p = '\0';
    } else {  
      /**
       * Small fix by Gargoyle - 19/07/2006
       * Check to see if we are at the end of the file. If so
       * keep the line, otherwise throw it away!
       */
      if ((num = fgetc(file)) != EOF) {
        while ((num = fgetc(file)) != EOF && num != '\n');
        line[0]='\0';
      }
    }
    parse(line);
    chomp(line);
    if (strlen(line) == 0);
    else if (is_variable_line(line)) {
      char *tmp = line;
      option = tmp;
      while (*tmp && !ISSPACE((int)*tmp) && !ISSEPARATOR(*tmp))
        tmp++; 
      *tmp++=0;
      while (ISSPACE((int)*tmp) || ISSEPARATOR(*tmp))
        tmp++;
      value  = tmp;
      while (*tmp)
        tmp++;
      *tmp++=0;
      hash_add(H, option, value); 
    } else {
    char *tmp = xstrdup(line);
      while (strstr(tmp, "$")) {
        tmp = evaluate(H, tmp);
      }
      l->line = (char**)realloc(l->line, sizeof(char *) * (l->index + 1));
      l->line[l->index] = (char *)strdup(tmp);
      l->index++;	
      
      free(tmp);
    }
    memset(line, 0, BUFSIZE);
  }

  fclose(file);
  xfree(line);
  hash_destroy(H);
  return l->index;
}

int
read_cmd_line(LINES *l, char *url)
{
  int x = 0;
  /* char array to hold contents */
  char head[BUFSIZE];
 
  /* make sure config_t has been initialized. */
  if(!l){
    printf("Structure not initialized!\n");
    return -1;
  }
 
  l->index = 0;
  while(x < 4){
  snprintf(head, sizeof(head), "%s", url);
  parse(head);
  chomp(head);
  if (strlen(head) == 0);
  else {
    l->line = (char**)realloc(l->line, sizeof(char *) * (l->index + 1));
    l->line[l->index] = (char *)strdup(head);
    l->index++;
  }
  x++;
  }
  return l->index;
} 

BOOLEAN
is_variable_line(char *line)
{
  char *pos, *x;
  char c;

  /**
   * check for variable assignment; make sure that on the left side 
   * of the = is nothing but letters, numbers, and/or underscores.
   */
  pos = strstr(line, "=");
  if (pos != NULL) {
    for (x = line; x < pos; x++) {
      c = *x;
      /* c must be A-Z, a-z, 0-9, or underscore. */
      if ((c < 'a' || c > 'z') &&
          (c < 'A' || c > 'Z') &&
          (c < '0' || c > '9') &&
          (c != '_')){
            return FALSE;
      }
    }
  } else {
    /** 
     * if it has no '=' then it can't be a variable line 
     */
    return FALSE;
  }
  return TRUE;
}

