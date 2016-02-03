/**
 * HTTP/HTTPS client support
 *
 * Copyright (C) 2000-2014 by
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
 * 
 */
#include <setup.h>
#include <client.h>
#include <signal.h>
#include <sock.h>
#include <ssl.h>
#include <ftp.h>
#include <http.h>
#include <url.h>
#include <util.h>
#include <auth.h>
#include <cookies.h>
#include <date.h>
#include <joedog/boolean.h>
#include <joedog/defs.h>

#if defined(hpux) || defined(__hpux) || defined(WINDOWS)
#define SIGNAL_CLIENT_PLATFORM
#endif

/**
 * local prototypes
 */
private BOOLEAN __request(CONN *C, URL U, CLIENT *c);
private BOOLEAN __http(CONN *C, URL U, CLIENT *c);
private BOOLEAN __ftp (CONN *C, URL U, CLIENT *c);
private BOOLEAN __init_connection(CONN *C, URL U, CLIENT *c);
private void    __increment_failures();
private int     __select_color(int code);

#ifdef  SIGNAL_CLIENT_PLATFORM
static void signal_handler( int i );
static void signal_init();
#else /*CANCEL_CLIENT_PLATFORM*/
void clean_up();
#endif/*SIGNAL_CLIENT_PLATFORM*/
 
/**
 * local variables
 */
#ifdef SIGNAL_CLIENT_PLATFORM
static pthread_once_t once = PTHREAD_ONCE_INIT;
#endif/*SIGNAL_CLIENT_PLATFORM*/
float himark = 0;
float lomark = -1;  

/**
 * The thread entry point for clients.
 *
 * #ifdef SIGNAL_CLIENT_PLATFORM
 * the thread entry point for signal friendly clients.
 * (currently only HP-UX and Windows)
 *
 * #ifndef SIGNAL_CLIENT_PLATFORM
 * assume cancel client.
 * thread entry point for cancellable friendly operating systems like
 * aix, GNU/linux, solaris, bsd, etc.
 */
