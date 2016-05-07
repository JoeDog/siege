##
## src/Makefile.am
##
## Copyright (C) 2000-2010 by
## Jeffrey Fulmer - <jeff@joedog.org>, et al.
## This file is distributed as part of Siege
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
##

bin_PROGRAMS       =   siege

WARN_CFLAGS        =   @WARN_CFLAGS@

AM_CFLAGS          =   $(SSL_INCLUDE) $(Z_INCLUDE) $(PTHREAD_CFLAGS) $(WARN_CFLAGS) $(SSL_CFLAGS) $(Z_CFLAGS)

AM_LDFLAGS         =   $(SSL_LDFLAGS) $(Z_LDFLAGS) $(PTHREAD_LDFLAGS)  

LIBS               =   $(SSL_LIBS) $(Z_LIBS)

siege_SOURCES      =   \
ansidecl.h             \
array.c    array.h     \
auth.c     auth.h      \
base64.c   base64.h    \
browser.c  browser.h   \
cache.c    cache.h     \
cookie.c   cookie.h    \
cookies.c  cookies.h   \
cfg.c      cfg.h       \
creds.c    creds.h     \
crew.c     crew.h      \
data.c     data.h      \
date.c     date.h      \
eval.c     eval.h      \
ftp.c      ftp.h       \
getopt.c   getopt1.c   \
handler.c  handler.h   \
hash.c     hash.h      \
http.c     http.h      \
init.c     init.h      \
load.c     load.h      \
log.c      log.h       \
main.c     setup.h     \
md5.c      md5.h       \
memory.c   memory.h    \
notify.c   notify.h    \
page.c     page.h      \
parser.c   parser.h    \
perl.c     perl.h      \
response.c response.h  \
sock.c     sock.h      \
ssl.c      ssl.h       \
stralloc.c stralloc.h  \
timer.c    timer.h     \
url.c      url.h       \
util.c     util.h      \
version.c  version.h

AUTOMAKE_OPTIONS   =   foreign no-dependencies                   
 
