dnl REQUIRE AUTOCONF 2.50 OR HIGHER...
AC_PREREQ(2.63) 

dnl
dnl Process this file with autoconf to produce a configure script.
dnl
AC_INIT(
  m4_esyscmd_s([sed -ne 's/.*program_name.*"\(.*\)"\;/\1/p' src/version.c]), 
  m4_esyscmd_s([sed -ne 's/.*version_string.*"\(.*\)"\;/\1/p' src/version.c]),
) 
AC_CONFIG_AUX_DIR(utils)  
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/setup.h])
AC_CONFIG_HEADERS(include/config.h)

dnl canonicalize the host
AC_CANONICAL_HOST

DATE=`${CONFIG_SHELL} ${srcdir}/utils/mkstamp`
VERSION=`sed -ne 's/.*version_string.*"\(.*\)"\;/\1/p' ${srcdir}/src/version.c`
PROGRAM=`sed -ne 's/.*program_name.*"\(.*\)"\;/\1/p' src/version.c`
AUTHOR=`sed -ne 's/.*author_name.*"\(.*\)"\;/\1/p' ${srcdir}/src/version.c`
EMAIL=`sed -ne 's/.*email_address.*"\(.*\)"\;/\1/p' ${srcdir}/src/version.c`
YEARS=`sed -ne 's/.*years.*"\(.*\)"\;/\1/p' ${srcdir}/src/version.c`
PLATFORM="${host_vendor}-${host_cpu}-${host_os}"
AC_SUBST(DATE)
AC_SUBST(PROGRAM)
AC_SUBST(AUTHOR)
AC_SUBST(EMAIL)
AC_SUBST(PLATFORM)
 
dnl Check for AIX weirdos
AC_AIX
 
dnl Checks for programs.
AC_PROG_CC 

AM_PROG_LIBTOOL 

dnl
dnl Program support
dnl
AC_PATH_PROG( PERL, perl, false )
AC_PROG_SHELL
AC_PROG_MAKE_SET
AC_PROG_INSTALL
case "$host_os" in
  *aix*)
    if test -n "${CC}" ; then
      AC_CHECK_PROGS(CC_R, xlc_r cc_r cc)
      if test "$CC_R" = cc ; then
        AC_MSG_ERROR([pthread support requires cc_r (or other suitable compiler) on AIX])
      else
        CC=$CC_R
        AC_SUBST(CC)
      fi
    fi
  ;;
esac 

if test -n "$GCC"; then
  oldCFLAGS=$CFLAGS
  CFLAGS="$CFLAGS -W -Wall -Werror -Wunused-value"
  AC_MSG_CHECKING(for buggy pthread mutex initializers)

  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([#include <pthread.h>],
    [pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; pthread_mutex_lock(&m); ])],
    [buggy_init="no"], [buggy_init="yes"])

  if test $buggy_init = "no" ; then
     WARN_CFLAGS="-W -Wall -Wunused-value"
     AC_MSG_RESULT(no)
  else
    AC_MSG_RESULT(yes)
    WARN_CFLAGS="-W -Wall -Wno-missing-braces -Wunused-value"
  fi
fi
CFLAGS=$oldCFLAGS
AC_SUBST(WARN_CFLAGS)

AC_MSG_CHECKING([for dlopen() in -ldld])
olibs=$LIBS
LIBS="$LIBS -ldld"
AC_TRY_LINK(, 
  [ extern void* dlopen(); dlopen(); ],
  AC_MSG_RESULT(yes);
  LIBS=$olibs;
  LDL=-ldld
  AC_DEFINE(HAVE_SHL_LOAD, 1, [ Define if we have shl_load() ]),
  AC_MSG_RESULT(no);
  LDL=
  LIBS=$olibs)

AC_MSG_CHECKING([for dlopen() in -ldl])
olibs=$LIBS
LIBS="$LIBS -ldl"
AC_TRY_LINK(,
  [ extern void* dlopen(); dlopen(); ],
  AC_MSG_RESULT(yes);
  LIBS=$olibs;
  LDL=-ldl,
  AC_MSG_RESULT(no);
  LDL=
  LIBS=$olibs)

