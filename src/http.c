/**
 * HTTP/HTTPS protocol support 
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
 *--
 */
#include <setup.h>
#include <http.h>
#include <stdio.h>
#include <stdarg.h>
#include <cookies.h>
#include <string.h>
#include <util.h>
#include <load.h>
#include <joedog/defs.h>

#define MAXFILE 10240

private char *__parse_pair(char **str);
private char *__dequote(char *str);

/**
 * HTTPS tunnel; set up a secure tunnel with the
 * proxy server. CONNECT server:port HTTP/1.0
 */
BOOLEAN
https_tunnel_request(CONN *C, char *host, int port)
{
  size_t  rlen, n;
  char    request[256];

  if (C->encrypt == TRUE && auth_get_proxy_required(my.auth)) {
    snprintf(
      request, sizeof(request),
      "CONNECT %s:%d HTTP/1.0\015\012"
      "User-agent: Proxy-User\015\012"
      "\015\012",
      host, port
    );    
    rlen = strlen(request); 
    echo ("%s", request);
    C->encrypt = FALSE;
    if ((n = socket_write(C, request, rlen)) != rlen){
      NOTIFY(ERROR, "HTTP: unable to write to socket." );
      return FALSE;
    }
  } else {
    return FALSE; 
  }
  return TRUE;
}

int
https_tunnel_response(CONN *C)
{
  int  x, n;
  char c;
  char line[256];
  int  code = 100;

  while(TRUE){
    x = 0;
    memset( &line, '\0', sizeof( line ));
    while ((n = read(C->sock, &c, 1)) == 1) {
      line[x] = c;
      echo ("%c", c);
      if((line[0] == '\n') || (line[1] == '\n')){
        return code;
      }
      if( line[x] == '\n' ) break;
      x ++;
    }
    line[x]=0;
    if( strncasecmp( line, "http", 4 ) == 0 ){
      code = atoi(line + 9);
    }
  }
}