void *
start_routine(CLIENT *client)
{
  int i;
  int       x, y;    // loop counters, indices 
  int       ret;     //function return value  
  int       len;     // main loop length 
  CONN *C = NULL;    // connection data (sock.h)  

#ifdef SIGNAL_CLIENT_PLATFORM
  sigset_t  sigs;
#else
  int type, state;
#endif 

  C = xcalloc(sizeof(CONN), 1);
  C->sock = -1;

  if (client->cookies != NULL) {
    char **keys = hash_get_keys(client->cookies);
    for (i = 0; i < hash_get_entries(client->cookies); i ++){
      /** 
       * We need a local copy of the variable to pass to cookies_add
       */
      char *tmp;
      int   len = strlen(hash_get(client->cookies, keys[i]));
      tmp = xmalloc(len+2);
      memset(tmp, '\0', len+2);
      snprintf(tmp, len+1, "%s", hash_get(client->cookies, keys[i]));
      cookies_add(my.cookies, tmp, ".");
      xfree(tmp);
    }
  }

#ifdef SIGNAL_CLIENT_PLATFORM
  pthread_once(&once, signal_init);
  sigemptyset(&sigs);
  sigaddset(&sigs, SIGUSR1);
  pthread_sigmask(SIG_UNBLOCK, &sigs, NULL);
#else
  #if defined(_AIX)
    pthread_cleanup_push((void(*)(void*))clean_up, NULL);
  #else
    pthread_cleanup_push((void*)clean_up, C);
  #endif
#endif /*SIGNAL_CLIENT_PLATFORM*/

#ifdef SIGNAL_CLIENT_PLATFORM
#else/*CANCEL_CLIENT_PLATFORM*/
  #if defined(sun)
    pthread_setcanceltype (PTHREAD_CANCEL_DEFERRED, &type);
  #elif defined(_AIX)
    pthread_setcanceltype (PTHREAD_CANCEL_DEFERRED, &type);
  #elif defined(hpux) || defined(__hpux)
    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, &type);
  #else
    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, &type);
  #endif
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &state);
#endif/*SIGNAL_CLIENT_PLATFORM*/ 
  if (my.login == TRUE) {
    URL tmp = new_url(array_next(my.lurl));
    url_set_ID(tmp, 0);
    __request(C, tmp, client);
  }

  len = (my.reps == -1) ? (int)array_length(client->urls) : my.reps;
  y   = (my.reps == -1) ? 0 : client->id * (my.length / my.cusers);
  for (x = 0; x < len; x++, y++) {
    x = ((my.secs > 0) && ((my.reps <= 0)||(my.reps == MAXREPS))) ? 0 : x;
    if (my.internet == TRUE) {
      y = (unsigned int) (((double)pthread_rand_np(&(client->rand_r_SEED)) /
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
    URL tmp = array_get(client->urls, y);
    if (tmp != NULL && url_get_hostname(tmp) != NULL) {
      client->auth.bids.www = 0; /* reset */
      if ((ret = __request(C, tmp, client))==FALSE) {
        __increment_failures();
      }
    }
 
    if (my.failures > 0 && my.failed >= my.failures) {
      break;
    }
  }

  #ifdef SIGNAL_CLIENT_PLATFORM
  #else/*CANCEL_CLIENT_PLATFORM*/
  /**
   * every cleanup must have a pop
   */
  pthread_cleanup_pop(0);
  #endif/*SIGNAL_CLIENT_PLATFORM*/ 
  if (C->sock >= 0){
    C->connection.reuse = 0;    
    socket_close(C);
  }
  xfree(C);
  C = NULL;

  return(NULL);
}

private BOOLEAN
__request(CONN *C, URL U, CLIENT *client) {
  C->scheme = url_get_scheme(U);
 
  switch (C->scheme) {
    case FTP:
      return __ftp(C, U, client);
    case HTTP:
    case HTTPS:
    default:
      return __http(C, U, client);
  }
}

/**
 * HTTP client request.
 * The protocol is executed in http.c 
 * This function invoked functions inside that module
 * and it gathers statistics about the request. 
 */
private BOOLEAN
__http(CONN *C, URL U, CLIENT *client)
{
  unsigned long bytes  = 0;
  int      code, fail;  
  float    etime; 
  clock_t  start, stop;
  struct   tms t_start, t_stop; 
  HEADERS  *head; 
#ifdef  HAVE_LOCALTIME_R
  struct   tm keepsake;
#endif/*HAVE_LOCALTIME_R*/
  time_t   now; 
  struct   tm *tmp;
  size_t   len;
  char     fmtime[65];
  URL  redirect_url = NULL; 
 
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
    if (my.verbose && !my.get) {
      NOTIFY ( 
        ERROR,
        "%s %d %6.2f secs: %7d bytes ==> %s\n",
        "UNSPPRTD", 501, 0.00, 0, "PROTOCOL NOT SUPPORTED BY SIEGE" 
      );
    } /* end if my.verbose */
    return FALSE;
  }

  if (my.delay >= 1) {
    pthread_sleep_np(
     (unsigned int) (((double)pthread_rand_np(&(client->rand_r_SEED)) /
                     ((double)RAND_MAX + 1) * my.delay ) + .5) 
    );
  } else if (my.delay >= .001) {
    pthread_usleep_np(
     (unsigned int) (((double)pthread_rand_np(&(client->rand_r_SEED)) /
                     ((double)RAND_MAX + 1) * my.delay * 1000000 ) + .0005) 
    );
  }

  /* record transaction start time */
  start = times(&t_start);  

  if (! __init_connection(C, U, client)) return FALSE;
  
  /**
   * write to socket with a GET/POST/PUT/DELETE/HEAD
   */
  if (url_get_method(U) == POST) { 
    if ((http_post(C, U)) == FALSE) {
      C->connection.reuse = 0;
      socket_close(C);
      return FALSE;
    }
  } else { 
    if ((http_get(C, U)) == FALSE) {
      C->connection.reuse = 0;
      socket_close(C);
      return FALSE;
    }
  } 

  /**
   * read from socket and collect statistics.
   */
  if ((head = http_read_headers(C, U))==NULL) {
    C->connection.reuse = 0; 
    socket_close(C); 
    echo ("%s:%d NULL headers", __FILE__, __LINE__);
    return FALSE; 
  } 

  bytes = http_read(C); 

  if (!my.zero_ok && (bytes < 1)) { 
    C->connection.reuse = 0; 
    socket_close(C); 
    http_free_headers(head); 
    echo ("%s:%d zero bytes back from server", __FILE__, __LINE__);
    return FALSE; 
  } 
  stop     =  times(&t_stop); 
  etime    =  elapsed_time(stop - start);  
  code     =  (head->code <  400 || head->code == 401 || head->code == 407) ? 1 : 0;
  fail     =  (head->code >= 400 && head->code != 401 && head->code != 407) ? 1 : 0; 
  /**
   * quantify the statistics for this client.
   */
  client->bytes += bytes;
  client->time  += etime;
  client->code  += code;
  client->fail  += fail;
  if (head->code == 200) {
    client->ok200++;
  }

  /**
   * check to see if this transaction is the longest or shortest
   */
  if (etime > himark) {
    himark = etime;
  }
  if ((lomark < 0) || (etime < lomark)) {
    lomark = etime;
  }
  client->himark = himark;
  client->lomark = lomark;

  /**
   * verbose output, print statistics to stdout
   */
  if ((my.verbose && !my.get) && (!my.debug)) {
    int  color     = __select_color(head->code);
    char *time_str = (my.timestamp==TRUE)?timestamp():"";
    if (my.csv) {
      if (my.display)
        DISPLAY(color, "%s%s%s%4d,%s,%d,%6.2f,%7lu,%s,%d,%s",
        time_str, (my.mark)?my.markstr:"", (my.mark)?",":"", client->id, head->head, head->code, 
        etime, bytes, url_get_display(U), url_get_ID(U), fmtime
      );
      else
        DISPLAY(color, "%s%s%s%s,%d,%6.2f,%7lu,%s,%d,%s",
          time_str, (my.mark)?my.markstr:"", (my.mark)?",":"", head->head, head->code, 
          etime, bytes, url_get_display(U), url_get_ID(U), fmtime
        );
    } else {
      if (my.display)
        DISPLAY(
          color, "%4d) %s %d %6.2f secs: %7lu bytes ==> %-4s %s", 
          client->id, head->head, head->code, etime, bytes, url_get_method_name(U), url_get_display(U)
        ); 
      else
        DISPLAY ( 
          color, "%s%s %d %6.2f secs: %7lu bytes ==> %-4s %s", 
          time_str, head->head, head->code, etime, bytes, url_get_method_name(U), 
		  url_get_display(U)
        );
    } /* else not my.csv */
    if (my.timestamp) xfree(time_str);
  }

  /**
   * close the socket and free memory.
   */
  if (!my.keepalive) {
    socket_close(C);
  }
 
  /**
   * deal with HTTP > 300 
   */
  switch (head->code) {
    case 301:
    case 302:
    case 303:
    case 307:
      if (my.follow && head->redirect[0]) {
        /**  
         * XXX: What if the server sends us
         * Location: path/file.htm 
         *  OR
         * Location: /path/file.htm
         */ 
        redirect_url = url_normalize(U, head->redirect); //new_url(head->redirect);

        if (empty(url_get_hostname(redirect_url))) { 
          url_set_hostname(redirect_url, url_get_hostname(U));
        }
        if (head->code == 307) {
          url_set_conttype(redirect_url,url_get_conttype(U));
          url_set_method(redirect_url, url_get_method(U));

          if (url_get_method(redirect_url) == POST) {
            url_set_postdata(redirect_url, url_get_postdata(U), url_get_postlen(U));
          }
        }
        if ((__request(C, redirect_url, client)) == FALSE) {
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
      client->auth.www = (client->auth.www==0)?1:client->auth.www;
      if ((client->auth.bids.www++) < my.bids - 1) {
        BOOLEAN b;
        if (head->auth.type.www == DIGEST) {
          client->auth.type.www = DIGEST;
          b = auth_set_digest_header(
            my.auth, &(client->auth.wchlg), &(client->auth.wcred), &(client->rand_r_SEED),
            head->auth.realm.www, head->auth.challenge.www
          );
	  if (b == FALSE) {
	    NOTIFY(ERROR, "unable to set digest header");
	    return FALSE;
	  }
        }
        if (head->auth.type.www == NTLM) {
          client->auth.type.www =  NTLM;
          b = auth_set_ntlm_header(my.auth, HTTP, head->auth.challenge.www, head->auth.realm.www);
        }
        if (head->auth.type.www == BASIC) {
          client->auth.type.www =  BASIC;
          auth_set_basic_header(my.auth, HTTP, head->auth.realm.www);
        }
        if ((__request(C, U, client)) == FALSE) {
          fprintf(stderr, "ERROR from http_request\n");
          return FALSE;
        }
      }
      break;
    case 407:
      /**
       * Proxy-Authenticate challenge from the proxy server.
       */
      client->auth.proxy = (client->auth.proxy==0)?1:client->auth.proxy;
      if ((client->auth.bids.proxy++) < my.bids - 1) {
        if (head->auth.type.proxy == DIGEST) {
          BOOLEAN b;
          client->auth.type.proxy =  DIGEST;
          b = auth_set_digest_header (
            my.auth, &(client->auth.pchlg), &(client->auth.pcred), &(client->rand_r_SEED),
            head->auth.realm.proxy, head->auth.challenge.proxy
          );
	  if (b == FALSE) {
            fprintf(stderr, "ERROR: Unable to respond to a proxy authorization challenge\n");
            fprintf(stderr, "       in the following HTTP realm: '%s'\n", head->auth.realm.proxy);
            fprintf(stderr, "       Did you set proxy-login credentials in the conf file?\n");
	    return FALSE;
	  }
        }
        if (head->auth.type.proxy == BASIC) {
          client->auth.type.proxy = BASIC;
          auth_set_basic_header(my.auth, PROXY, head->auth.realm.proxy);
        }
        if ((__request(C, U, client)) == FALSE)
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

  client->hits  ++; 
  http_free_headers(head);

  return TRUE;
}

/**
 * HTTP client request.
 * The protocol is executed in http.c 
 * This function invoked functions inside that module
 * and it gathers statistics about the request. 
 */
private BOOLEAN
__ftp(CONN *C, URL U, CLIENT *client)
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

  if (! __init_connection(C, U, client)) { 
    NOTIFY (
      ERROR, "%s:%d connection failed %s:%d",
      __FILE__, __LINE__, url_get_hostname(U), url_get_port(U)
    );
    xfree(D);
    return FALSE;
  }

  start = times(&t_start);
  if (C->sock < 0) {
    NOTIFY (
      ERROR, "%s:%d connection failed %s:%d",
      __FILE__, __LINE__, url_get_hostname(U), url_get_port(U)
    );
    socket_close(C);
    xfree(D);
    return FALSE;
  } 

  if (url_get_username(U) == NULL || strlen(url_get_username(U)) < 1) {
    url_set_username(U, auth_get_ftp_username(my.auth, url_get_hostname(U)));
  }

  if (url_get_password(U) == NULL || strlen(url_get_password(U)) < 1) {
    url_set_password(U, auth_get_ftp_password(my.auth, url_get_hostname(U)));
  }

  if (ftp_login(C, U) == FALSE) {
    if (my.verbose) {
      int  color = __select_color(C->ftp.code);
      DISPLAY ( 
        color, "FTP/%d %6.2f secs: %7lu bytes ==> %-6s %s", 
        C->ftp.code, 0.0, bytes, url_get_method_name(U), url_get_request(U)
      );
    }
    xfree(D);
    client->fail += 1;
    return FALSE;
  }

  ftp_pasv(C);
  if (C->ftp.pasv == TRUE) {
    debug("Connecting to: %s:%d", C->ftp.host, C->ftp.port);
    D->sock = new_socket(D, C->ftp.host, C->ftp.port);
    if (D->sock < 0) {
      debug (
        "%s:%d connection failed. error %d(%s)",__FILE__, __LINE__, errno,strerror(errno)
      );
      client->fail += 1;
      socket_close(D);
      xfree(D);
      return FALSE;
    }
  }
  if (url_get_method(U) == POST || url_get_method(U) == PUT) {
    ftp_stor(C, U); 
    bytes = ftp_put(D, U);
    code  = C->ftp.code;
  } else {
    if (ftp_size(C, U) == TRUE) {
      if (ftp_retr(C, U) == TRUE) {
        bytes = ftp_get(D, U, C->ftp.size);
      }
    }
    code = C->ftp.code;
  }
  socket_close(D);
  ftp_quit(C);

  pass  = (bytes == C->ftp.size) ? 1 : 0;
  fail  = (pass  == 0) ? 1 : 0; 
  stop  =  times(&t_stop);
  etime =  elapsed_time(stop - start);
  client->bytes += bytes;
  client->time  += etime;
  client->code  += pass;
  client->fail  += fail;
  
  /**
   * check to see if this transaction is the longest or shortest
   */
  if (etime > himark) {
    himark = etime;
  }
  if ((lomark < 0) || (etime < lomark)) {
    lomark = etime;
  }
  client->himark = himark;
  client->lomark = lomark;
 
  if (my.verbose) {
    int  color = __select_color(code);
    DISPLAY ( 
      color, "FTP/%d %6.2f secs: %7lu bytes ==> %-6s %s", 
      code, etime, bytes, url_get_method_name(U), url_get_request(U)
    );
  }
  client->hits++; 
  xfree(D);
  return TRUE;
}

#ifdef SIGNAL_CLIENT_PLATFORM
private void
signal_handler(int sig)
{
  pthread_exit(&sig);
}
 
private void
signal_init()
{
  struct sigaction sa;
 
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);
}
#else/*CANCEL_CLIENT_PLATFORM*/

void
clean_up()
{
  return;
}
#endif

private BOOLEAN 
__init_connection(CONN *C, URL U, CLIENT *client)
{
  C->pos_ini              = 0;
  C->inbuffer             = 0;
  C->content.transfer     = NONE;
  C->content.length       = 0;
  C->connection.keepalive = (C->connection.max==1)?0:my.keepalive;
  C->connection.reuse     = (C->connection.max==1)?0:my.keepalive;
  C->connection.tested    = (C->connection.tested==0)?1:C->connection.tested;
  C->auth.www             = client->auth.www;
  C->auth.wchlg           = client->auth.wchlg;
  C->auth.wcred           = client->auth.wcred;
  C->auth.proxy           = client->auth.proxy;
  C->auth.pchlg           = client->auth.pchlg;
  C->auth.pcred           = client->auth.pcred;
  C->auth.type.www        = client->auth.type.www;
  C->auth.type.proxy      = client->auth.type.proxy;
  memset(C->buffer, 0, sizeof(C->buffer));

  debug (
    "%s:%d attempting connection to %s:%d",
    __FILE__, __LINE__,
    (auth_get_proxy_required(my.auth))?auth_get_proxy_host(my.auth):url_get_hostname(U),
    (auth_get_proxy_required(my.auth))?auth_get_proxy_port(my.auth):url_get_port(U)
  );

  if (!C->connection.reuse || C->connection.status == 0) {
    if (auth_get_proxy_required(my.auth)) {
      debug (
        "%s:%d creating new socket:     %s:%d", 
        __FILE__, __LINE__, auth_get_proxy_host(my.auth), auth_get_proxy_port(my.auth)
      );
      C->sock = new_socket(C, auth_get_proxy_host(my.auth), auth_get_proxy_port(my.auth));
    } else {
      debug (
        "%s:%d creating new socket:     %s:%d", 
        __FILE__, __LINE__, url_get_hostname(U), url_get_port(U)
      );
      C->sock = new_socket(C, url_get_hostname(U), url_get_port(U));
    }
  }

  if (my.keepalive) {
    C->connection.reuse = TRUE;
  }

  if (C->sock < 0) {
    debug (
      "%s:%d connection failed. error %d(%s)",__FILE__, __LINE__, errno,strerror(errno)
    );
    socket_close(C);
    return FALSE;
  }

  debug (
    "%s:%d good socket connection:  %s:%d",
    __FILE__, __LINE__,
    (auth_get_proxy_required(my.auth))?auth_get_proxy_host(my.auth):url_get_hostname(U),
    (auth_get_proxy_required(my.auth))?auth_get_proxy_port(my.auth):url_get_port(U)
  );

  if (C->encrypt == TRUE) {
    if (auth_get_proxy_required(my.auth)) {
      https_tunnel_request(C, url_get_hostname(U), url_get_port(U));
      https_tunnel_response(C);
    }
    C->encrypt = TRUE;
    if (SSL_initialize(C, url_get_hostname(U))==FALSE) {
      return FALSE;
    }
  }
  return TRUE;
}

private void
__increment_failures()
{
  pthread_mutex_lock(&(my.lock));
  my.failed++;
  pthread_mutex_unlock(&(my.lock));
  pthread_testcancel();
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

