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
#ifndef __FTP_H
#define __FTP_H

#include <sock.h>
#include <url.h>

BOOLEAN ftp_login(CONN *C, URL U);
BOOLEAN ftp_ascii(CONN *C);
BOOLEAN ftp_binary(CONN *C);
BOOLEAN ftp_size(CONN *C, URL U);
BOOLEAN ftp_cwd(CONN *C, URL U);
BOOLEAN ftp_pasv(CONN *C);
BOOLEAN ftp_list(CONN *C, CONN *D, URL U);
BOOLEAN ftp_stor(CONN *C, URL U);
BOOLEAN ftp_retr(CONN *C, URL U);
size_t  ftp_get(CONN *D, URL U, size_t size);
size_t  ftp_put(CONN *D, URL U);
BOOLEAN ftp_quit(CONN *C);

#endif/*__FTP_H*/
