/**
 * Browser instance
 *
 * Copyright (C) 2016 by
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
#ifdef  HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

#include <setup.h>
#include <signal.h>
#include <sock.h>
#include <ssl.h>
#include <ftp.h>
#include <http.h>
#include <hash.h>
#include <array.h>
#include <util.h>
#include <parser.h>
#include <perl.h>
#include <response.h>
#include <memory.h>
#include <notify.h>
#include <browser.h>

#if defined(hpux) || defined(__hpux) || defined(WINDOWS)
# define SIGNAL_CLIENT_PLATFORM
#endif

#ifdef SIGNAL_CLIENT_PLATFORM
static pthread_once_t once = PTHREAD_ONCE_INIT;
#endif/*SIGNAL_CLIENT_PLATFORM*/

float __himark = 0;
float __lomark = -1;

struct BROWSER_T
{
  int      id;
  size_t   tid;
  ARRAY    urls;
  ARRAY    parts;
  HASH     cookies;
  CONN *   conn;
#ifdef SIGNAL_CLIENT_PLATFORM
  sigset_t sigs;
#else
  int      type; 
  int      state;
#endif
  float    total;  
  float    available;
  float    lowest;
  float    highest;
  float    elapsed;
  float    time;
  float    himark;
  float    lomark;
  clock_t  start;
  clock_t  stop;
  struct   tms  t_start;
  struct   tms  t_stop;
  struct {
    DCHLG *wchlg;
    DCRED *wcred;
    int    www;
    DCHLG *pchlg;
    DCRED *pcred;
    int  proxy;
    struct {
      int  www;
      int  proxy;
    } bids;
    struct {
      TYPE www;
      TYPE proxy;
    } type;
  } auth;
  unsigned int  code;
  unsigned int  count;
  unsigned int  okay;
  unsigned int  fail;
  unsigned long hits;
  unsigned long long bytes;
  unsigned int  rseed;
};

size_t BROWSERSIZE = sizeof(struct BROWSER_T);

private BOOLEAN __init_connection(BROWSER this, URL U);
private BOOLEAN __request(BROWSER this, URL U); 
private BOOLEAN __http(BROWSER this, URL U);
private BOOLEAN __ftp(BROWSER this, URL U);
private BOOLEAN __no_follow(const char *hostname);
private void    __increment_failures();
private int     __select_color(int code);
private void    __display_result(BROWSER this, RESPONSE resp, URL U, unsigned long bytes, float etime);


#ifdef  SIGNAL_CLIENT_PLATFORM
private void    __signal_handler(int sig);
private void    __signal_init();
#else/*CANCEL_CLIENT_PLATFORM*/
private void    __signal_cleanup();
#endif/*SIGNAL_CLIENT_PLATFORM*/


BROWSER
new_browser(int id)
{
  BROWSER this;

  this = calloc(BROWSERSIZE,1);
  this->id        = id;
  this->total     = 0.0;
  this->available = 0.0;
  this->count     = 0.0;
  this->okay      = 0;
  this->fail      = 0;
  this->lowest    =  -1;
  this->highest   = 0.0;
  this->elapsed   = 0.0;
  this->bytes     = 0.0;
  this->urls      = NULL;
  this->parts     = new_array();
  this->rseed     = urandom();
  return this;
}

BROWSER 
browser_destroy(BROWSER this)
{
  if (this != NULL) {
    /**
     * NOTE: this->urls is a reference to main.c:urls It was
     * never instantiated in this class.  We'll reclaim that 
     * memory when we deconstruct main.c:urls 
     */

    if (this->parts != NULL) {
      URL u;
      while ((u = (URL)array_pop(this->parts)) != NULL) {
        u = url_destroy(u);
      }
      this->parts = array_destroy(this->parts);
    }
    xfree(this);
  }
  this = NULL;
  return this; 
}

unsigned long
browser_get_hits(BROWSER this)
{
  return this->hits;
}

unsigned long long 
browser_get_bytes(BROWSER this)
{
  return this->bytes;
}

float 
browser_get_time(BROWSER this)
{
  return this->time;
}

