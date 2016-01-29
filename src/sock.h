/**
 * SIEGE socket header file
 *
 * Copyright (C) 2000-2016 by
 * by Jeffrey Fulmer - <jeff@joedog.org>, et al.
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
#ifndef SOCK_H
#define SOCK_H

#ifdef  HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif/*HAVE_NETINET_IN_H*/
 
#ifdef  HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif/*HAVE_ARPA_INET_H*/
 
#ifdef  HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif/*HAVE_SYS_SOCKET_H*/
 
#ifdef  HAVE_NETDB_H
# include <netdb.h>
#endif/*HAVE_NETDB_H*/ 

#ifdef  HAVE_POLL
# include <poll.h>
#endif/*HAVE_POLL*/

#ifdef  HAVE_SSL
# include <openssl/ssl.h>
# include <openssl/err.h>
# include <openssl/rsa.h>
# include <openssl/crypto.h>
# include <openssl/x509.h>
# include <openssl/pem.h>
#endif/*HAVE_SSL*/

#include <auth.h>
#include <page.h>
#include <cache.h>
#include <joedog/boolean.h>

typedef enum
{
  S_CONNECTING = 1,
  S_READING    = 2,
  S_WRITING    = 4,
  S_DONE       = 8
} S_STATUS; 

typedef enum
{
  UNDEF = 0,
  READ  = 1,
  WRITE = 2,
  RDWR  = 3
} SDSET;  

typedef struct
{
  int      sock;       /* socket file descriptor          */
  S_STATUS status; 
  BOOLEAN  encrypt;    /* TRUE=encrypt, FALSE=clear       */
  SCHEME   scheme;
  PAGE     page;
  CACHE    cache;
  struct {
    int    transfer;   /* transfer encoding specified     */
    size_t length;     /* length of data chunks           */
  } content;           /* content encoding data           */
  struct {
    int  max;          /* max number of keep-alives       */ 
    int  timeout;      /* server specified timeout value  */
    int  reuse;        /* boolean, reuse socket if TRUE   */
    int  status;       /* socket status: open=1, close=0  */
    int  keepalive;    /* keep-alive header directive     */
    int  tested;       /* boolean, has socket been tested */
  } connection;        /* persistent connection data      */
  struct {
    DCHLG *wchlg;
    DCRED *wcred;
    int    www;
    DCHLG *pchlg;
    DCRED *pcred;
    int    proxy;
    struct {
      TYPE www;
      TYPE proxy;
    } type;
  } auth;
#ifdef  HAVE_SSL
  SSL        *ssl;
  SSL_CTX    *ctx;
  const SSL_METHOD *method;
  X509       *cert;
#else 
  BOOLEAN     nossl;
#endif/*HAVE_SSL*/
  size_t   inbuffer;
  int      pos_ini;
  char     buffer[4096];
  char     chkbuf[1024];
#ifdef  HAVE_POLL
  struct   pollfd pfd[1];
#endif/*HAVE_POLL*/
  fd_set   *ws;
  fd_set   *rs;
  SDSET    state;  
  struct {
    int      code; 
    char     host[64]; /* FTP data host */
    int      port;     /* FTP data port */
    size_t   size;     /* FTP file size */
    BOOLEAN  pasv;
  } ftp;
} CONN; 

int       new_socket     (CONN *conn, const char *hostname, int port);
BOOLEAN   socket_check   (CONN *C, SDSET test);
int       socket_write   (CONN *conn, const void *b, size_t n);
ssize_t   socket_read    (CONN *conn, void *buf, size_t len); 
ssize_t   socket_readline(CONN *C, char *ptr, size_t maxlen);  
void      socket_close   (CONN *C);

#endif /* SOCK_H */