AC_SUBST(LDL)

dnl
dnl check for random device
dnl
AC_CACHE_CHECK(for random device, ac_cv_have_dev_random,
[if test -r "/dev/random" && test -r "/dev/urandom" ; then
  ac_cv_have_dev_random=yes; else ac_cv_have_dev_random=no; fi])
if test "$ac_cv_have_dev_random" = yes; then
  AC_DEFINE([HAVE_DEV_RANDOM], 1, [Discovered a random device])
fi

dnl 
dnl with SSL support
dnl
MYSSL=off
sslfound=locate
SSL_INCLUDE=
SSL_LDFLAGS=
SSL_LIBS=
AC_ARG_WITH(ssl,dnl
[  --with-ssl=ARG          where ARG is the ssl directory or "no" ],
[  MYSSL="$withval"                                               ])
AC_ARG_WITH(nossl,dnl
[  --without-ssl           do NOT use SSL (same as --with-ssl=no) ],
[  MYSSL="no"                                                     ])
if test "$MYSSL" = "no"
then
  AC_MSG_RESULT(checking for ssl support... no)
else
  AC_MSG_RESULT(checking for ssl support... yes)
  if test "$MYSSL" = "off" ; then
    MYSSL=""
  fi
fi

dnl 
dnl Check for ssl libraries
dnl
if test "$MYSSL" = "no"
then
  AC_MSG_RESULT(HTTPS/ssl support disabled)
else
  AC_CHECK_HEADERS($MYSSL/include/openssl/opensslv.h, sslfound=yes, sslfound=locate)
  if test $sslfound = "locate"; then
     dnl the user probably misunderstood the option....
     for dir in /usr /usr/local /usr/local/ssl /usr/pkg /usr/lib/ssl /usr/include/ssl /usr/include; do
       AC_CHECK_HEADERS($dir/include/openssl/opensslv.h, sslfound=yes, sslfound=no)
       if test $sslfound = "yes" ; then
         SSL_CFLAGS="-DOPENSSL_NO_KRB5"
         SSL_LDFLAGS="-L$dir/lib"
         SSL_INCLUDE="-I$dir/include/openssl -I$dir/include"
         SSL_LIBS="-lssl -lcrypto"
         AC_MSG_CHECKING([for OpenSSL version])
         CPPFLAGS="$SSL_INCLUDE"
         AC_EGREP_CPP(yes,[
           #include <openssl/opensslv.h>
           #if OPENSSL_VERSION_NUMBER >= 0x0090800fL 
           yes
           #endif
         ],[
           SSL_LIBS="$LDL $SSL_LIBS"
           AC_MSG_RESULT([>= 0.9.8 (appropriate flag set)])
         ],[
           AC_MSG_RESULT([< 0.9.8])
         ]) 
         AC_SUBST(SSL_CFLAGS)
         AC_SUBST(SSL_INCLUDE)
         AC_SUBST(SSL_LDFLAGS)
         AC_SUBST(SSL_LIBS)
         AC_DEFINE([HAVE_SSL], 1, [Discovered OpenSSL library for HTTPS])
         break
      fi
    done
  else
    echo "found ssl in $MYSSL"
    SSL_CFLAGS="-DOPENSSL_NO_KRB5"
    SSL_LDFLAGS="-L$MYSSL/lib"
    SSL_INCLUDE="-I$MYSSL/include/openssl -I$MYSSL/include"
    SSL_LIBS="-lssl -lcrypto"
    AC_MSG_CHECKING([for OpenSSL version])
    CPPFLAGS="$SSL_INCLUDE"
    AC_EGREP_CPP(yes,[
      #include <openssl/opensslv.h>
      #if OPENSSL_VERSION_NUMBER >= 0x0090800fL 
      yes
      #endif
    ],[
      SSL_LIBS="$LDL $SSL_LIBS"
      AC_MSG_RESULT([>= 0.9.8 (appropriate flag set)])
    ],[
      AC_MSG_RESULT([< 0.9.8])
    ]) 
    AC_SUBST(SSL_CFLAGS)
    AC_SUBST(SSL_INCLUDE)
    AC_SUBST(SSL_LDFLAGS)
    AC_SUBST(SSL_LIBS)
    AC_DEFINE(HAVE_SSL)
  fi
