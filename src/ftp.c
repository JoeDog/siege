/**
 * FTP protocol support
 *
 * Copyright (C) 2013-2014 by
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
#include <stdio.h>
#include <util.h>
#include <ftp.h>
#include <load.h>
#include <perl.h>
#include <memory.h>
#include <notify.h>
#include <pthread.h>
#include <joedog/defs.h>

private int     __request(CONN *C, char *fmt, ...);
private int     __response(CONN *C);
private int     __response_code(const char *buf);
private BOOLEAN __in_range(int code, int lower, int upper);

BOOLEAN
ftp_login(CONN *C, URL U)
{
  int  code = 120;
  char tmp[128]; 

  code = __response(C);
  if (! okay(code)) {
    NOTIFY(ERROR, "FTP: Server responded: %d", code);
    return FALSE;
  }

  snprintf(tmp, sizeof(tmp), "%s", (url_get_username(U)==NULL)?"anonymous":url_get_username(U));
  code = __request(C, "USER %s", tmp); 
  if (code != 331) {
    if (okay(code)) return TRUE; 
  }

  memset(tmp, '\0', sizeof(tmp));
  snprintf(tmp, sizeof(tmp), "%s", (url_get_password(U)==NULL)?"siege@joedog.org":url_get_password(U));
  code = __request(C, "PASS %s", tmp);
  return __in_range(code, 200, 299);
}

BOOLEAN
ftp_pasv(CONN *C)
{
  int  i, code;
  char *ptr;
  unsigned char addr[6];

  code = __request(C, "PASV");
  if (!okay(code)) return FALSE;

  ptr = (char *) C->chkbuf;
  for (ptr += 4; *ptr && !isdigit(*ptr); ptr++);

  if (!*ptr) return FALSE;

  for (i = 0; i < 6; i++) {
    addr[i] = 0;
    for (; isdigit(*ptr); ptr++)
      addr[i] = (*ptr - '0') + 10 * addr[i];

    if (*ptr == ',') ptr++;
    else if (i < 5)  return FALSE;
  }
  snprintf(C->ftp.host, sizeof(C->ftp.host), "%d.%d.%d.%d", addr[0],addr[1],addr[2],addr[3]);
  C->ftp.port = (addr[4] << 8) + addr[5];

  return TRUE;  
}

BOOLEAN
ftp_cwd(CONN *C, URL U) 
{
  int code;

  code = __request(C, "CWD %s", url_get_path(U));
  return okay(code);
}

BOOLEAN
ftp_ascii(CONN *C)
{
  C->ftp.code = __request(C, "TYPE A");
  return okay(C->ftp.code); 
}

BOOLEAN
ftp_binary(CONN *C)
{
  C->ftp.code = __request(C, "TYPE I");
  return okay(C->ftp.code); 
}

BOOLEAN
ftp_quit(CONN *C)
{
  C->ftp.code = __request(C, "QUIT");
  return okay(C->ftp.code); 
}

BOOLEAN 
ftp_size(CONN *C, URL U)
{
  int size;
  int resp;
  
  if (ftp_binary(C) != TRUE) return FALSE;

  C->ftp.code = __request(C, "SIZE %s%s", url_get_path(U), url_get_file(U));
  if (!okay(C->ftp.code)) {
    return FALSE;
  } else {
    if (sscanf(C->chkbuf, "%d %d", &resp, &size) == 2) {
      C->ftp.size = size;
      return TRUE;
    } else {
      return FALSE;
    }
  }
  return TRUE;
}

BOOLEAN
ftp_stor(CONN *C, URL U) 
{
  size_t  len;
  char    *file;
  size_t  id = pthread_self();
  int     num = 2;
  char    **parts;
 
  if (id < 0.0) {
    id = -id;
  }
 
  len   = strlen(url_get_file(U))+17; 
  parts = split('.', url_get_file(U), &num);

  file = xmalloc(len);
  memset(file, '\0', len);

  /* NOTE: changed %u to %zu as per C99 */
  snprintf(file, len, "%s-%zu.%s", parts[0], id, (parts[1]==NULL)?"":parts[1]);
  if (my.unique) {
    C->ftp.code = __request(C, "STOR %s", file);
  } else {
    C->ftp.code = __request(C, "STOR %s", url_get_file(U));
  }
  xfree(file);
  split_free(parts, num);
  return (okay(C->ftp.code));
}


BOOLEAN
ftp_retr(CONN *C, URL U) 
{
  C->ftp.code = __request(C, "RETR %s%s", url_get_path(U), url_get_file(U));
  return (okay(C->ftp.code));
}

