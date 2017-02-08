/**
 * SIEGE socket library
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

#ifdef  HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

#include <setup.h> 
#include <sock.h>
#include <util.h>
#include <memory.h>
#include <notify.h>
#include <joedog/boolean.h>
#include <joedog/defs.h>
#include <pthread.h>
#include <fcntl.h>

#ifdef HAVE_POLL
# include <poll.h>
#endif/*HAVE_POLL*/

#ifdef  HAVE_UNISTD_H
# include <unistd.h>
#endif/*HAVE_UNISTD_H*/

#ifdef  HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif/*HAVE_ARPA_INET_H*/
 
#ifdef  HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif/*HAVE_SYS_SOCKET_H*/ 

#ifdef  HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif/*HAVE_NETINET_IN_H*/
 
#ifdef  HAVE_NETDB_H
# include <netdb.h>
#endif/*HAVE_NETDB_H*/

#ifdef  HAVE_SSL
# include <openssl/rand.h>
#endif/*HAVE_SSL*/

/** 
 * local prototypes 
 */
private int     __socket_block(int socket, BOOLEAN block);
private ssize_t __socket_write(int sock, const void *vbuf, size_t len);  
private BOOLEAN __socket_check(CONN *C, SDSET mode);
private BOOLEAN __socket_select(CONN *C, SDSET mode);
#ifdef  HAVE_POLL
private BOOLEAN __socket_poll(CONN *C, SDSET mode);
#endif/*HAVE_POLL*/
#ifdef  HAVE_SSL
private ssize_t __ssl_socket_write(CONN *C, const void *vbuf, size_t len);
#endif/*HAVE_SSL*/

/**
 * new_socket
 * returns int, socket handle
 */