unsigned int  
browser_get_code(BROWSER this)
{
  return this->code;
}

unsigned int  
browser_get_okay(BROWSER this)
{
  return this->okay;
}

unsigned int  
browser_get_fail(BROWSER this)
{
  return this->fail;
}

float
browser_get_himark(BROWSER this)
{
  return this->himark;
}

float
browser_get_lomark(BROWSER this)
{
  return this->lomark;
}

void *
start(BROWSER this)
{
  int x;
  int y;
  int ret;
  int len; 
  this->conn  = NULL;
  this->conn = xcalloc(sizeof(CONN), 1);
  this->conn->sock       = -1;
  this->conn->page       = new_page("");
  this->conn->cache      = new_cache();

#ifdef SIGNAL_CLIENT_PLATFORM
  pthread_once(&this->once, __signal_init);
  sigemptyset(&this->sigs);
  sigaddset(&this->sigs, SIGUSR1);
  pthread_sigmask(SIG_UNBLOCK, &this->sigs, NULL);
#else/*CANCEL_CLIENT_PLATFORM*/
  #if defined(_AIX)
    pthread_cleanup_push((void(*)(void*))__signal_cleanup, NULL);
  #else
    pthread_cleanup_push((void*)__signal_cleanup, this->conn);
  #endif

  #if defined(sun)
    pthread_setcanceltype (PTHREAD_CANCEL_DEFERRED, &this->type);
  #elif defined(_AIX)
    pthread_setcanceltype (PTHREAD_CANCEL_DEFERRED, &this->type);
  #elif defined(hpux) || defined(__hpux)
    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, &this->type);
  #else
    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, &this->type);
  #endif
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &this->state);
#endif/*SIGNAL_CLIENT_PLATFORM*/

  if (my.login == TRUE) {
    URL tmp = new_url(array_next(my.lurl));
    url_set_ID(tmp, 0);
    __request(this, tmp);
  }

  len = (my.reps == -1) ? (int)array_length(this->urls) : my.reps;
  y   = (my.reps == -1) ? 0 : this->id * (my.length / my.cusers);
  for (x = 0; x < len; x++, y++) {
    x = ((my.secs > 0) && ((my.reps <= 0)||(my.reps == MAXREPS))) ? 0 : x;
    if (my.internet == TRUE) {
      y = (unsigned int) (((double)pthread_rand_np(&(this->rseed)) /
                          ((double)RAND_MAX + 1) * my.length ) + .5);
      y = (y >= my.length)?my.length-1:y;
      y = (y < 0)?0:y;
    } else {
      /**
       * URLs accessed sequentially; when reaching the end, start over
       * with clean slate, ie. reset (delete) cookies (eg. to let a new
       * session start)
       */
      if (y >= my.length) {
        y = 0;
        if (my.expire) {
          cookies_delete_all(my.cookies);
        }
      }
    }
    if (y >= my.length || y < 0) {
      y = 0;
    }

    /**
     * This is the initial request from the command line
     * or urls.txt file. If it is text/html then it will
     * be parsed in __http request function.
     */
    URL tmp = array_get(this->urls, y);
    if (tmp != NULL && url_get_hostname(tmp) != NULL) {
      this->auth.bids.www = 0; /* reset */
      if ((ret = __request(this, tmp))==FALSE) {
        __increment_failures();
      }
    }

    /**
     * If we parsed http resources, we'll request them here
     */
    if (my.parser == TRUE && this->parts != NULL) {
      URL  u;
      while ((u = (URL)array_pop(this->parts)) != NULL) {
        if (url_get_scheme(u) == UNSUPPORTED) {
          ;;
        } else if (my.cache && is_cached(this->conn->cache, u)) {
          RESPONSE r = new_response();
          response_set_code(r, "HTTP/1.1 200 OK");
          response_set_from_cache(r, TRUE);
          __display_result(this, r, u, 0, 0.00);
          r = response_destroy(r);
          ;;
        } else {
          this->auth.bids.www = 0;
          // We'll only request files on the same host as the page
          if (! __no_follow(url_get_hostname(u))) {
            if ((ret = __request(this, u))==FALSE) {
              __increment_failures();
            }
          }
        }
        u = url_destroy(u);
      }
    }

    /**
     * Delay between interactions -D num /--delay=num
     */
    if (my.delay >= 1) {
      pthread_sleep_np(
       (unsigned int) (((double)pthread_rand_np(&(this->rseed)) /
                       ((double)RAND_MAX + 1) * my.delay ) + .5)
      );
    } else if (my.delay >= .001) {
      pthread_usleep_np(
       (unsigned int) (((double)pthread_rand_np(&(this->rseed)) /
                       ((double)RAND_MAX + 1) * my.delay * 1000000 ) + .0005)
      );
    }

    if (my.failures > 0 && my.failed >= my.failures) {
      break;
    }
  }