BOOLEAN
http_get(CONN *C, URL U)
{
  size_t rlen;
  size_t mlen; 
  char   protocol[16]; 
  char   keepalive[16];
  char   hoststr[512];
  char   authwww[512];
  char   authpxy[512];
  char   accept[] = "Accept: */*\015\012";
  char   encoding[512];
  char   *request;
  char   portstr[16];
  char   fullpath[4096];
  char   cookie[MAX_COOKIE_SIZE+8];
  char * ifmod = url_get_if_modified_since(U);
  char * ifnon = url_get_etag(U);

  memset(hoststr, '\0', sizeof hoststr);
  memset(cookie,  '\0', sizeof cookie);
  memset(portstr, '\0', sizeof portstr);

  /* Request path based on proxy settings */
  if(auth_get_proxy_required(my.auth)){
    sprintf(
      fullpath, "%s://%s:%d%s", C->encrypt == FALSE?"http":"https", 
      url_get_hostname(U), url_get_port(U), url_get_request(U)
    );
  } else {
    sprintf(fullpath, "%s", url_get_request(U));
  }
  if ((url_get_port(U)==80 && C->encrypt==FALSE) || (url_get_port(U)==443 && C->encrypt==FALSE)){
    portstr[0] = '\0';  
  } else {
    snprintf(portstr, sizeof portstr, ":%d", url_get_port(U));
  }

  /**
   * Set the protocol and keepalive strings
   * based on configuration conditions....
   */
  if (my.protocol == FALSE || my.get == TRUE) {
    snprintf(protocol, sizeof(protocol), "HTTP/1.0");
  } else {
    snprintf(protocol, sizeof(protocol), "HTTP/1.1");
  }
  if (C->connection.keepalive == TRUE) {
    snprintf(keepalive, sizeof(keepalive), "keep-alive");
  } else {
    snprintf(keepalive, sizeof(keepalive), "close");
  }

  cookies_header(my.cookies, url_get_hostname(U), cookie);
  if (C->auth.www) {
    if (C->auth.type.www==DIGEST) {
      snprintf (
        authwww, sizeof(authwww), "%s", 
        auth_get_digest_header(my.auth, HTTP, C->auth.wchlg, C->auth.wcred, url_get_method_name(U), fullpath)
      );
    } else if (C->auth.type.www==NTLM) {
      snprintf(authwww, sizeof(authwww), "%s", auth_get_ntlm_header(my.auth, HTTP));
    } else {
      snprintf(authwww, sizeof(authwww), "%s", auth_get_basic_header(my.auth, HTTP));
    }
  }
  if (C->auth.proxy) {
    if (C->auth.type.proxy==DIGEST) {
      snprintf (
        authpxy, sizeof(authpxy), "%s", 
        auth_get_digest_header(my.auth, PROXY, C->auth.pchlg, C->auth.pcred, url_get_method_name(U), fullpath)
      );
    } else  {
      snprintf(authpxy, sizeof(authpxy), "%s", auth_get_basic_header(my.auth, PROXY));
    }
  }

  /* Only send the Host header if one wasn't provided by the configuration. */
  if (strncasestr(my.extra, "host:", sizeof(my.extra)) == NULL) {
    // as per RFC2616 14.23, send the port if it's not default
    if ((url_get_scheme(U) == HTTP  && url_get_port(U) != 80) || 
        (url_get_scheme(U) == HTTPS && url_get_port(U) != 443)) {
      rlen = snprintf(hoststr, sizeof(hoststr), "Host: %s:%d\015\012", url_get_hostname(U), url_get_port(U));
    } else {
      rlen = snprintf(hoststr, sizeof(hoststr), "Host: %s\015\012", url_get_hostname(U));
    }
  }

  mlen = strlen(url_get_method_name(U)) +
         strlen(fullpath) +
         strlen(protocol) +
         strlen(hoststr)  +
         strlen((C->auth.www==TRUE)?authwww:"") +
         strlen((C->auth.proxy==TRUE)?authpxy:"") +
         strlen(cookie) +
         strlen((ifmod!=NULL)?ifmod:"") +
         strlen((ifnon!=NULL)?ifnon:"") +
         strlen((strncasecmp(my.extra, "Accept:", 7)==0) ? "" : accept) +
         sizeof(encoding) +
         strlen(my.uagent) +
         strlen(my.extra) +
         strlen(keepalive) +
         128; 
   request = (char*)xmalloc(mlen);
   memset(request, '\0', mlen);
   memset(encoding, '\0', sizeof(encoding));
   if (! my.get) {
     snprintf(encoding, sizeof(encoding), "Accept-Encoding: %s\015\012", my.encoding); 
   }

  /** 
   * build a request string to pass to the server       
   */
  rlen = snprintf (
    request, mlen,
    "%s %s %s\015\012"                     /* operation, fullpath, protocol */
    "%s"                                   /* hoststr                */
    "%s"                                   /* authwww   or empty str */
    "%s"                                   /* authproxy or empty str */
    "%s"                                   /* cookie    or empty str */
    "%s"                                   /* ifmod     or empty str */
    "%s"                                   /* ifnon     or empty str */
    "%s"                                   /* Conditional Accept:    */
    "%s"                                   /* encoding */
    "User-Agent: %s\015\012"               /* my uagent   */
    "%s"                                   /* my.extra    */
    "Connection: %s\015\012\015\012",      /* keepalive   */
    url_get_method_name(U), fullpath, protocol, hoststr,
    (C->auth.www==TRUE)?authwww:"",
    (C->auth.proxy==TRUE)?authpxy:"",
    (strlen(cookie) > 8)?cookie:"", 
    (ifmod!=NULL)?ifmod:"",
    (ifnon!=NULL)?ifnon:"",
    (strncasecmp(my.extra, "Accept:", 7)==0) ? "" : accept,
    encoding, my.uagent, my.extra, keepalive 
  );

  /**
   * XXX: I hate to use a printf here (as opposed to echo) but we
   * don't want to preface the headers with [debug] in debug mode
   */
  if ((my.debug || my.get) && !my.quiet) { printf("%s\n", request); fflush(stdout); }
  
  if (rlen == 0 || rlen > mlen) { 
    NOTIFY(FATAL, "HTTP %s: request buffer overrun!", url_get_method_name(U));
  }

  if ((socket_write(C, request, rlen)) < 0) {
    xfree(ifmod);
    xfree(ifnon);
    return FALSE;
  }
   
  xfree(ifmod);
  xfree(ifnon);
  xfree(request);
  return TRUE;
}