int
new_socket(CONN *C, const char *hostparam, int portparam)
{
  int conn;
  int res;
  int opt;
  int herrno;
  struct sockaddr_in cli; 
  struct hostent     *hp;
  char   hn[512];
  int    port;
#if defined(__GLIBC__)
  struct hostent hent;
  char hbf[8192];
#elif defined(sun)
# ifndef HAVE_GETIPNODEBYNAME
  struct hostent hent;
  char hbf[8192];
# endif/*HAVE_GETIPNODEBYNAME*/
#elif defined(_AIX)
  char *aixbuf;
  int  rc;
#endif/*_AIX*/

  if (hostparam == NULL) {
    NOTIFY(ERROR, "Unable to resolve host %s:%d",  __FILE__, __LINE__);
    return -1; 
  }

  C->encrypt  = (C->scheme == HTTPS) ? TRUE: FALSE;
  C->state    = UNDEF;
  C->ftp.pasv = TRUE;
  C->ftp.size = 0;

  memset(hn, '\0', sizeof hn);
 
  /* if we are using a proxy, then we make a socket
     connection to that server rather then a httpd */ 
  if (auth_get_proxy_required(my.auth)) {
    snprintf(hn, sizeof(hn), "%s", auth_get_proxy_host(my.auth));
    port = auth_get_proxy_port(my.auth);
  } else {
    snprintf(hn, sizeof(hn), "%s", hostparam);
    port = portparam;
  }

  /* create a socket, return -1 on failure */
  if ((C->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    switch (errno) {
      case EPROTONOSUPPORT: { NOTIFY(ERROR, "unsupported protocol %s:%d",  __FILE__, __LINE__); break; }
      case EMFILE:          { NOTIFY(ERROR, "descriptor table full %s:%d", __FILE__, __LINE__); break; }
      case ENFILE:          { NOTIFY(ERROR, "file table full %s:%d",       __FILE__, __LINE__); break; }
      case EACCES:          { NOTIFY(ERROR, "permission denied %s:%d",     __FILE__, __LINE__); break; }
      case ENOBUFS:         { NOTIFY(ERROR, "insufficient buffer %s:%d",   __FILE__, __LINE__); break; }
      default:              { NOTIFY(ERROR, "unknown socket error %s:%d",  __FILE__, __LINE__); break; }
    } socket_close(C); return -1;
  }
  if (fcntl(C->sock, F_SETFD, O_NDELAY) < 0) {
    NOTIFY(ERROR, "unable to set close control %s:%d", __FILE__, __LINE__);
  }

#if defined(__GLIBC__)
  {
    memset(hbf, '\0', sizeof hbf);
    /* for systems using GNU libc */
    if ((gethostbyname_r(hostparam, &hent, hbf, sizeof(hbf), &hp, &herrno) < 0)) {
      hp = NULL;
    }
  }
#elif defined(sun)
# ifdef HAVE_GETIPNODEBYNAME
  hp = getipnodebyname(hn, AF_INET, 0, &herrno);
# else /* default use gethostbyname_r*/
  {
    memset(hbf, '\0', sizeof hbf);
    hp = gethostbyname_r(hn, &hent, hbf, sizeof(hbf), &herrno); 
  }
# endif/*HAVE_GETIPNODEBYNAME*/
#elif defined(_AIX)
  aixbuf = (char*)xmalloc(8192);
  rc  = gethostbyname_r(hn, (struct hostent *)aixbuf,
                       (struct hostent_data *)(aixbuf + sizeof(struct hostent)));
  hp = (struct hostent*)aixbuf;
#elif (defined(hpux) || defined(__hpux) || defined(__osf__))
  hp = gethostbyname(hn);
  herrno = h_errno;
#else
  /**
   * Let's just hope gethostbyname is tread-safe
   */
  hp = gethostbyname(hn);
  herrno = h_errno;
#endif/*OS SPECIFICS*/ 

  /**
   * If hp is NULL, then we did not get good information
   * from the name server. Let's notify the user and bail
   */
  if (hp == NULL) {
    switch(herrno) {
      case HOST_NOT_FOUND: { NOTIFY(ERROR, "Host not found: %s\n", hostparam);                           break; }
      case NO_ADDRESS:     { NOTIFY(ERROR, "Host does not have an IP address: %s\n", hostparam);         break; }
      case NO_RECOVERY:    { NOTIFY(ERROR, "A non-recoverable resolution error for %s\n", hostparam);    break; }
      case TRY_AGAIN:      { NOTIFY(ERROR, "A temporary resolution error for %s\n", hostparam);          break; }
      default:             { NOTIFY(ERROR, "Unknown error code from gethostbyname for %s\n", hostparam); break; }
    }
    return -1; 
  } 

  memset((void*) &cli, 0, sizeof(cli));
  memcpy(&cli.sin_addr, hp->h_addr, hp->h_length);
#if defined(sun)
# ifdef  HAVE_FREEHOSTENT
  freehostent(hp);
# endif/*HAVE_FREEHOSTENT*/ 
#endif
  cli.sin_family = AF_INET;
  cli.sin_port = htons(port);

  if (C->connection.keepalive) {
    opt = 1; 
    if (setsockopt(C->sock,SOL_SOCKET,SO_KEEPALIVE,(char *)&opt,sizeof(opt))<0) {
      switch (errno) {
        case EBADF:       { NOTIFY(ERROR, "invalid descriptor %s:%d",    __FILE__, __LINE__); break; }
        case ENOTSOCK:    { NOTIFY(ERROR, "not a socket %s:%d",          __FILE__, __LINE__); break; }
        case ENOPROTOOPT: { NOTIFY(ERROR, "not a protocol option %s:%d", __FILE__, __LINE__); break; }
        case EFAULT:      { NOTIFY(ERROR, "setsockopt unknown %s:%d",    __FILE__, __LINE__); break; }
        default:          { NOTIFY(ERROR, "unknown sockopt error %s:%d", __FILE__, __LINE__); break; }
      } socket_close(C); return -1;
    }
  }

  if ((__socket_block(C->sock, FALSE)) < 0) {
    NOTIFY(ERROR, "socket: unable to set socket to non-blocking %s:%d", __FILE__, __LINE__);
    return -1; 
  }

  /**
   * connect to the host 
   * evaluate the server response and check for
   * readability/writeability of the socket....
   */ 
  conn = connect(C->sock, (struct sockaddr *)&cli, sizeof(struct sockaddr_in));
  pthread_testcancel();
  if (conn < 0 && errno != EINPROGRESS) {
    switch (errno) {
      case EACCES:        {NOTIFY(ERROR, "socket: %d EACCES",                  pthread_self()); break;}
      case EADDRNOTAVAIL: {NOTIFY(ERROR, "socket: %d address is unavailable.", pthread_self()); break;}
      case ETIMEDOUT:     {NOTIFY(ERROR, "socket: %d connection timed out.",   pthread_self()); break;}
      case ECONNREFUSED:  {NOTIFY(ERROR, "socket: %d connection refused.",     pthread_self()); break;}
      case ENETUNREACH:   {NOTIFY(ERROR, "socket: %d network is unreachable.", pthread_self()); break;}
      case EISCONN:       {NOTIFY(ERROR, "socket: %d already connected.",      pthread_self()); break;}
      default:            {NOTIFY(ERROR, "socket: %d unknown network error.",  pthread_self()); break;}
    } socket_close(C); return -1;
  } else {
    if (__socket_check(C, READ) == FALSE) {
      pthread_testcancel();
      NOTIFY(WARNING, "socket: read check timed out(%d) %s:%d", my.timeout, __FILE__, __LINE__);
      socket_close(C);
      return -1; 
    } else { 
      /**
       * If we reconnect and receive EISCONN, then we have a successful connection
       */
      res = connect(C->sock, (struct sockaddr *)&cli, sizeof(struct sockaddr_in)); 
      if((res < 0)&&(errno != EISCONN)){
        NOTIFY(ERROR, "socket: unable to connect %s:%d", __FILE__, __LINE__);
        socket_close(C);
        return -1; 
      }
      C->status = S_READING; 
    }
  } /* end of connect conditional */

  if ((__socket_block(C->sock, TRUE)) < 0) {
    NOTIFY(ERROR, "socket: unable to set socket to non-blocking %s:%d", __FILE__, __LINE__);
    return -1; 
  }

  C->connection.status = 1; 
  return(C->sock);
}

/**
 * Conditionally determines whether or not a socket is ready.
 * This function calls __socket_poll if HAVE_POLL is defined in
 * config.h, else it uses __socket_select
 */
private BOOLEAN 
__socket_check(CONN *C, SDSET mode)
{
#ifdef HAVE_POLL
 if (C->sock >= FD_SETSIZE) {
   return __socket_poll(C, mode);
 } else {
   return __socket_select(C, mode);
 } 
#else 
 return __socket_select(C, mode);
#endif/*HAVE_POLL*/
}

#ifdef HAVE_POLL
private BOOLEAN
__socket_poll(CONN *C, SDSET mode)
{
  int res;
  int timo = (my.timeout) ? my.timeout * 1000 : 15000;
  __socket_block(C->sock, FALSE);

  C->pfd[0].fd     = C->sock + 1;
  C->pfd[0].events |= POLLIN;

  do {
    res = poll(C->pfd, 1, timo);
    pthread_testcancel();
    if (res < 0) puts("LESS THAN ZERO!");
  } while (res < 0); // && errno == EINTR);

  if (res == 0) {
    errno = ETIMEDOUT;
  }
 
  if (res <= 0) {
    C->state = UNDEF;
    NOTIFY(WARNING, 
      "socket: polled(%d) and discovered it's not ready %s:%d", 
      (my.timeout)?my.timeout:15, __FILE__, __LINE__
    );
    return FALSE;
  } else {
    C->state = mode;
    return TRUE;
  }
}
#endif/*HAVE_POLL*/

private BOOLEAN
__socket_select(CONN *C, SDSET mode)
{
  struct timeval timeout;
  int    res;
  fd_set rs;
  fd_set ws;
  memset((void *)&timeout, '\0', sizeof(struct timeval));
  timeout.tv_sec  = (my.timeout > 0)?my.timeout:30;
  timeout.tv_usec = 0;

  if ((C->sock < 0) || (C->sock >= FD_SETSIZE)) {
    // FD_SET can't handle it
    return FALSE;
  }

  do {
    FD_ZERO(&rs);
    FD_ZERO(&ws);
    FD_SET(C->sock, &rs);
    FD_SET(C->sock, &ws);
    res = select(C->sock+1, &rs, &ws, NULL, &timeout);
    pthread_testcancel();
  } while (res < 0 && errno == EINTR);

  if (res == 0) {
    errno = ETIMEDOUT;
  }

  if (res <= 0) {
    C->state = UNDEF;
    NOTIFY(WARNING, "socket: select and discovered it's not ready %s:%d", __FILE__, __LINE__);
    return FALSE;
  } else {
    C->state = mode;
    return TRUE;
  }
}

/**
 * local function
 * set socket to non-blocking
 */
private int
__socket_block(int sock, BOOLEAN block)
{
#if HAVE_FCNTL_H 
  int flags;
  int retval;
#elif defined(FIONBIO)
  ioctl_t status;
#else 
  return sock;
#endif
// return sock;
  if (sock==-1) {
    return sock;
  }

#if HAVE_FCNTL_H 
  if ((flags = fcntl(sock, F_GETFL, 0)) < 0) {
    switch (errno) {
      case EACCES: { NOTIFY(ERROR, "EACCES %s:%d",                 __FILE__, __LINE__); break; }
      case EBADF:  { NOTIFY(ERROR, "bad file descriptor %s:%d",    __FILE__, __LINE__); break; }
      case EAGAIN: { NOTIFY(ERROR, "address is unavailable %s:%d", __FILE__, __LINE__); break; }
      default:     { NOTIFY(ERROR, "unknown network error %s:%d",  __FILE__, __LINE__); break; }
    } return -1;
  }

  if (block) { 
    flags &= ~O_NDELAY;
  } else {
    flags |=  O_NDELAY;
    #if (defined(hpux) || defined(__hpux) || defined(__osf__)) || defined(__sun)
    #else
    flags |=  O_NONBLOCK;
    #endif
  }

  if ((retval = fcntl(sock, F_SETFL, flags)) < 0) {
    NOTIFY(ERROR, "unable to set fcntl flags %s:%d", __FILE__, __LINE__);
    return -1;
  } 
  return retval;

#elif defined(FIONBIO)
  status = block ? 0 : 1;
  return ioctl(sock, FIONBIO, &status);
#endif
}  

/**
 * returns ssize_t
 * writes vbuf to sock
 */
private ssize_t
__socket_write(int sock, const void *vbuf, size_t len)
{
  size_t      n;
  ssize_t     w;
  const char *buf;
 
  buf = vbuf;
  n   = len;
  while (n > 0) {
    if ((w = write( sock, buf, n)) <= 0) {
      if (errno == EINTR) {
        w = 0;
      } else {
        return -1;
      }
    }
    n   -= w;
    buf += w;
  }
  return len;
}

/**
 * local function
 * returns ssize_t
 * writes vbuf to sock
 */
#ifdef  HAVE_SSL
private ssize_t
__ssl_socket_write(CONN *C, const void *vbuf, size_t len)
{
  size_t      n;
  ssize_t     w;
  const char *buf;
  int         err;

  buf = vbuf;
  n   = len;

  while (n > 0) {
    if ((w = SSL_write(C->ssl, buf, n)) <= 0) {
      if (w < 0) {
        err = SSL_get_error(C->ssl, w);

        switch (err) {
          case SSL_ERROR_WANT_READ:
          case SSL_ERROR_WANT_WRITE:
			NOTIFY(DEBUG, "SSL_write non-critical error %d", err);
          return 0;
        case SSL_ERROR_SYSCALL:
          NOTIFY(ERROR, "SSL_write() failed (syscall)");
          return -1;
        case SSL_ERROR_SSL:
          return -1;
        }
      }
      NOTIFY(ERROR, "SSL_write() failed.");
      return -1;
    }
    n   -= w;
    buf += w;
  }
  return len;
}
#endif/*HAVE_SSL*/

ssize_t
socket_read(CONN *C, void *vbuf, size_t len)
{
  int type;
  size_t      n;
  ssize_t     r;
  char *buf;
  int ret_eof = 0;
 
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &type);
 
  buf = vbuf;
  n   = len;
  if (C->encrypt == TRUE) {
  #ifdef HAVE_SSL
    while (n > 0) {
      if (__socket_check(C, READ) == FALSE) {
        NOTIFY(WARNING, "socket: read check timed out(%d) %s:%d", (my.timeout)?my.timeout:15, __FILE__, __LINE__);
	return -1;
      }
	  r = SSL_read(C->ssl, buf, n);
      if (r < 0) {
        if (errno == EINTR || SSL_get_error(C->ssl, r) == SSL_ERROR_WANT_READ)
          r = 0;
        else
          return -1;
      }
      else if (r == 0) break;
      n   -= r;
      buf += r;
    }   /* end of while    */
  #endif/*HAVE_SSL*/
  } else { 
    while (n > 0) {
      if (C->inbuffer < len) {
        if (__socket_check(C, READ) == FALSE) {
          NOTIFY(WARNING, "socket: read check timed out(%d) %s:%d", (my.timeout)?my.timeout:15, __FILE__, __LINE__);
          return -1;
        }
      }
      if (C->inbuffer <  n) {
        int lidos;
        memmove(C->buffer,&C->buffer[C->pos_ini],C->inbuffer);
        C->pos_ini = 0;
	if (__socket_check(C, READ) == FALSE) {
          NOTIFY(WARNING, "socket: read check timed out(%d) %s:%d", (my.timeout)?my.timeout:15, __FILE__, __LINE__);
	  return -1;
	}
        lidos = read(C->sock, &C->buffer[C->inbuffer], sizeof(C->buffer)-C->inbuffer);
        if (lidos == 0)
          ret_eof = 1;
        if (lidos < 0) {
          if (errno==EINTR || errno==EAGAIN)
            lidos = 0;
          if (errno==EPIPE){
            return 0;
          } else {
            NOTIFY(ERROR, "socket: read error %s %s:%d", strerror(errno), __FILE__, __LINE__);
            return 0; /* was return -1 */
          }
        }
        C->inbuffer += lidos;
      }
      if (C->inbuffer >= n) {
        r = n;
      } else {
        r = C->inbuffer;
      }
      if (r == 0) break;
      memmove(buf,&C->buffer[C->pos_ini],r);
      C->pos_ini  += r;
      C->inbuffer -= r;
      n   -= r;
      buf += r;
      if (ret_eof) break;
    } /* end of while */
  }   /* end of else  */
 
  pthread_setcanceltype(type,NULL);
  pthread_testcancel();
  return (len - n);
}                                                                                                    
/**
 * this function is used for chunked
 * encoding transfers to acquire the 
 * size of the message check.
 */
