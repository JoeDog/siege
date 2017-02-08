/**
 * Siege environment initialization.
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
 */  
#include <init.h>
#include <setup.h>
#include <auth.h>
#include <util.h>
#include <hash.h>
#include <eval.h>
#include <perl.h>
#include <memory.h>
#include <notify.h>
#include <fcntl.h>
#include <stralloc.h>
#include <version.h>
#include <joedog/boolean.h>
#include <joedog/path.h>
#include <joedog/defs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#define LINESZ 1024

int
init_config( void )
{
  char *e;
  char dir[256];
  int  res;
  struct stat buf;
  BOOLEAN needed = FALSE;

  /**
   * Beginning with siege-3.1.4 the local configuration was 
   * moved to $HOME/.siege/siege.conf Let's configure ourselves
   */
  memset(dir, '\0', sizeof(dir));
  snprintf(dir, sizeof(dir), "%s/.siege", getenv("HOME"));
  if (stat(dir, &buf) < 0 && ENOENT == errno) {
    needed = TRUE;
  } else {
    if (! S_ISDIR(buf.st_mode)) {
      if (unlink(dir) == 0) needed = TRUE;
    }
  } 

  /**
   * We need to create a $HOME/.siege dir and configur ourselves.
   * NOTE: We're going to assume siege.config is installed and in
   * the PATH. If this is not the case, then blame the distributor
   */  
  while (needed) {
    int ret; 

    mkdir(dir, 0750);
    ret = system("siege.config");
    if (WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT))
      break;
    needed = FALSE;
  }

  /** 
   * Check if we were passed the -R switch to use a different siegerc file.
   * If not, check for the presence of the SIEGERC variable, otherwise
   * use default of ~/.siege/siege.conf 
   */
  if(strcmp(my.rc, "") == 0){
    if((e = getenv("SIEGERC")) != NULL){
      snprintf(my.rc, sizeof(my.rc), "%s", e);
    } else {
      snprintf(my.rc, sizeof(my.rc), "%s/.siege/siege.conf", getenv("HOME"));
      if (stat(my.rc, &buf) < 0 && errno == ENOENT) {
        snprintf(my.rc, sizeof(my.rc), CNF_FILE);
      }
    }
  } 

  my.debug          = FALSE;
  my.quiet          = FALSE;
  my.color          = TRUE;
  my.internet       = FALSE;
  my.config         = FALSE;
  my.csv            = FALSE;
  my.fullurl        = FALSE;
  my.escape         = TRUE;
  my.parser         = FALSE;
  my.secs           = -1;
  my.limit          = 255;
  my.reps           = MAXREPS; 
  my.bids           = 5;
  my.login          = FALSE;
  my.failures       = 1024;
  my.failed         = 0;
  my.auth           = new_auth();
  auth_set_proxy_required(my.auth, FALSE);
  auth_set_proxy_port(my.auth, 3128);
  my.timeout        = 30;
  my.timestamp      = FALSE;
  my.chunked        = FALSE;
  my.unique         = TRUE;
  my.extra[0]       = 0;
  my.follow         = TRUE;
  my.zero_ok        = TRUE; 
  my.signaled       = 0;
  my.ssl_timeout    = 300;
  my.ssl_cert       = NULL;
  my.ssl_key        = NULL;
  my.ssl_ciphers    = NULL; 
  my.lurl           = new_array();
  my.cookies        = new_cookies();
  my.nomap          = xcalloc(1, sizeof(LINES));
  my.nomap->index   = 0;
  my.nomap->line    = NULL;

  if ((res = pthread_mutex_init(&(my.lock), NULL)) != 0)
    NOTIFY(FATAL, "unable to initiate lock");
  if ((res = pthread_cond_init(&(my.cond), NULL )) != 0)
    NOTIFY(FATAL, "unable to initiate condition");

  if (load_conf(my.rc) < 0) {
    /** Beginning with siege-3.1.4 we should never get here (in theory) **/
    fprintf( stderr, "**************************************************\n" );
    fprintf( stderr, "siege: could not open %s\n", my.rc );
    fprintf( stderr, "run \'siege.config\' to generate a new config file\n" );
    fprintf( stderr, "**************************************************\n" );
    return -1;
  }
  
  if (strlen(my.file) < 1) { 
    snprintf(
      my.file, sizeof( my.file ),
      "%s", URL_FILE
    );
  }

  if (strlen(my.uagent) < 1) 
    snprintf( 
      my.uagent, sizeof(my.uagent),
      "Mozilla/5.0 (%s) Siege/%s", PLATFORM, version_string 
    );

  if (strlen(my.conttype) < 1) 
    snprintf( 
      my.conttype, sizeof(my.conttype),
      "application/x-www-form-urlencoded"
    );

  if (strlen(my.encoding) < 1)
    snprintf(
      my.encoding, sizeof(my.encoding), "*"
    );

  if (strlen(my.logfile) < 1) 
    snprintf( 
      my.logfile, sizeof(my.logfile),
      "%s", LOG_FILE 
    );
  return 0;  
}