BOOLEAN
http_post(CONN *C, URL U)
{
  size_t rlen;
  size_t mlen;
  char   hoststr[128];
  char   authwww[128];
  char   authpxy[128]; 
  char   accept[] = "Accept: */*\015\012";
  char   encoding[512];
  char * request; 
  char   portstr[16];
  char   protocol[16]; 
  char   keepalive[16];
  char   cookie[MAX_COOKIE_SIZE];
  char   fullpath[4096];

  memset(hoststr,  '\0', sizeof(hoststr));
  memset(cookie,   '\0', sizeof(cookie));
  memset(portstr,  '\0', sizeof portstr);
  memset(protocol, '\0', sizeof portstr);
  memset(keepalive,'\0', sizeof portstr);

  if (auth_get_proxy_required(my.auth)) {
   sprintf(
      fullpath, 
      "%s://%s:%d%s", 
      C->encrypt == FALSE?"http":"https", url_get_hostname(U), url_get_port(U), url_get_request(U)
    ); 
  } else {
    sprintf(fullpath, "%s", url_get_request(U));
  }

  if ((url_get_port(U)==80 && C->encrypt==FALSE) || (url_get_port(U)==443 && C->encrypt==TRUE)) {
    portstr[0] = '\0';  ;
  } else {
    snprintf(portstr, sizeof portstr, ":%d", url_get_port(U));
  }

  /** 
   * Set the protocol and keepalive strings
   * based on configuration conditions.... 
   */
  if (my.protocol == FALSE || my.get == TRUE) { 
    snprintf(protocol, sizeof(protocol), "HTTP/1.0");
  } else {
    snprintf(protocol, sizeof(protocol), "HTTP/1.1");
  }
  if (C->connection.keepalive == TRUE) {
    snprintf(keepalive, sizeof(keepalive), "keep-alive");
  } else {
    snprintf(keepalive, sizeof(keepalive), "close");
  }

  cookies_header(my.cookies, url_get_hostname(U), cookie);
  if (C->auth.www) {
    if (C->auth.type.www==DIGEST) {
      snprintf (
        authwww, sizeof(authwww), "%s", 
        auth_get_digest_header(my.auth, HTTP, C->auth.wchlg, C->auth.wcred, url_get_method_name(U), fullpath)
      );
    } else if (C->auth.type.www==NTLM) {
      snprintf(authwww, sizeof(authwww), "%s", auth_get_ntlm_header(my.auth, HTTP));
    } else {
      snprintf(authwww, sizeof(authwww), "%s", auth_get_basic_header(my.auth, HTTP));
    }
  }
  if (C->auth.proxy) {
    if (C->auth.type.proxy==DIGEST) {
      snprintf (
        authpxy, sizeof(authpxy), "%s", 
        auth_get_digest_header(my.auth, PROXY, C->auth.pchlg, C->auth.pcred, url_get_method_name(U), fullpath)
      );
    } else  {
      snprintf(authpxy, sizeof(authpxy), "%s", auth_get_basic_header(my.auth, PROXY));
    }
  }

  /* Only send the Host header if one wasn't provided by the configuration. */
  if (strncasestr(my.extra, "host:", sizeof(my.extra)) == NULL) {
    // as per RFC2616 14.23, send the port if it's not default
    if ((url_get_scheme(U) == HTTP  && url_get_port(U) != 80) ||
        (url_get_scheme(U) == HTTPS && url_get_port(U) != 443)) {
      rlen = snprintf(hoststr, sizeof(hoststr), "Host: %s:%d\015\012", url_get_hostname(U), url_get_port(U));
    } else {
      rlen = snprintf(hoststr, sizeof(hoststr), "Host: %s\015\012", url_get_hostname(U));
    }
  }

  mlen = strlen(url_get_method_name(U)) +
         strlen(fullpath) +
         strlen(protocol) +
         strlen(hoststr)  +
         strlen((C->auth.www==TRUE)?authwww:"") +
         strlen((C->auth.proxy==TRUE)?authpxy:"") +
         strlen(cookie) +
         strlen((strncasecmp(my.extra, "Accept:", 7)==0) ? "" : accept) +
         sizeof(encoding) +
         strlen(my.uagent) +
         strlen(url_get_conttype(U)) +
         strlen(my.extra) +
         strlen(keepalive) + 
         url_get_postlen(U) +
         128; 
   request = (char*)xmalloc(mlen);
   memset(request, '\0', mlen);
   memset(encoding, '\0', sizeof(encoding));
   if (! my.get) {
     snprintf(encoding, sizeof(encoding), "Accept-Encoding: %s\015\012", my.encoding); 
   }

  /** 
   * build a request string to
   * post on the web server
   */
  rlen = snprintf (
    request, mlen,
    "%s %s %s\015\012"
    "%s"
    "%s"
    "%s"
    "%s"
    "%s"
    "%s"
    "User-Agent: %s\015\012%s"
    "Connection: %s\015\012"
    "Content-type: %s\015\012"
    "Content-length: %ld\015\012\015\012",
    url_get_method_name(U), fullpath, protocol, hoststr,
    (C->auth.www==TRUE)?authwww:"",
    (C->auth.proxy==TRUE)?authpxy:"",
    (strlen(cookie) > 8)?cookie:"", 
    (strncasecmp(my.extra, "Accept:", 7)==0) ? "" : accept,
    encoding, my.uagent, my.extra, keepalive, url_get_conttype(U), (long)url_get_postlen(U)
  ); 

  if (rlen < mlen) {
    memcpy(request + rlen, url_get_postdata(U), url_get_postlen(U));
    request[rlen+url_get_postlen(U)] = 0;
  }
  rlen += url_get_postlen(U);
 
  if (my.get || my.debug) printf("%s\n\n", request);

  if (rlen == 0 || rlen > mlen) {
    NOTIFY(FATAL, "HTTP %s: request buffer overrun! Unable to continue...", url_get_method_name(U)); 
  }

  if ((socket_write(C, request, rlen)) < 0) {
    return FALSE;
  }

  xfree(request);
  return TRUE;
}