ssize_t
socket_readline(CONN *C, char *ptr, size_t maxlen)
{
  int type;
  int n, len, res;
  char c;

  len = maxlen;
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &type); 

  for (n = 1; n < len; n ++) {
    if ((res = socket_read(C, &c, 1)) == 1) {
      *ptr++ = c;
      if (c=='\n') break;
    }
    else if (res == 0) {
      if (n == 1) 
        return 0; 
      else 
        break; 
    } else {
      return -1; /* something bad happened */
    }
  } /* end of for loop */

  *ptr=0;
  
  pthread_setcanceltype(type,NULL);
  pthread_testcancel(); 

  return n;
}

/**
 * returns void
 * socket_write wrapper function.
 */
int
socket_write(CONN *C, const void *buf, size_t len)
{
  int     type;
  size_t bytes;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &type); 

  if (C->encrypt == TRUE) {
    /* handle HTTPS protocol */
    #ifdef HAVE_SSL
    /** 
     * Yeah, sure, this looks like a potential 
     * endless loop, (see: Loop, endless), but 
     * a socket timeout will break it...
     */
    do {
      if ((bytes = __ssl_socket_write(C, buf, len)) != len) {
        if (bytes == 0)
          ;
        else 
          return -1;
      }
    } while (bytes == 0);
    #else
    NOTIFY(ERROR, "%s:%d protocol NOT supported", __FILE__, __LINE__);
    return -1;
    #endif/*HAVE_SSL*/
  } else {
    /* assume HTTP */
    if ((bytes = __socket_write(C->sock, buf, len)) != len) {
      NOTIFY(ERROR, "unable to write to socket %s:%d", __FILE__, __LINE__);
      return -1;
    }
  }

  pthread_setcanceltype(type,NULL); 
  pthread_testcancel(); 

  return bytes;
} 