int
show_config(int EXIT)
{
  char *method;
  switch (my.method) {
    case GET:
      method = strdup("GET");
      break;
    case HEAD:
    default: 
      method = strdup("HEAD");
      break;
  }
  printf("CURRENT  SIEGE  CONFIGURATION\n");
  printf("%s\n", my.uagent); 
  printf("Edit the resource file to change the settings.\n");
  printf("----------------------------------------------\n");
  printf("version:                        %s\n", version_string);
  printf("verbose:                        %s\n", my.verbose  ? "true"     : "false");
  printf("color:                          %s\n", my.color  ? "true"     : "false");
  printf("quiet:                          %s\n", my.quiet    ? "true"     : "false");
  printf("debug:                          %s\n", my.debug    ? "true"     : "false");
  printf("protocol:                       %s\n", my.protocol ? "HTTP/1.1" : "HTTP/1.0");
  printf("HTML parser:                    %s\n", my.parser   ? "enabled"  : "disabled");
  printf("get method:                     %s\n", method);
  if (auth_get_proxy_required(my.auth)){
    printf("proxy-host:                     %s\n", auth_get_proxy_host(my.auth));
    printf("proxy-port:                     %d\n", auth_get_proxy_port(my.auth));
  }
  printf("connection:                     %s\n", my.keepalive?"keep-alive":"close");
  printf("concurrent users:               %d\n", my.cusers);
  if (my.secs > 0)
    printf( "time to run:                    %d seconds\n", my.secs);
  else
    printf( "time to run:                    n/a\n");
  if ((my.reps > 0) && (my.reps != MAXREPS))
    printf( "repetitions:                    %d\n", my.reps);
  else
    printf("repetitions:                    n/a\n" );
  printf("socket timeout:                 %d\n", my.timeout);
  printf("cache enabled:                  %s\n", my.cache==TRUE ? "true" : "false");
  printf("accept-encoding:                %s\n", my.encoding);
  printf("delay:                          %.3f sec%s\n", my.delay, (my.delay > 1) ? "s" : "");
  printf("internet simulation:            %s\n", my.internet?"true":"false");
  printf("benchmark mode:                 %s\n", my.bench?"true":"false");
  printf("failures until abort:           %d\n", my.failures);
  printf("named URL:                      %s\n", my.url==NULL||strlen(my.url) < 2 ? "none" : my.url);
  printf("URLs file:                      %s\n", strlen(my.file) > 1 ? my.file : URL_FILE);
  printf("thread limit:                   %d\n", (my.limit < 1) ? 255 : my.limit);
  printf("logging:                        %s\n", my.logging ? "true" : "false");
  printf("log file:                       %s\n", (my.logfile == NULL) ? LOG_FILE : my.logfile);
  printf("resource file:                  %s\n", my.rc);
  printf("timestamped output:             %s\n", my.timestamp?"true":"false");
  printf("comma separated output:         %s\n", my.csv?"true":"false");
  printf("allow redirects:                %s\n", my.follow?"true":"false");
  printf("allow zero byte data:           %s\n", my.zero_ok?"true":"false"); 
  printf("allow chunked encoding:         %s\n", my.chunked?"true":"false"); 
  printf("upload unique files:            %s\n", my.unique?"true":"false"); 
  if (my.parser == TRUE && my.nomap->index > 0) {
    int i;
    printf("no-follow:\n"); 
    for (i = 0; i < my.nomap->index; i++) {
      printf(" - %s\n", my.nomap->line[i]);
    } 
  }
  //printf("proxy auth:                     " ); display_authorization(PROXY);printf("\n");
  //printf("www auth:                       " ); display_authorization(WWW); 
  printf("\n");

  xfree(method);
  my.auth    = auth_destroy(my.auth);
  my.lurl    = array_destroy(my.lurl);
  my.cookies = cookies_destroy(my.cookies); 

  if (EXIT) exit(0);
  else return 0;
}