#ifdef SIGNAL_CLIENT_PLATFORM
#else/*CANCEL_CLIENT_PLATFORM*/
  // XXX: every cleanup must have a pop
  pthread_cleanup_pop(0);
#endif/*SIGNAL_CLIENT_PLATFORM*/

  if (this->conn->sock >= 0){
    this->conn->connection.reuse = 0;
    socket_close(this->conn);
  }
  this->conn->page  = page_destroy(this->conn->page);
  this->conn->cache = cache_destroy(this->conn->cache); //XXX: do we want to persist this?
  xfree(this->conn);
  this->conn = NULL;

  return NULL;
}

void
browser_set_urls(BROWSER this, ARRAY urls)
{
  this->urls = urls;
}

void
browser_set_cookies(BROWSER this, HASH cookies)
{
  int i = 0;

  this->cookies = cookies;

  if (this->cookies != NULL) {
    char **keys = hash_get_keys(this->cookies);
    for (i = 0; i < hash_get_entries(this->cookies); i ++){
      /**
       * We need a local copy of the variable to pass to cookies_add
       */
      char *tmp;
      int   len = strlen(hash_get(this->cookies, keys[i]));
      tmp = xmalloc(len+2);
      memset(tmp, '\0', len+2);
      snprintf(tmp, len+1, "%s", (char*)hash_get(this->cookies, keys[i]));
      cookies_add(my.cookies, tmp, ".");
      xfree(tmp);
    }
  }
}

private BOOLEAN
__request(BROWSER this, URL U) {
  this->conn->scheme = url_get_scheme(U);

  switch (this->conn->scheme) {
    case FTP:
      return __ftp(this, U);
    case HTTP:
    case HTTPS:
    default:
      return __http(this, U);
  }
}

/**
 * HTTP client request.
 * The protocol is executed in http.c
 * This function invoked functions inside that module
 * and it gathers statistics about the request.
 */