/**
 * returns void
 * frees ssl resources if using ssl and
 * closes the connection and the socket.
 */
void
socket_close(CONN *C)
{
  int   type;
  int   ret   = 0;
#ifdef  HAVE_SSL
  int tries = 0;
#endif/*HAVE_SSL*/
  if (C==NULL) return;

  /* XXX Is this necessary? */ 
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &type); 

  if (C->encrypt == TRUE) {
#ifdef  HAVE_SSL
    if (!C->connection.reuse || C->connection.max == 1){
      if (C->ssl != NULL) {
        do {
          ret = SSL_get_shutdown(C->ssl);
          if (ret < 0) { 
            NOTIFY(WARNING, "socket: SSL Socket closed by server: %s:%d", __FILE__, __LINE__);
            break; //what an asshole; is this IIS? 
          }

          ret = SSL_shutdown(C->ssl);
          if (ret == 1) {
            break;
          }
          tries++;
        } while(tries < 5);
      }
      SSL_free(C->ssl);
      C->ssl = NULL;
      SSL_CTX_free(C->ctx);
      C->ctx = NULL;
      close(C->sock);
      C->sock              = -1;
      C->connection.status =  0;
      C->connection.max    =  0;
      C->connection.tested =  0;
    }
#endif/*HAVE_SSL*/
  } else {
    if (C->connection.reuse == 0 || C->connection.max == 1) {
      if (C->sock != -1) {
        if ((__socket_block(C->sock, FALSE)) < 0)
          NOTIFY(ERROR, "unable to set to non-blocking %s:%d", __FILE__, __LINE__);
        if ((C->connection.status > 1)&&(ret = shutdown(C->sock, 2)) < 0)
          NOTIFY(ERROR, "unable to shutdown the socket %s:%d", __FILE__, __LINE__);
        if ((ret = close(C->sock)) < 0)
          NOTIFY(ERROR, "unable to close the socket %s:%d",    __FILE__, __LINE__);
      }
      C->sock                 = -1;
      C->connection.status    =  0;
      C->connection.max       =  0;
      C->connection.tested    =  0;
    }
  }
  C = NULL;
  pthread_setcanceltype(type,NULL);
  pthread_testcancel(); 

  return;
} 