int
readline(char **s, FILE *fp)
{
  int  c;
  int  i;
  int  len  = 0;
  int  size = 0;
  char *tmp = NULL;
  char *ptr = NULL;

  ptr  = xmalloc(LINESZ);
  size = LINESZ;

  while ((c = fgetc(fp))!=EOF && c!='\n') {
    if (len >= size) {
      // the buffer is full - extend it
      tmp = realloc(ptr, size + LINESZ);
      if (tmp == NULL) {
        free(ptr);
        return -1;
      }
      ptr = tmp;
      size += LINESZ;
    }
    ptr[len++] = (char)c;
  }
  if (len == 0) {
    if (c == EOF) { /* empty file or read error */
      free(ptr);
      return -1;
    } else { /* empty line */
      ptr[0] = '\0';
      *s = ptr;
      return 0;
    }
  }

  if (len+1 != size) { /* no room for '\0' */
    tmp = realloc(ptr, len+1);
    if (tmp == NULL) {
      free(ptr);
      return -1;
    }
    ptr = tmp;
  }
  ptr[len] = '\0';

  for (i = 0; ptr[i] != '\0'; i++) {
    if (ptr[i] == '#') {
      ptr[i] = '\0';
    }
  }
  *s = ptr;

  return len;
}

int
load_conf(char *filename)
{
  FILE    *fp;
  HASH    H;
  char    *line;
  char    *option;
  char    *optionptr;
  char    *value;
#ifdef  HAVE_ZLIB
  BOOLEAN zlib = TRUE;
#else  
  BOOLEAN zlib = FALSE;
#endif/*HAVE_ZLIB*/
 
  if ((fp = fopen(filename, "r")) == NULL) {
    return -1;
  } 

  H = new_hash();

  while (readline(&line, fp) != -1) {
    char *tmp = line;
    line = trim(line);
    if (*line == '#' || *line == '\0' || strlen(line) < 1) {
      free(line);
      continue;
    }

    optionptr = option = xstrdup(line);
    while (*optionptr && !ISSPACE((int)*optionptr) && !ISSEPARATOR(*optionptr)) {
      optionptr++;
    }
    *optionptr++='\0';

    while (ISSPACE((int)*optionptr) || ISSEPARATOR(*optionptr)) {
      optionptr++;
    }
    value = xstrdup(optionptr);
    while (*line)
      line++;  
    while (strstr(option, "$")) {
      option = evaluate(H, option);
    }
    while (strstr(value, "$")){
      value = evaluate(H, value);
    } 
    if (strmatch(option, "verbose")) {
      if (!strncasecmp(value, "true", 4))
        my.verbose = TRUE;
      else
        my.verbose = FALSE;
    } 
    else if (strmatch(option, "color")) {
      if (strmatch(value, "false") || strmatch(value, "off"))
        my.color = FALSE;
      else
        my.color = TRUE;
    } 
    else if (strmatch(option, "quiet")) {
      if (!strncasecmp(value, "true", 4))
        my.quiet = TRUE;
      else
        my.quiet = FALSE;
    } 
    else if (strmatch(option, "parser")) {
      if (!strncasecmp(value, "true", 4))
        my.parser = TRUE;
      else
        my.parser = FALSE;
    } 
    else if (strmatch(option, "nofollow")) {
      if (value && strlen(value) > 3) {
        my.nomap->line = (char**)realloc(my.nomap->line, sizeof(char *) * (my.nomap->index + 1));
        my.nomap->line[my.nomap->index] = (char *)strdup(value);
        my.nomap->index++;
      }
    }
    else if (strmatch(option, "csv")) {
      if (!strncasecmp(value, "true", 4))
        my.csv = TRUE;
      else
        my.csv = FALSE;
    } 
    else if (strmatch(option, "fullurl")) {
      if (!strncasecmp(value, "true", 4))
        my.fullurl = TRUE;
      else
        my.fullurl = FALSE;
    } 
    else if (strmatch(option, "display-id")) {
      if (!strncasecmp(value, "true", 4))
        my.display = TRUE;
      else
        my.display = FALSE;
    } 
    else if (strmatch( option, "logging")) {
      if (!strncasecmp( value, "true", 4))
        my.logging = TRUE;
      else
        my.logging = FALSE;
    }
    else if (strmatch(option, "show-logfile")) {
      if (!strncasecmp(value, "true", 4))
        my.shlog = TRUE;
      else
        my.shlog = FALSE;
    }
    else if (strmatch(option, "logfile")) {
      strncpy(my.logfile, value, sizeof(my.logfile)); 
    } 
    else if (strmatch(option, "concurrent")) {
      if (value != NULL) {
        my.cusers = atoi(value);
      } else {
        my.cusers = 10;
      }
    } 
    else if (strmatch(option, "reps")) {
      if (value != NULL) {
        my.reps = atoi(value);
      } else {
        my.reps = 5;
      }
    }
    else if (strmatch(option, "limit")) {
      if (value != NULL) {
        my.limit = atoi(value);
      } else {
        my.limit = 255;
      }
    }
    else if (strmatch(option, "time")) {
      parse_time(value);
    }
    else if (strmatch(option, "delay")) {
      if (value != NULL) {
        my.delay = atof(value);
      } else {
        my.delay = 1;
      }
    }
    else if (strmatch(option, "timeout")) {
      if (value != NULL) {
        my.timeout = atoi(value);
      } else {
        my.timeout = 15;
      }
    }
    else if (strmatch(option, "timestamp")) {
      if (!strncasecmp(value, "true", 4)) 
        my.timestamp = TRUE;
      else
        my.timestamp = FALSE;
    }
    else if (strmatch(option, "internet")) {
      if (!strncasecmp(value, "true", 4))
        my.internet = TRUE;
      else
        my.internet = FALSE;
    }
    else if (strmatch(option, "benchmark")) {
      if (!strncasecmp(value, "true", 4)) 
        my.bench = TRUE;
      else
        my.bench = FALSE;
    }
    else if (strmatch(option, "cache")) {
      if (!strncasecmp(value, "true", 4)) 
        my.cache = TRUE;
      else
        my.cache = FALSE;
    }
    else if (strmatch( option, "debug")) {
      if (!strncasecmp( value, "true", 4))
        my.debug = TRUE;
      else
        my.debug = FALSE;
    }
    else if (strmatch(option, "gmethod")) {
      if (strmatch(value, "GET")) {
        my.method = GET; 
      } else {
        my.method = HEAD; 
      } 
    }
    else if (strmatch(option, "file")) {
      memset(my.file, 0, sizeof(my.file));
      strncpy(my.file, value, sizeof(my.file));
    }
    else if (strmatch(option, "url")) {
      my.url = stralloc(value);
    }
    else if (strmatch(option, "user-agent")) {
      strncpy(my.uagent, value, sizeof(my.uagent));
    }
    else if (strmatch(option, "accept-encoding")) {
      BOOLEAN compress = FALSE;
      if ((strstr(value, "gzip") != NULL)||(strstr(value, "compress") != NULL)) {
        compress = TRUE;
      }
      if (compress == TRUE && zlib == FALSE) {
        NOTIFY(WARNING, "Zip encoding disabled; siege requires zlib support to enable it");
      } else {
        strncpy(my.encoding, value, sizeof(my.encoding));
      }
    }
    #if 1
    else if (!strncasecmp(option, "login", 5)) {
      if(strmatch(option, "login-url")){  
        my.login = TRUE;
        array_push(my.lurl, value);
      } else {
        /* user login info */
        auth_add(my.auth, new_creds(HTTP, value));
      }
    }
    #endif 
    else if (strmatch(option, "attempts")) {
      if (value != NULL) {
        my.bids = atoi(value);
      } else { 
        my.bids = 3;
      }
    }
    else if (strmatch(option, "connection")) {
      if (!strncasecmp(value, "keep-alive", 10))
        my.keepalive = TRUE;
      else
        my.keepalive = FALSE; 
    }
    else if (strmatch(option, "protocol")) {
      if (!strncasecmp(value, "HTTP/1.1", 8))
        my.protocol = TRUE;
      else
        my.protocol = FALSE; 
    }
    else if (strmatch(option, "proxy-host")) {
      auth_set_proxy_host(my.auth, trim(value));
    }
    else if (strmatch(option, "proxy-port")) {
      if (value != NULL) {
        auth_set_proxy_port(my.auth, atoi(value));
      } else {
        auth_set_proxy_port(my.auth, 3128);
      }
    } 
    else if (strmatch(option, "ftp-login")) {
      auth_add(my.auth, new_creds(FTP, value));
    }
    else if (strmatch(option, "proxy-login")) {
      auth_add(my.auth, new_creds(PROXY, value));
    }
    else if (strmatch(option, "failures")) {
      if (value != NULL) {
        my.failures = atoi(value);
      } else {
        my.failures = 30;
      }
    }
    else if (strmatch(option, "chunked")) {
      if (!strncasecmp(value, "true", 4))
        my.chunked = TRUE;
      else
        my.chunked = FALSE;  
    }
    else if (strmatch(option, "unique")) {
      if (!strncasecmp(value, "true", 4))
        my.unique = TRUE;
      else
        my.unique = FALSE;  
    }
    else if (strmatch(option, "header")) {
      if (!strchr(value,':')) NOTIFY(FATAL, "no ':' in http-header");
      if ((strlen(value) + strlen(my.extra) + 3) > 512) NOTIFY(FATAL, "too many headers");
      strcat(my.extra,value);
      strcat(my.extra,"\015\012");
    }
    else if (strmatch(option, "expire-session")) {
      if (!strncasecmp(value, "true", 4 ))
        my.expire = TRUE;
      else
        my.expire = FALSE;
    }
    else if (strmatch(option, "follow-location")) {
      if (!strncasecmp(value, "true", 4))
        my.follow = TRUE;
      else
        my.follow = FALSE;
    }
    else if (strmatch(option, "url-escaping")) {
      if (!strncasecmp(value, "false", 5))
        my.escape = FALSE;
      else
        my.escape = TRUE;
    } 
    else if (strmatch(option, "zero-data-ok")) {
      if (!strncasecmp(value, "true", 4))
        my.zero_ok = TRUE;
      else
        my.zero_ok = FALSE;
    } 
    else if (strmatch(option, "ssl-cert")) {
      my.ssl_cert = stralloc(value);
    }
    else if (strmatch(option, "ssl-key")) {
      my.ssl_key = stralloc(value);
    }
    else if (strmatch(option, "ssl-timeout")) {
      if (value != NULL) {
        my.ssl_timeout = atoi(value);
      } else {
        my.ssl_timeout = 15;
      }
    }
    else if (strmatch(option, "ssl-ciphers")) {
      my.ssl_ciphers = stralloc(value);
    } 
    else if (strmatch(option, "spinner")) {
      if (!strncasecmp(value, "true", 4))
        my.spinner = TRUE;
      else
        my.spinner = FALSE;
    } else {
      hash_add(H, option, value);
    }
    xfree(tmp);
    xfree(value);
    xfree(option);
  }
  H = hash_destroy(H);
  fclose(fp);
  return 0;
}

/**
 * don't be insulted, the author is the DS Module   ;-)
 */ 
void
ds_module_check(void)
{
  if (my.bench) { 
#if defined(hpux) || defined(__hpux)
    my.delay = 1; 
#else
    my.delay = 0; 
#endif
  }

  if (my.secs > 0 && ((my.reps > 0) && (my.reps != MAXREPS))) {
    NOTIFY(ERROR, "CONFIG conflict: selected time and repetition based testing" );
    fprintf( stderr, "defaulting to time-based testing: %d seconds\n", my.secs );
    my.reps = MAXREPS;
  }

  if (my.cusers <= 0) {   /* set concurrency to 1 */
    my.cusers = 1;        /* if not defined       */
  }

  if (my.get) {
    my.cusers  = 1;
    my.reps    = 1;
    my.logging = FALSE;
    my.bench   = TRUE;
  }
  
  if (my.quiet) { 
    my.verbose = FALSE; // Why would you set quiet and verbose???
    my.debug   = FALSE; // why would you set quiet and debug?????
  }
}