private BOOLEAN
__http(BROWSER this, URL U)
{
  unsigned long bytes  = 0;
  int      code, okay, fail;
  float    etime;
  clock_t  start, stop;
  struct   tms t_start, t_stop;
  RESPONSE resp;
  char     *meta = NULL;
#ifdef  HAVE_LOCALTIME_R
  struct   tm keepsake;
#endif/*HAVE_LOCALTIME_R*/
  time_t   now;
  struct   tm *tmp;
  size_t   len;
  char     fmtime[65];
  URL      redirect_url = NULL;

  page_clear(this->conn->page);

  if (my.csv) {
    now = time(NULL);
    #ifdef HAVE_LOCALTIME_R
    tmp = (struct tm *)localtime_r(&now, &keepsake);
    #else
    tmp = localtime(&now);
    #endif/*HAVE_LOCALTIME_R*/
    if (tmp) {
      len = strftime(fmtime, 64, "%Y-%m-%d %H:%M:%S", tmp);
      if (len == 0) {
        memset(fmtime, '\0', 64);
        snprintf(fmtime, 64, "n/a");
      }
    } else {
      snprintf(fmtime, 64, "n/a");
    }
  }

  if (url_get_scheme(U) == UNSUPPORTED) {
    if (my.verbose && !my.get && !my.print) {
      NOTIFY (
        ERROR,
        "%s %d %6.2f secs: %7d bytes ==> %s\n",
        "UNSPPRTD", 501, 0.00, 0, "PROTOCOL NOT SUPPORTED BY SIEGE"
      );
    } /* end if my.verbose */
    return FALSE;
  }

  /* record transaction start time */
  start = times(&t_start);
  if (! __init_connection(this, U)) return FALSE;

  /**
   * write to socket with a GET/POST/PUT/DELETE/HEAD
   */
  if (url_get_method(U) == POST || url_get_method(U) == PUT || url_get_method(U) == PATCH) {
    if ((http_post(this->conn, U)) == FALSE) {
      this->conn->connection.reuse = 0;
      socket_close(this->conn);
      return FALSE;
    }
  } else {
    if ((http_get(this->conn, U)) == FALSE) {
      this->conn->connection.reuse = 0;
      socket_close(this->conn);
      return FALSE;
    }
  }

  /**
   * read from socket and collect statistics.
   */
  if ((resp = http_read_headers(this->conn, U))==NULL) {
    this->conn->connection.reuse = 0;
    socket_close(this->conn);
    echo ("%s:%d NULL headers", __FILE__, __LINE__);
    return FALSE;
  }

  code = response_get_code(resp);

  if (code == 418) {
    /**
     * I don't know what server we're talking to but I 
     * know what it's not. It's not an HTTP server....
     */
    this->conn->connection.reuse = 0;
    socket_close(this->conn);
    stop  =  times(&t_stop);
    etime =  elapsed_time(stop - start);
    this->hits ++;
    this->time += etime;
    this->fail += 1;

    __display_result(this, resp, U, 0, etime);
    resp = response_destroy(resp);
    return FALSE;
  }

  bytes = http_read(this->conn, resp);
  if (my.print) {
    printf("%s\n", page_value(this->conn->page));
  }

  if (my.parser == TRUE) {
    if (strmatch(response_get_content_type(resp), "text/html") && code < 300) {
      int   i;
      html_parser(this->parts, U, page_value(this->conn->page));
      for (i = 0; i < (int)array_length(this->parts); i++) {
        URL url  = (URL)array_get(this->parts, i);
        if (url_is_redirect(url)) {
          URL tmp = (URL)array_remove(this->parts, i);
          meta    = xstrdup(url_get_absolute(tmp));
          tmp     = url_destroy(tmp);
        }
      }
    }
  }

  if (!my.zero_ok && (bytes < 1)) {
    this->conn->connection.reuse = 0;
    socket_close(this->conn);
    resp = response_destroy(resp);
    echo ("%s:%d zero bytes back from server", __FILE__, __LINE__);
    return FALSE;
  }
  stop     =  times(&t_stop);
  etime    =  elapsed_time(stop - start);
  okay     =  response_success(resp);
  fail     =  response_failure(resp);
  /**
   * quantify the statistics for this client.
   */
  this->bytes += bytes;
  this->time  += etime;
  this->code  += okay;
  this->fail  += fail;
  if (code == 200) {
    this->okay++;
  }
 
  /**
   * check to see if this transaction is the longest or shortest
   */
  if (etime > __himark) {
    __himark = etime;
  }
  if ((__lomark < 0) || (etime < __lomark)) {
    __lomark = etime;
  }
  this->himark = __himark;
  this->lomark = __lomark;

  /**
   * verbose output, print statistics to stdout
   */
  __display_result(this, resp, U, bytes, etime);

  /**
   * close the socket and free memory.
   */
  if (!my.keepalive) {
    socket_close(this->conn);
  }

  switch (code) {
    case 200:
      if (meta != NULL && strlen(meta) > 2) {
        /**
         * <meta http-equiv="refresh" content="0; url=https://www.joedog.org/haha.html" />
         */
        redirect_url = url_normalize(U, meta);
        xfree(meta);
        meta = NULL;
        page_clear(this->conn->page);
        if (empty(url_get_hostname(redirect_url))) {
          url_set_hostname(redirect_url, url_get_hostname(U));
        }
        url_set_redirect(U, FALSE);
        url_set_redirect(redirect_url, FALSE);
        if ((__request(this, redirect_url)) == FALSE) {
          redirect_url = url_destroy(redirect_url);
          return FALSE;
        }
        redirect_url = url_destroy(redirect_url);
      }
      break;
    case 301:
    case 302:
    case 303:
    case 307:
      if (my.follow && response_get_location(resp) != NULL) {
        /**
         * XXX: What if the server sends us
         * Location: path/file.htm
         *  OR
         * Location: /path/file.htm
         */
        redirect_url = url_normalize(U, response_get_location(resp));

        if (empty(url_get_hostname(redirect_url))) {
          url_set_hostname(redirect_url, url_get_hostname(U));
        }
        if (code == 307) {
          url_set_conttype(redirect_url,url_get_conttype(U));
          url_set_method(redirect_url, url_get_method(U));

          if (url_get_method(redirect_url) == POST || url_get_method(redirect_url) == PUT || url_get_method(redirect_url) == PATCH) {
            url_set_postdata(redirect_url, url_get_postdata(U), url_get_postlen(U));
          }
        }
        if ((__request(this, redirect_url)) == FALSE) {
          redirect_url = url_destroy(redirect_url);
          return FALSE;
        }
      }
      redirect_url = url_destroy(redirect_url);
      break;
    case 401:
      /**
       * WWW-Authenticate challenge from the WWW server
       */
      this->auth.www = (this->auth.www==0) ? 1 : this->auth.www;
      if ((this->auth.bids.www++) < my.bids - 1) {
        BOOLEAN b;
        if (response_get_www_auth_type(resp) == DIGEST) {
          this->auth.type.www = DIGEST;
          b = auth_set_digest_header(
            my.auth, &(this->auth.wchlg), &(this->auth.wcred), &(this->rseed),
            response_get_www_auth_realm(resp), response_get_www_auth_challenge(resp)
          );
          if (b == FALSE) {
            fprintf(stderr, "ERROR: Unable to respond to an authorization challenge\n");
            fprintf(stderr, "       in the following realm: '%s'\n", response_get_www_auth_realm(resp));
            fprintf(stderr, "       Did you set login credentials in the conf file?\n");
            resp = response_destroy(resp);
            return FALSE;
          }
        }
        if (response_get_www_auth_type(resp) == NTLM) {
          this->auth.type.www =  NTLM;
          b = auth_set_ntlm_header (
            my.auth, HTTP, response_get_www_auth_challenge(resp), response_get_www_auth_realm(resp)
          );
        }
        if (response_get_www_auth_type(resp) == BASIC) {
          this->auth.type.www =  BASIC;
          auth_set_basic_header(my.auth, HTTP, response_get_www_auth_realm(resp));
        }
        if ((__request(this, U)) == FALSE) {
          fprintf(stderr, "ERROR from http_request\n");
          return FALSE;
        }
      }
      break;
    case 407:
      /**
       * Proxy-Authenticate challenge from the proxy server.
       */
      this->auth.proxy = (this->auth.proxy==0) ? 1 : this->auth.proxy;
      if ((this->auth.bids.proxy++) < my.bids - 1) {
        if (response_get_proxy_auth_type(resp) == DIGEST) {
          BOOLEAN b;
          this->auth.type.proxy =  DIGEST;
          b = auth_set_digest_header (
            my.auth, &(this->auth.pchlg), &(this->auth.pcred), &(this->rseed),
            response_get_proxy_auth_realm(resp), response_get_proxy_auth_challenge(resp)
          );
          if (b == FALSE) {
            fprintf(stderr, "ERROR: Unable to respond to a proxy authorization challenge\n");
            fprintf(stderr, "       in the following HTTP realm: '%s'\n", response_get_proxy_auth_realm(resp));
            fprintf(stderr, "       Did you set proxy-login credentials in the conf file?\n");
            resp = response_destroy(resp);
            return FALSE;
          }
        }
        if (response_get_proxy_auth_type(resp) == BASIC) {
          this->auth.type.proxy = BASIC;
          auth_set_basic_header(my.auth, PROXY, response_get_proxy_auth_realm(resp));
        }
        if ((__request(this, U)) == FALSE)
          return FALSE;
      }
      break;
    case 408:
    case 500:
    case 501:
    case 502:
    case 503:
    case 504:
    case 505:
    case 506:
    case 507:
    case 508:
    case 509:
      return FALSE;
    default:
      break;
  }

  this->hits++;
  resp = response_destroy(resp);

  return TRUE;
}

