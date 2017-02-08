/**
 * HTTP/HTTPS protocol support 
 *
 * Copyright (C) 2000-2016 by
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
#ifdef  HAVE_ZLIB
# include <zlib.h>
#endif/*HAVE_ZLIB*/
#include <cookies.h>
#include <string.h>
#include <util.h>
#include <load.h>
#include <page.h>
#include <memory.h>
#include <notify.h>
#include <response.h>
#include <joedog/defs.h>

#define MAXFILE 0x10000 // 65536

#ifndef HAVE_ZLIB
#define MAX_WBITS 1024 // doesn't matter - we don't use it
#endif/*HAVE_ZLIB*/

pthread_mutex_t __mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  __cond  = PTHREAD_COND_INITIALIZER;

private int     __gzip_inflate(int window, const char *src, int srcLen, const char *dst, int dstLen);

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
  char   fullpath[MAX_COOKIE_SIZE*2];
  char   cookie[MAX_COOKIE_SIZE+8];
  char * ifnon = NULL;
  char * ifmod = NULL; 

  memset(hoststr, '\0', sizeof hoststr);
  memset(cookie,  '\0', sizeof cookie);
  memset(portstr, '\0', sizeof portstr);

  ifnon = cache_get_header(C->cache, C_ETAG, U);
  ifmod = cache_get_header(C->cache, C_LAST, U);

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
  if (my.protocol == FALSE || my.get == TRUE || my.print == TRUE) {
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
   if (! my.get || ! my.print) {
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
  if ((my.debug || my.get || my.print) && !my.quiet) { printf("%s\n", request); fflush(stdout); }
  
  if (rlen == 0 || rlen > mlen) { 
    NOTIFY(FATAL, "HTTP %s: request buffer overrun!", url_get_method_name(U));
  }
  //XXX: printf("%s", request);
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
  char   fullpath[MAX_COOKIE_SIZE*2];

  memset(hoststr,  '\0', sizeof(hoststr));
  memset(cookie,   '\0', MAX_COOKIE_SIZE);
  memset(portstr,  '\0', sizeof(portstr));
  memset(protocol, '\0', sizeof(protocol));
  memset(keepalive,'\0', sizeof(keepalive));

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
  if (my.protocol == FALSE || my.get == TRUE || my.print == TRUE) { 
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
    } else if(C->auth.type.www==NTLM) {
      snprintf(authwww, sizeof(authwww), "%s", auth_get_ntlm_header(my.auth, HTTP));
    } else {
      snprintf(authwww, sizeof(authwww), "%s", auth_get_basic_header(my.auth, HTTP));
    }
  }
  if (C->auth.proxy) {
    if (C->auth.type.proxy==DIGEST) {
      snprintf (
        authpxy, sizeof(authpxy), "%s", 
        auth_get_digest_header(my.auth, HTTP, C->auth.pchlg, C->auth.pcred, url_get_method_name(U), fullpath)
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
   if (! my.get || ! my.print) {
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
 
  if (my.get || my.debug || my.print) printf("%s\n\n", request);

  if (rlen == 0 || rlen > mlen) {
    NOTIFY(FATAL, "HTTP %s: request buffer overrun! Unable to continue...", url_get_method_name(U)); 
  }

  if ((socket_write(C, request, rlen)) < 0) {
    return FALSE;
  }

  xfree(request);
  return TRUE;
}

/**
 * returns HEADERS struct
 * reads from http/https socket and parses
 * header information into the struct.
 */
RESPONSE
http_read_headers(CONN *C, URL U)
{ 
  int  x;
  int  n; 
  char c; 
  char line[MAX_COOKIE_SIZE]; 
  RESPONSE resp = new_response();
  
  while (TRUE) {
    x = 0;
    //memset(&line, '\0', MAX_COOKIE_SIZE); //VL issue #4
    while ((n = socket_read(C, &c, 1)) == 1) {
      if (x < MAX_COOKIE_SIZE - 1)
        line[x] = c; 
      else 
        line[x] = '\n';
      echo("%c", c);
      if (x <= 1 && line[x] == '\n') { //VL issue #4, changed from (line[0] == '\n' || line[1] == '\n')
			return resp; 
      }
      if (line[x] == '\n') break;
      x ++;
    }
    line[x]='\0';

    // string carriage return
    if (x > 0 && line[x-1] == '\r') line[x-1]='\0';

    if (strncasecmp(line, "http", 4) == 0) {
      response_set_code(resp, line);
    }
    if (strncasecmp(line, CONTENT_TYPE, strlen(CONTENT_TYPE)) == 0) {
      response_set_content_type(resp, line);
    }
    if (strncasecmp(line, CONTENT_ENCODING, strlen(CONTENT_ENCODING)) == 0) {
      response_set_content_encoding(resp, line);
    }
    if (strncasecmp(line, CONTENT_LENGTH, strlen(CONTENT_LENGTH)) == 0) { 
      response_set_content_length(resp, line);
      C->content.length = atoi(line + 16); 
    }
    if (strncasecmp(line, SET_COOKIE, strlen(SET_COOKIE)) == 0) {
      if (my.cookies) {
        char tmp[MAX_COOKIE_SIZE];
        memset(tmp, '\0', MAX_COOKIE_SIZE);
        strncpy(tmp, line+12, strlen(line));
        cookies_add(my.cookies, tmp, url_get_hostname(U));
      }
    }
    if (strncasecmp(line, CONNECTION, strlen(CONNECTION)) == 0) {
      response_set_connection(resp, line);
    }
    if (strncasecmp(line, "keep-alive: ", 12) == 0) {
      if (response_set_keepalive(resp, line) == TRUE) {
        C->connection.timeout = response_get_keepalive_timeout(resp);
        C->connection.max     = response_get_keepalive_max(resp);
      } 
    }
    if (strncasecmp(line, LOCATION, strlen(LOCATION)) == 0) {
      response_set_location(resp, line);
    }
    if (strncasecmp(line, LAST_MODIFIED, strlen(LAST_MODIFIED)) == 0) {
      response_set_last_modified(resp, line);
      char *date;
      size_t len = strlen(line);
      if(my.cache){
        date = xmalloc(len);
        memcpy(date, line+15, len-14);
        cache_add(C->cache, C_LAST, U, date);
        xfree(date); 
      }
    }
    if (strncasecmp(line, ETAG, strlen(ETAG)) == 0) {
      char   *etag;
      size_t len = strlen(line);
      if (my.cache) {
        etag = (char *)xmalloc(len);
        memset(etag, '\0', len);
        memcpy(etag, line+6, len-5);
        cache_add(C->cache, C_ETAG, U, etag);
        xfree(etag);
      }
    }
    if (strncasecmp(line, WWW_AUTHENTICATE, strlen(WWW_AUTHENTICATE)) == 0) {
      response_set_www_authenticate(resp, line);
    } 
    if (strncasecmp(line, PROXY_AUTHENTICATE, strlen(PROXY_AUTHENTICATE)) == 0) {
      response_set_proxy_authenticate(resp, line);
    }                     
    if (strncasecmp(line, TRANSFER_ENCODING, strlen(TRANSFER_ENCODING)) == 0) {
      response_set_transfer_encoding(resp, line);
    }
    if (strncasecmp(line, EXPIRES, strlen(EXPIRES)) == 0) {
      char   *expires;
      size_t  len = strlen(line); 
      if (my.cache) {
        expires = (char *)xmalloc(len);
        memset(expires, '\0', len);
        memcpy(expires, line+9, len-8);
        cache_add(C->cache, C_EXPIRES, U, expires);
        xfree(expires);
      }
    }
    if (strncasecmp(line, "cache-control: ", 15) == 0) {
      /* printf("%s\n", line+15); */
    }
    if (n <=  0) { 
      echo ("read error: %s:%d", __FILE__, __LINE__);
      resp = response_destroy(resp);
      return resp; 
    } /* socket closed */
  } /* end of while TRUE */

  return resp;
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
  
ssize_t
http_read(CONN *C, RESPONSE resp)
{ 
  int    n      = 0;
  int    chunk  = 0;
  size_t bytes  = 0;
  size_t length = 0;
  char   dest[MAXFILE*6];
  char   *ptr = NULL;
  char   *tmp = NULL;
  size_t size = MAXFILE; 

  if (C == NULL) {
	  NOTIFY(FATAL, "Connection is NULL! Unable to proceed"); 
	  return 0;
  }

  if (C->content.length == 0) //VL
	  return 0;
  else if (C->content.length == (size_t)~0L)
	  C->content.length = 0; //not to break code below...
  
  memset(dest, '\0', sizeof dest);

  //pthread_mutex_lock(&__mutex);  //VL - moved
  
  if (C->content.length > 0) {
    length = C->content.length;
    ptr    = xmalloc(length+1);
    memset(ptr, '\0', length+1);
    do {
      if ((n = socket_read(C, ptr, length)) == 0) {
        break;
      }
      bytes += n;
    } while (bytes < length); 
  } else if (my.chunked && response_get_transfer_encoding(resp) == CHUNKED) {
    int  r = 0;
    bytes  = 0;
    BOOLEAN done = FALSE;

    ptr = xmalloc(size);
    memset(ptr, '\0', size);

    do {
      chunk = http_chunk_size(C);
      if (chunk == 0){
        socket_readline(C, C->chkbuf, sizeof(C->chkbuf)); //VL - issue #3
        break;
      } else if (chunk < 0) {
        chunk = 0;
        continue;
      }  
      while (n < chunk) {
        int remaining_in_chunk = chunk - n;
        int space_in_buf = size - bytes;
        int to_read = remaining_in_chunk < space_in_buf ? remaining_in_chunk : space_in_buf;
        r = socket_read(C, &ptr[bytes], to_read); 
        bytes += r;
        if (r <= 0) {
          done = TRUE;
          break;
        }
        n += r;
        if (bytes >= size) {
          tmp = realloc(ptr, size*2);
          if (tmp == NULL) {
            free(ptr);
            return -1; 
          }
          ptr   = tmp;
          size *= 2;
        }
      }
      n = 0;
    } while (! done);
    ptr[bytes] = '\0';
  } else {
    ptr = xmalloc(size);
    memset(ptr, '\0', size);
    do {
      n = socket_read(C, ptr, size);
      bytes += n;
      if (n <= 0) { 
        break;
      }
    } while (TRUE);
  }

  if (response_get_content_encoding(resp) == GZIP) {
    __gzip_inflate(MAX_WBITS+32, ptr, bytes, dest, sizeof(dest));
  }
  if (response_get_content_encoding(resp) == DEFLATE) {
    __gzip_inflate(-MAX_WBITS, ptr, bytes, dest, sizeof(dest));
  }
  if (strlen(dest) > 0) {
    page_concat(C->page, dest, strlen(dest));
  } else {
    page_concat(C->page, ptr, strlen(ptr));
  }
  xfree(ptr);
  echo ("\n");
  //pthread_mutex_unlock(&__mutex);
  return bytes;
}

private int
__gzip_inflate(int window, const char *src, int srcLen, const char *dst, int dstLen)
{
  int err=-1;

#ifndef HAVE_ZLIB
  NOTIFY(ERROR,
    "gzip transfer-encoding requires zlib (%d, %d, %d)", window, srcLen, dstLen
  );
  dst = strdup(src);
  (void)(dst); // shut the compiler up....
  return err;
#else
  z_stream strm;
  int ret        = -1;
  strm.zalloc    = Z_NULL;
  strm.zfree     = Z_NULL;
  strm.opaque    = Z_NULL;
  strm.avail_in  = srcLen;
  strm.avail_out = dstLen;
  strm.next_in   = (Bytef *)src;
  strm.next_out  = (Bytef *)dst;

  err = inflateInit2(&strm, window);
  if (err == Z_OK) {
    err = inflate(&strm, Z_FINISH);
    if (err == Z_STREAM_END){
      ret = strm.total_out;
    } else {
      inflateEnd(&strm);
      return err;
    }
  } else {
    inflateEnd(&strm);
    return err;
  }
  inflateEnd(&strm);
  return ret;
#endif/*HAVE_ZLIB*/
}