fi

dnl 
dnl with zlib support
dnl
MYZ=off
zfound=locate
Z_INCLUDE=
Z_LDFLAGS=
Z_LIBS=
AC_ARG_WITH(zlib,dnl
[  --with-zlib=ARG         where ARG is the zlib directory or "no"  ],
[  MYZ="$withval"                                                   ])
AC_ARG_WITH(nozlib,dnl
[  --without-zlib          do NOT use ZLIB (same as --with-zlib=no) ],
[  MYZ="no"                                                         ])
if test "$MYZ" = "no"
then
  AC_MSG_RESULT(checking for zlib support... no)
else
  AC_MSG_RESULT(checking for zlib support... yes)
  if test "$MYZ" = "off" ; then 
    MYZ=""
  fi
fi

dnl 
dnl Check for zlib
dnl
if test "$MYZ" = "no"
then
  AC_MSG_RESULT(Encoding: gzip/compress support disabled)
else
  AC_CHECK_HEADERS($MYZ/include/zlib.h, zfound=yes, zfound=locate)
  if test $zfound = "locate"; then
     dnl the user probably misunderstood the option....
     for dir in /usr /usr/local /usr/local/ssl /usr/pkg /usr/lib/zlib /usr/include/zlib /usr/include; do
       AC_CHECK_HEADERS($dir/include/zlib.h, zfound=yes, zfound=no)
       if test $zfound = "yes" ; then
         Z_CFLAGS=""
         Z_LDFLAGS="-L$dir/lib"
         Z_INCLUDE="-I$MYZ/include/zlib -I$MYZ/include"
         Z_LIBS="-lz"
         AC_MSG_CHECKING([for ZLIB version])
         CPPFLAGS="$Z_INCLUDE"
         AC_SUBST(Z_CFLAGS)
         AC_SUBST(Z_INCLUDE)
         AC_SUBST(Z_LDFLAGS)
         AC_SUBST(Z_LIBS)
         AC_DEFINE([HAVE_ZLIB], 1, [Discovered ZLIB for gzip/compress encoding])
         break
      fi
    done
  else
    echo "found ssl in $MYZ"
    Z_CFLAGS=""
    Z_LDFLAGS="-L$MYZ/lib"
    Z_INCLUDE="-I$MYZ/include/zlib -I$MYZ/include"
    Z_LIBS="-lz"
    AC_MSG_CHECKING([for OpenSSL version])
    CPPFLAGS="$Z_INCLUDE"
    AC_SUBST(Z_CFLAGS)
    AC_SUBST(Z_INCLUDE)
    AC_SUBST(Z_LDFLAGS)
    AC_SUBST(Z_LIBS)
    AC_DEFINE(HAVE_ZLIB)
  fi
fi


AC_C_INLINE
AC_CHECK_TYPE(int8_t,   char)
AC_CHECK_TYPE(int16_t,  short)
AC_CHECK_TYPE(int32_t,  int)
AC_CHECK_TYPE(int64_t,  long long)
AC_CHECK_TYPE(uint8_t,  unsigned char)
AC_CHECK_TYPE(uint16_t, unsigned short)
AC_CHECK_TYPE(uint32_t, unsigned int)
AC_CHECK_TYPE(uint64_t, unsigned long long)
AC_CHECK_TYPE(u_int32_t,unsigned int)
AC_CHECK_TYPE(ssize_t,  int)

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h)
AC_CHECK_HEADERS(limits.h)
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(signal.h)
AC_CHECK_HEADERS(sys/socket.h)
AC_CHECK_HEADERS(sys/select.h)
AC_CHECK_HEADERS(sys/types.h)
AC_CHECK_HEADERS(sys/time.h sys/times.h)
AC_CHECK_HEADERS(sys/resource.h)
AC_CHECK_HEADERS(errno.h)
AC_CHECK_HEADERS(arpa/inet.h)
AC_CHECK_HEADERS(netinet/in.h)
AC_CHECK_HEADERS(netdb.h)
AC_CHECK_HEADERS(pthread.h) 
AC_CHECK_HEADERS(string.h) 
AC_CHECK_HEADERS(strings.h) 
AC_CHECK_HEADERS(sched.h) 
AC_CHECK_HEADERS(openssl/e_os.h)
AC_CHECK_HEADERS(openssl/e_os2.h)