/**
 * HTTP client request.
 * The protocol is executed in http.c
 * This function invoked functions inside that module
 * and it gathers statistics about the request.
 */
private BOOLEAN
__ftp(BROWSER this, URL U)
{
  int     pass;
  int     fail;
  int     code = 0;      // capture the relevent return code
  float   etime;         // elapsed time
  CONN    *D    = NULL;  // FTP data connection
  size_t  bytes = 0;     // bytes from server
  clock_t start, stop;
  struct  tms t_start, t_stop;

  D = xcalloc(sizeof(CONN), 1);
  D->sock = -1;

  if (! __init_connection(this, U)) {
    NOTIFY (
      ERROR, "%s:%d connection failed %s:%d",
      __FILE__, __LINE__, url_get_hostname(U), url_get_port(U)
    );
    xfree(D);
    return FALSE;
  }

  start = times(&t_start);
  if (this->conn->sock < 0) {
    NOTIFY (
      ERROR, "%s:%d connection failed %s:%d",
      __FILE__, __LINE__, url_get_hostname(U), url_get_port(U)
    );
    socket_close(this->conn);
    xfree(D);
    return FALSE;
  }

  if (url_get_username(U) == NULL || strlen(url_get_username(U)) < 1) {
    url_set_username(U, auth_get_ftp_username(my.auth, url_get_hostname(U)));
  }

  if (url_get_password(U) == NULL || strlen(url_get_password(U)) < 1) {
    url_set_password(U, auth_get_ftp_password(my.auth, url_get_hostname(U)));
  }

  if (ftp_login(this->conn, U) == FALSE) {
    if (my.verbose) {
      int  color = __select_color(this->conn->ftp.code);
      DISPLAY (
        color, "FTP/%d %6.2f secs: %7lu bytes ==> %-6s %s",
        this->conn->ftp.code, 0.0, bytes, url_get_method_name(U), url_get_request(U)
      );
    }
    xfree(D);
    this->fail += 1;
    return FALSE;
  }

  ftp_pasv(this->conn);
  if (this->conn->ftp.pasv == TRUE) {
    debug("Connecting to: %s:%d", this->conn->ftp.host, this->conn->ftp.port);
    D->sock = new_socket(D, this->conn->ftp.host, this->conn->ftp.port);
    if (D->sock < 0) {
      debug (
        "%s:%d connection failed. error %d(%s)",__FILE__, __LINE__, errno,strerror(errno)
      );
      this->fail += 1;
      socket_close(D);
      xfree(D);
      return FALSE;
    }
  }
  if (url_get_method(U) == POST || url_get_method(U) == PUT || url_get_method(U) == PATCH) {
    ftp_stor(this->conn, U);
    bytes = ftp_put(D, U);
    code  = this->conn->ftp.code;
  } else {
    if (ftp_size(this->conn, U) == TRUE) {
      if (ftp_retr(this->conn, U) == TRUE) {
        bytes = ftp_get(D, U, this->conn->ftp.size);
      }
    }
    code = this->conn->ftp.code;
  }
  socket_close(D);
  ftp_quit(this->conn);

  pass  = (bytes == this->conn->ftp.size) ? 1 : 0;
  fail  = (pass  == 0) ? 1 : 0;
  stop  =  times(&t_stop);
  etime =  elapsed_time(stop - start);
  this->bytes += bytes;
  this->time  += etime;
  this->code  += pass;
  this->fail  += fail;

  /**
   * check to see if this transaction is the longest or shortest
   */
  if (etime > __himark) {
    __himark = etime;
  }
  if ((__lomark < 0) || (etime < __lomark)) {
    __lomark = etime;
  }
  this->himark = __himark;
  this->lomark = __lomark;

  if (my.verbose) {
    int  color = (my.color == TRUE) ? __select_color(code) : -1;
    DISPLAY (
      color, "FTP/%d %6.2f secs: %7lu bytes ==> %-6s %s",
      code, etime, bytes, url_get_method_name(U), url_get_request(U)
    );
  }
  this->hits++;
  xfree(D);
  return TRUE;
}