void
http_free_headers(HEADERS *h)
{
  xfree(h->redirect);
  xfree(h->auth.realm.proxy);
  xfree(h->auth.realm.www);
  xfree(h);
}

/**
 * returns HEADERS struct
 * reads from http/https socket and parses
 * header information into the struct.
 */
HEADERS *
http_read_headers(CONN *C, URL U)
{ 
  int  x;           /* while loop index      */
  int  n;           /* assign socket_read    */
  char c;           /* assign char read      */
  HEADERS *h;       /* struct to hold it all */
  char line[MAX_COOKIE_SIZE];  /* assign chars read     */
  
  h = xcalloc(sizeof(HEADERS), 1);
  
  while (TRUE) {
    x = 0;
    memset(&line, '\0', MAX_COOKIE_SIZE);
    while ((n = socket_read(C, &c, 1)) == 1) {
      if (x < MAX_COOKIE_SIZE - 1)
        line[x] = c; 
      else 
        line[x] = '\n';
      echo("%c", c);  
      if ((line[0] == '\n') || (line[1] == '\n')) { 
        return h;
      }
      if (line[x] == '\n') break;
      x ++;
    }
    line[x]=0;
    /* strip trailing CR */
    if (x > 0 && line[x-1] == '\r') line[x-1]=0;
    if (strncasecmp(line, "http", 4) == 0) {
      strncpy( h->head, line, 8);
      h->code = atoi(line + 9); 
    }
    if (strncasecmp(line, "content-length: ", 16) == 0) { 
      C->content.length = atoi(line + 16); 
    }
    if (strncasecmp(line, "set-cookie: ", 12) == 0) {
      if (my.cookies) {
        memset(h->cookie, '\0', sizeof(h->cookie));
        strncpy(h->cookie, line+12, strlen(line));
        cookies_add(my.cookies, h->cookie, url_get_hostname(U));
      }
    }
    if (strncasecmp(line, "connection: ", 12 ) == 0) {
      if (strncasecmp(line+12, "keep-alive", 10) == 0) {
        h->keepalive = 1;
      } else if (strncasecmp(line+12, "close", 5) == 0) {
        h->keepalive = 0;
      }
    }
    if (strncasecmp(line, "keep-alive: ", 12) == 0) {
      char *tmp    = "";
      char *option = "", *value = "";
      char *newline = (char*)line;
      while ((tmp = __parse_pair(&newline)) != NULL) {
        option = tmp;
        while (*tmp && !ISSPACE((int)*tmp) && !ISSEPARATOR(*tmp))
          tmp++;
        *tmp++=0;
        while (ISSPACE((int)*tmp) || ISSEPARATOR(*tmp))
          tmp++;
        value  = tmp;
        while (*tmp)
          tmp++;  
        if (!strncasecmp(option, "timeout", 7)) {
          if(value != NULL){
            C->connection.timeout = atoi(value);
          } else {
            C->connection.timeout = 15;
          }
        }
        if (!strncasecmp(option, "max", 3)) {
          if(value != NULL){
            C->connection.max = atoi(value);
          } else {
            C->connection.max = 0;
          }
        }
      }
    }
    if (strncasecmp(line, "location: ", 10) == 0) {
      size_t len  = strlen(line);
      h->redirect = xmalloc(len);
      memcpy(h->redirect, line+10, len-10);
      h->redirect[len-10] = 0;
    }
    if (strncasecmp(line, "last-modified: ", 15) == 0) {
      char *date;
      size_t len = strlen(line);
      if(my.cache){
        date = xmalloc(len);
        memcpy(date, line+15, len-14);
        url_set_last_modified(U, date);
        xfree(date); 
      }
    }
    if (strncasecmp(line, "etag: ", 6) == 0) {
      char   *etag;
      size_t len = strlen(line);
      if(my.cache){
        etag = xmalloc(len);
        memcpy(etag, line+6, len-5);
        etag[len-1] = '\0';
        url_set_etag(U, etag);
        xfree(etag);
      }
    }
    if (strncasecmp(line, "www-authenticate: ", 18) == 0) {
      char *tmp     = ""; 
      char *option  = ""; 
      char *value   = "";
      char *newline = (char*)line;
      if (strncasecmp(line+18, "digest", 6) == 0) {
        newline += 24;
        h->auth.type.www      = DIGEST;
        h->auth.challenge.www = xstrdup(line+18);
      } else if (strncasecmp(line+18, "ntlm", 4) == 0) {
        newline += 22;
        h->auth.type.www = NTLM;
        h->auth.challenge.www = xstrdup(line+18);
      } else {
        /** 
         * XXX: If a server sends more than one www-authenticate header
         *      then we want to use one we've already parsed. 
         */
        if (h->auth.type.www != DIGEST && h->auth.type.www != NTLM) {
          newline += 23;
          h->auth.type.www = BASIC;
        }
      }
      while ((tmp = __parse_pair(&newline)) != NULL) {
        option = tmp; 
        while (*tmp && !ISSPACE((int)*tmp) && !ISSEPARATOR(*tmp))
          tmp++;
        *tmp++=0;
        while (ISSPACE((int)*tmp) || ISSEPARATOR(*tmp))
          tmp++; 
        value  = tmp;
        while (*tmp)
          tmp++;
        if (!strncasecmp(option, "realm", 5)) {
          if(value != NULL){
	    h->auth.realm.www = xstrdup(__dequote(value));
            url_set_realm(U, __dequote(value));
          } else {
            h->auth.realm.www = xstrdup("");
          }
        }
      } /* end of parse pairs */
    } 
    if (strncasecmp(line, "proxy-authenticate: ", 20) == 0) {
      char *tmp     = ""; 
      char *option  = "", *value = "";
      char *newline = (char*)line;
      if(strncasecmp(line+20, "digest", 6) == 0){
        newline += 26;
        h->auth.type.proxy      = DIGEST;
        h->auth.challenge.proxy = xstrdup(line+20);
      } else {
        newline += 25;
        h->auth.type.proxy = BASIC;
      }
      while((tmp = __parse_pair(&newline)) != NULL){
        option = tmp; 
        while(*tmp && !ISSPACE((int)*tmp) && !ISSEPARATOR(*tmp))
          tmp++;
        *tmp++=0;
        while(ISSPACE((int)*tmp) || ISSEPARATOR(*tmp))
          tmp++; 
        value  = tmp;
        while(*tmp)
          tmp++;
        if (!strncasecmp(option, "realm", 5)) {
          if (value != NULL) {
	    h->auth.realm.proxy = xstrdup(__dequote(value));
          } else {
            h->auth.realm.proxy = xstrdup("");
          }
        }
      } /* end of parse pairs */
    }
    if (strncasecmp(line, "transfer-encoding: ", 19) == 0) {
      if (strncasecmp(line+20, "chunked", 7)) {
        C->content.transfer = CHUNKED; 
      } else if (strncasecmp(line+20, "trailer", 7)) {
        C->content.transfer = TRAILER; 
      } else {
        C->content.transfer = NONE;
      }
    }
    if (strncasecmp(line, "expires: ", 9) == 0) {
      /* printf("%s\n", line+10);  */
    }
    if (strncasecmp(line, "cache-control: ", 15) == 0) {
      /* printf("%s\n", line+15); */
    }
    if (n <=  0) { 
      echo ("read error: %s:%d", __FILE__, __LINE__);
      http_free_headers(h);
      return(NULL); 
    } /* socket closed */
  } /* end of while TRUE */

  return h;
}