dnl
dnl Checks for typedefs, structures, and compiler characteristics.
dnl
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME

dnl
dnl Return type of signal-handlers
dnl
AC_TYPE_SIGNAL

dnl
dnl Checks for library functions.
dnl
AC_FUNC_ALLOCA
AC_CHECK_FUNCS(strchr memcpy strncpy strstr strlen)
AC_CHECK_FUNCS(strncasecmp strncmp)
AC_CHECK_FUNCS(socket, , AC_CHECK_LIB(socket, socket))
AC_CHECK_FUNCS(gethostbyname, , AC_CHECK_LIB(nsl, gethostbyname))
AC_CHECK_FUNCS(snprintf)
AC_CHECK_FUNCS(strdup)
AC_CHECK_FUNCS(rand_r)
AC_CHECK_FUNCS(localtime_r)
AC_CHECK_FUNCS(gethostbyname_r)
AC_CHECK_FUNCS(gmtime_r)
AC_CHECK_FUNCS(getipnodebyname)
AC_CHECK_FUNCS(freehostent)
AC_CHECK_FUNCS(getopt_long)
AC_CHECK_FUNCS(poll)

dnl
dnl Check for socket library
dnl
AC_CHECK_LIB(socket, socket)

dnl
dnl Check for pthread support
dnl
PTHREAD_CFLAGS=error
PTHREAD_LDFLAGS=error

dnl If it's error, then the user didn't 
dnl define it.
if test "x$PTHREAD_LDFLAGS" = xerror; then
  AC_CHECK_LIB(pthread, pthread_attr_init, [
             PTHREAD_CFLAGS="-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS"
             PTHREAD_LDFLAGS="-lpthread" ])
fi

if test "x$PTHREAD_LDFLAGS" = xerror; then
  AC_CHECK_LIB(pthreads, pthread_attr_init, [
             PTHREAD_CFLAGS="-D_THREAD_SAFE"
             PTHREAD_LDFLAGS="-lpthreads" ]) 
fi

if test "x$PTHREAD_LDFLAGS" = xerror; then
   AC_CHECK_LIB(c_r, pthread_attr_init, [
                PTHREAD_CFLAGS="-D_THREAD_SAFE -pthread"
                PTHREAD_LDFLAGS="-pthread" ])
fi

if test "x$PTHREAD_LDFLAGS" = xerror; then
   AC_CHECK_FUNC(pthread_attr_init, [
                 PTHREAD_CFLAGS="-D_REENTRANT"
                 PTHREAD_LDFLAGS="-lpthread" ])
fi

if test $PTHREAD_LDFLAGS = "error"; then
  AC_MSG_WARN(pthread library NOT found: guessing and hoping for the best....)
  PTHREAD_CFLAGS="-D_REENTRANT"
  PTHREAD_LDFLAGS="-lpthread" 
fi

AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LDFLAGS)

dnl
dnl On some platforms, notably Solaris, these
dnl variables are assigned literally by the user
dnl and not implied. (don't know why...)
case "$host_os" in
  *solaris*) 
    if test -z "$ac_given_sysconfdir" ; 
    then
      sysconfdir="$prefix/etc"
    fi
 
    if test -z "$ac_given_localstatedir" ; 
    then
      localstatedir="$prefix/var"
    fi
  ;;
  *) ;;
esac