private BOOLEAN
__init_connection(BROWSER this, URL U)
{
  this->conn->pos_ini              = 0;
  this->conn->inbuffer             = 0;
  this->conn->content.transfer     = NONE;
  this->conn->content.length       = (size_t)~0L;// VL - issue #2, 0 is a legit.value
  this->conn->connection.keepalive = (this->conn->connection.max==1)?0:my.keepalive;
  this->conn->connection.reuse     = (this->conn->connection.max==1)?0:my.keepalive;
  this->conn->connection.tested    = (this->conn->connection.tested==0)?1:this->conn->connection.tested;
  this->conn->auth.www             = this->auth.www;
  this->conn->auth.wchlg           = this->auth.wchlg;
  this->conn->auth.wcred           = this->auth.wcred;
  this->conn->auth.proxy           = this->auth.proxy;
  this->conn->auth.pchlg           = this->auth.pchlg;
  this->conn->auth.pcred           = this->auth.pcred;
  this->conn->auth.type.www        = this->auth.type.www;
  this->conn->auth.type.proxy      = this->auth.type.proxy;
  memset(this->conn->buffer, 0, sizeof(this->conn->buffer));

  debug (
    "%s:%d attempting connection to %s:%d",
    __FILE__, __LINE__,
    (auth_get_proxy_required(my.auth))?auth_get_proxy_host(my.auth):url_get_hostname(U),
    (auth_get_proxy_required(my.auth))?auth_get_proxy_port(my.auth):url_get_port(U)
  );

  if (!this->conn->connection.reuse || this->conn->connection.status == 0) {
    if (auth_get_proxy_required(my.auth)) {
      debug (
        "%s:%d creating new socket:     %s:%d",
        __FILE__, __LINE__, auth_get_proxy_host(my.auth), auth_get_proxy_port(my.auth)
      );
      this->conn->sock = new_socket(this->conn, auth_get_proxy_host(my.auth), auth_get_proxy_port(my.auth));
    } else {
      debug (
        "%s:%d creating new socket:     %s:%d",
        __FILE__, __LINE__, url_get_hostname(U), url_get_port(U)
      );
      this->conn->sock = new_socket(this->conn, url_get_hostname(U), url_get_port(U));
    }
  }

  if (my.keepalive) {
    this->conn->connection.reuse = TRUE;
  }

  if (this->conn->sock < 0) {
    debug (
      "%s:%d connection failed. error %d(%s)",__FILE__, __LINE__, errno,strerror(errno)
    );
    socket_close(this->conn);
    return FALSE;
  }

  debug (
    "%s:%d good socket connection:  %s:%d",
    __FILE__, __LINE__,
    (auth_get_proxy_required(my.auth))?auth_get_proxy_host(my.auth):url_get_hostname(U),
    (auth_get_proxy_required(my.auth))?auth_get_proxy_port(my.auth):url_get_port(U)
  );

  if (url_get_scheme(U) == HTTPS) {
    if (auth_get_proxy_required(my.auth)) {
      https_tunnel_request(this->conn, url_get_hostname(U), url_get_port(U));
      https_tunnel_response(this->conn);
    }
    this->conn->encrypt = TRUE;
    if (SSL_initialize(this->conn, url_get_hostname(U))==FALSE) {
      return FALSE;
    }
  }
  return TRUE;
}