int
http_chunk_size(CONN *C)
{
  int    n;
  char   *end;
  size_t length;

  memset(C->chkbuf, '\0', sizeof(C->chkbuf));
  if ((n = socket_readline(C, C->chkbuf, sizeof(C->chkbuf))) < 1) {
    NOTIFY(WARNING, "HTTP: unable to determine chunk size");
    return -1;
  }

  if (((C->chkbuf[0] == '\n')||(strlen(C->chkbuf)==0)||(C->chkbuf[0] == '\r'))) {
    return -1;
  }
 
  errno  = 0;
  if (!isxdigit((unsigned)*C->chkbuf))
    return -1;
  length = strtoul(C->chkbuf, &end, 16);
  if ((errno == ERANGE) || (end == C->chkbuf)) {
    NOTIFY(WARNING, "HTTP: invalid chunk line %s\n", C->chkbuf);
    return 0;
  } else {
    return length;
  }
  return -1;
}
  
/**
 * returns ssize_t
 */
ssize_t
http_read(CONN *C)
{ 
  int    n      = 0;
  char   c;
  int    chunk  = 0;
  size_t bytes  = 0;
  size_t length = 0;
  static char body[MAXFILE];

  if (C == NULL) NOTIFY(FATAL, "Connection is NULL! Unable to proceed"); 

  if (my.get || my.debug) {
    // The is debug / get mode - read a char and print it to stdout
    if (C->content.length > 0) {
      length = (C->content.length < MAXFILE) ? C->content.length:MAXFILE;
      do {
        if ((n = socket_read(C, &c, 1)) == 0)
          break;
        echo ("%c", c);
        bytes += n;
        length = (C->content.length - bytes < MAXFILE)?C->content.length-bytes:MAXFILE;
      } while (bytes < C->content.length);
    } else {
      do {
        /*if (my.chunked && C->content.transfer == CHUNKED) {
          chunk = http_chunk_size(C);
        } */
        if ((n = socket_read(C, &c, 1)) == 0)
          break;
        echo ("%c", c);
        bytes += n;
      } while(TRUE);
    }
  } else {
    // This is non-debug / non-get mode. We'll read into a larger bucket
    if (C->content.length > 0) {
      length = (C->content.length < MAXFILE) ? C->content.length:MAXFILE;
      do {
        memset(body, '\0', sizeof(body));
        if ((n = socket_read(C, body, length)) == 0)
          break;
        bytes += n;
        length = (C->content.length - bytes < MAXFILE)?C->content.length-bytes:MAXFILE; 
      } while (bytes < C->content.length); 
    } else if (my.chunked && C->content.transfer == CHUNKED) {
      int tries = 0;
      while (tries < 128) {
        chunk = http_chunk_size(C);
        if (chunk == 0)
          break;
        else if (chunk < 0) {
          tries ++;
          continue;
        }
        do {
          int n;
          memset(body, '\0', MAXFILE);
          n = socket_read(C, body, (chunk>MAXFILE)?MAXFILE:chunk);
          chunk -= n;
          bytes += n;
        } while(chunk > 0);
      }
    } else {
      do {
        memset(body, '\0', sizeof(body));
        if ((n = socket_read(C, body, sizeof(body))) == 0)
          break;
        bytes += n;
      } while(TRUE);
    }
  }
      
  //if (my.debug||my.get) { printf("\n"); fflush(stdout); }
  echo ("\n"); 
  
  return bytes;
}