size_t
ftp_put(CONN *D, URL U)
{
  size_t n;
  if ((n = socket_write(D, url_get_postdata(U), url_get_postlen(U))) !=  url_get_postlen(U)){
    NOTIFY(ERROR, "HTTP: unable to write to socket." );
    return -1;
  }

  return url_get_postlen(U);
}

size_t
ftp_get(CONN *D, URL U, size_t size) 
{
  int  n;
  char c;
  size_t bytes = 0;
  char *file;

  file = xmalloc(size);
  memset(file, '\0', size);

  do {
    if ((n = socket_read(D, &c, 1)) == 0)
      break;
    file[bytes] = c;
    bytes += n;
  } while (bytes < size);

  if (my.get) {
    write_file(U, file, size);
  } 
  xfree(file);
  
  return bytes;
}

BOOLEAN
ftp_list(CONN *C, CONN *D, URL U)
{
  int  n;
  char c;
  int  bytes;

  C->ftp.code = __request(C, "LIST %s", (url_get_file(U)==NULL)?url_get_path(U):url_get_file(U));

  if (C->ftp.code == 150) {
    if (D->sock < 1) {
      NOTIFY(ERROR, "unable to read from socket: %s:%d", C->ftp.host, C->ftp.port);
      return FALSE;
    }
    do {
      if ((n = socket_read(D, &c, 1)) == 0)
        break;
      if (my.verbose) printf("%c", c);
      bytes += n;
    } while (TRUE);    
  }
  return TRUE; 
}

int
__request(CONN *C, char *fmt, ...)
{
  int     code = 0;
  char    buf[1024];
  char    cmd[1024+8];
  size_t  len, n;
  va_list ap;

  memset(buf, '\0', sizeof(buf));
  memset(cmd, '\0', sizeof(cmd));
  
  va_start(ap, fmt);
  vsnprintf(buf, sizeof buf, fmt, ap);
  len = snprintf(cmd, sizeof(cmd), "%s\015\012", buf);

  if ((n = socket_write(C, cmd, len)) != len) {
    NOTIFY(ERROR, "FTP: unable to write to socket.");
    code = 500;
  }
  va_end(ap);
  debug(chomp(cmd));

  if (code == 500) {
    C->ftp.code = 500;
    return C->ftp.code;
  } else {
    C->ftp.code = __response(C);
    return C->ftp.code;
  }
}


private int
__response(CONN *C) 
{
  int     n;
  char    c;
  int     code = 120;
  BOOLEAN cont = TRUE;

  while (cont) {
    int x;

    while (TRUE) {
      x = 0;
      memset(C->chkbuf, '\0', sizeof(C->chkbuf));
      while ((n = socket_read(C, &c, 1)) == 1) {
        echo("%c", c);
        C->chkbuf[x] = c; 
        if (C->chkbuf[x] == '\n') break;
        x++;
      }
      if (isdigit(C->chkbuf[0]) && (C->chkbuf[3] != '-')) break;
    }
    code = __response_code(C->chkbuf);
    if (C->chkbuf[3] == ' ') {
      cont = FALSE;
    }
  }
  if (code > 499 && !my.quiet) {
    printf("%s\n", chomp(C->chkbuf));
  }
  return code; 
}

private int
__response_code(const char *buf)
{
  int  ret;
  char code[4];
  memset(code, '\0', sizeof(code));
  strncpy(code, buf, 3);
  code[3] = '\0';
  ret = atoi(code);
  return ret;
}

private BOOLEAN 
__in_range(int code, int lower, int upper)
{
  return (code >= lower && code <= upper);
}

/**
  From RFC 959
  200 Command okay.
  202 Command not implemented, superfluous at this site.
  211 System status, or system help reply.
  212 Directory status.
  213 File status.
  214 Help message.
  215 NAME system type.
  220 Service ready for new user.
  221 Service closing control connection.
  225 Data connection open; no transfer in progress.
  226 Closing data connection.
  227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
  230 User logged in, proceed.
  250 Requested file action okay, completed.
  257 "PATHNAME" created.
  331 User name okay, need password.
  332 Need account for login.
  350 Requested file action pending further information.
  421 Service not available, closing control connection.
  425 Can't open data connection.
  426 Connection closed; transfer aborted.
  450 Requested file action not taken.
  451 Requested action aborted: local error in processing.
  452 Requested action not taken.
  500 Syntax error, command unrecognized.
  501 Syntax error in parameters or arguments.
  502 Command not implemented.
  503 Bad sequence of commands.
  504 Command not implemented for that parameter.
  530 Not logged in.
  532 Need account for storing files.
  550 Requested action not taken.
  551 Requested action aborted: page type unknown.
  552 Requested file action aborted.
  553 Requested action not taken.
*/