private void
__display_result(BROWSER this, RESPONSE resp, URL U, unsigned long bytes, float etime)
{
  char   fmtime[65];
  #ifdef  HAVE_LOCALTIME_R
  struct tm keepsake;
  #endif/*HAVE_LOCALTIME_R*/
  time_t now;
  struct tm * tmp;
  size_t len;


  if (my.csv) {
    now = time(NULL);
    #ifdef HAVE_LOCALTIME_R
    tmp = (struct tm *)localtime_r(&now, &keepsake);
    #else
    tmp = localtime(&now);
    #endif/*HAVE_LOCALTIME_R*/
    if (tmp) {
      len = strftime(fmtime, 64, "%Y-%m-%d %H:%M:%S", tmp);
      if (len == 0) {
        memset(fmtime, '\0', 64);
        snprintf(fmtime, 64, "n/a");
      }
    } else {
      snprintf(fmtime, 64, "n/a");
    }
  }

  /**
   * verbose output, print statistics to stdout
   */
  if ((my.verbose && !my.get && !my.print) && (!my.debug)) {
    int  color   = (my.color == TRUE) ? __select_color(response_get_code(resp)) : -1;
    DATE date    = new_date(NULL);
    char *stamp  = (my.timestamp)?date_stamp(date):"";
    char *cached = response_get_from_cache(resp) ? "(C)":"   ";
    if (my.color && response_get_from_cache(resp) == TRUE) {
      color = GREEN;
    }

    if (my.csv) {
      if (my.display)
        DISPLAY(color, "%s%s%s%4d,%s,%d,%6.2f,%7lu,%s,%d,%s",
        stamp, (my.mark)?my.markstr:"", (my.mark)?",":"", this->id, response_get_protocol(resp),
        response_get_code(resp), etime, bytes, url_get_display(U), url_get_ID(U), fmtime
      );
      else
        DISPLAY(color, "%s%s%s%s,%d,%6.2f,%7lu,%s,%d,%s",
          stamp, (my.mark)?my.markstr:"", (my.mark)?",":"", response_get_protocol(resp),
          response_get_code(resp), etime, bytes, url_get_display(U), url_get_ID(U), fmtime
        );
    } else {
      if (my.display)
        DISPLAY(
          color, "%4d) %s %d %6.2f secs: %7lu bytes ==> %-4s %s",
          this->id, response_get_protocol(resp), response_get_code(resp),
          etime, bytes, url_get_method_name(U), url_get_display(U)
        );
      else
        DISPLAY (
          color, "%s%s %d%s %5.2f secs: %7lu bytes ==> %-4s %s",
          stamp, response_get_protocol(resp), response_get_code(resp), cached,
          etime, bytes, url_get_method_name(U), url_get_display(U)
        );
    } /* else not my.csv */
    date = date_destroy(date);
  }
  return;
}