/**
 * parses option=value pairs from an
 * http header, see keep-alive: above
 * while(( tmp = __parse_pair( &newline )) != NULL ){
 *   do_something( tmp );
 * }
 */
private char *
__parse_pair(char **str)
{
  int  okay  = 0;
  char *p    = *str;
  char *pair = NULL;
 
  if( !str || !*str ) return NULL;
  /**
   * strip the header label
   */
  while( *p && *p != ' ' )
    p++;
  *p++=0;
  if( !*p ){
    *str   = p;
    return NULL;
  }
 
  pair = p;
  while( *p && *p != ';' && *p != ',' ){
    if( !*p ){
      *str = p;
      return NULL;
    }
    if( *p == '=' ) okay = 1;
    p++;
  }
  *p++ = 0;
  *str = p;
 
  if( okay )
    return pair;
  else
    return NULL;
} 

char *
__rquote(char *str)
{
  char *ptr;
  int   len;

  len = strlen(str);
  for(ptr = str + len - 1; ptr >= str && ISQUOTE((int)*ptr ); --ptr);

  ptr[1] = '\0';

  return str;
}

char *
__lquote(char *str)
{
  char *ptr;
  int  len;

  for(ptr = str; *ptr && ISQUOTE((int)*ptr); ++ptr);

  len = strlen(ptr);
  memmove(str, ptr, len + 1);

  return str;
}

char *
__dequote(char *str)
{
  char *ptr;
  ptr = __rquote(str);
  str = __lquote(ptr);
  return str;
}