dnl 
dnl Create <joedog/joepath.h> header file
dnl
AC_OUTPUT_COMMANDS([
	outfile=include/joedog/path.h
	tmpfile=${outfile}
	cat > $tmpfile << _EOF_
/**
 * Path Header
 *
 * Copyright (C) 2000-2013 by 
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
#ifndef  JOEDOG_PATH_H
#define  JOEDOG_PATH_H

#define SIEGE_HOME     "$prefix"
#define URL_FILE       "$sysconfdir/urls.txt"
#define CNF_FILE       "$sysconfdir/siegerc"
#define LOG_FILE       "$localstatedir/log/siege.log"
#define PLATFORM       "$platform"

#endif/*JOEDOG_PATH_H*/
_EOF_
], [ prefix=$prefix sysconfdir=$sysconfdir localstatedir=$localstatedir platform=$PLATFORM ])

dnl
dnl update dates and versioning in doc
dnl
AC_OUTPUT_COMMANDS([
  for file in doc/bombardment.1 doc/siege.1 doc/siege.config.1 doc/siege2csv.1 doc/siegerc;
  do
    rm -f $file
    sed -e "s|\$_VERSION|$VERSION|g" \
        -e "s|\$_PROGRAM|$PROGRAM|g" \
        -e "s|\$_DATE|$DATE|g" \
        -e "s|\$_AUTHOR|$AUTHOR|g" \
        -e "s|\$_YEARS|$YEARS|g" \
        -e "s|\$_EMAIL|$EMAIL|g" \
        < $file.in > $file
  done
], [ VERSION=$VERSION DATE="$DATE" PROGRAM="$PROGRAM" AUTHOR="$AUTHOR" EMAIL="$EMAIL" ])

dnl
dnl create siege.config utility
dnl
AC_OUTPUT_COMMANDS([
  infile=utils/siege.config.in
  outfile=utils/siege.config
  rm -f $outfile
  sed -e "s|%_PREFIX%|$bindir|" \
      -e "s|%_SHELL%|$sh|" \
      < $infile >    $outfile
  cat doc/siegerc >> $outfile
  echo "_EOF_"    >> $outfile
  echo "echo \"New configuration template added to \$HOME/.siege\"" >> $outfile
  echo "echo \"Run siege -C to view the current settings in that file\"" >> $outfile
  echo "exit"     >> $outfile
  echo ""         >> $outfile
 
], [ bindir=$exec_prefix$bindir sh=$SHELL ]) 

dnl
dnl create bombardment utility
dnl
AC_OUTPUT_COMMANDS([
  infile=utils/bombardment.in
  outfile=utils/bombardment
  rm -f $outfile
  sed -e "s|%_PREFIX%|$bindir|" \
      -e "s|%_SHELL%|$sh|" \
      < $infile > $outfile
 
], [ bindir=$exec_prefix$bindir sh=$SHELL ]) 

dnl
dnl create siege2csv utility
dnl
AC_OUTPUT_COMMANDS([
  infile=utils/siege2csv.in 
  outfile=utils/siege2csv.pl
  rm -f $outfile
  sed -e "s|%_PREFIX%|$bindir|" \
      -e "s|%_PERL%|$LREP|" \
      < $infile > $outfile
 
], [ bindir=$exec_prefix$bindir LREP=$PERL ]) 

dnl
dnl Write platform to file for support reporting
dnl
AC_OUTPUT_COMMANDS([
	outfile=PLATFORM
	tmpfile=${outfile}
	cat > $tmpfile << _EOF_
$PLATFORM
_EOF_
], [PLATFORM=$PLATFORM])

AC_OUTPUT(Makefile src/Makefile doc/Makefile html/Makefile include/Makefile include/joedog/Makefile utils/Makefile)

echo
echo "--------------------------------------------------------"
echo "Configuration is complete"
echo
echo "Run the following commands to complete the installation:"
echo "  make "
echo "  make install"
echo
echo "For complete documentation:        http://www.joedog.org"
echo "--------------------------------------------------------" 