private void
__increment_failures()
{
  pthread_mutex_lock(&(my.lock));
  my.failed++;
  pthread_mutex_unlock(&(my.lock));
  pthread_testcancel();
}

private BOOLEAN
__no_follow(const char *hostname)
{
  int i;

  for (i = 0; i < my.nomap->index; i++) {
    if (stristr(hostname, my.nomap->line[i]) != NULL) {
      return TRUE;
    } 
  }
  return FALSE;
}


private int
__select_color(int code)
{
  switch(code) {
    case 150:
    case 200:
    case 201:
    case 202:
    case 203:
    case 204:
    case 205:
    case 206:
    case 226:
      return BLUE;
    case 300:
    case 301:
    case 302:
    case 303:
    case 304:
    case 305:
    case 306:
    case 307:
      return CYAN;
    case 400:
    case 401:
    case 402:
    case 403:
    case 404:
    case 405:
    case 406:
    case 407:
    case 408:
    case 409:
    case 410:
    case 411:
    case 412:
    case 413:
    case 414:
    case 415:
    case 416:
    case 417:
      return MAGENTA;
    case 500:
    case 501:
    case 502:
    case 503:
    case 504:
    case 505:
    default: // WTF?
      return RED;
  }
  return RED;
}

#ifdef SIGNAL_CLIENT_PLATFORM
private void
__signal_handler(int sig)
{
  pthread_exit(&sig);
}

private void
__signal_init()
{
  struct sigaction sa;

  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);
}
#else/*CANCEL_CLIENT_PLATFORM*/
private void
__signal_cleanup()
{
  return;
}
#endif
