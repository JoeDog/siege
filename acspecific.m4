# This file is part of Autoconf.                       -*- Autoconf -*-
# Macros that test for specific features.
# Copyright 1992, 1993, 1994, 1995, 1996, 1998, 1999, 2000, 2001
# Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.
#
# As a special exception, the Free Software Foundation gives unlimited
# permission to copy, distribute and modify the configure scripts that
# are the output of Autoconf.  You need not follow the terms of the GNU
# General Public License when using or distributing such scripts, even
# though portions of the text of Autoconf appear in them.  The GNU
# General Public License (GPL) does govern all other use of the material
# that constitutes the Autoconf program.
#
# Certain portions of the Autoconf source text are designed to be copied
# (in certain cases, depending on the input) into the output of
# Autoconf.  We call these the "data" portions.  The rest of the Autoconf
# source text consists of comments plus executable code that decides which
# of the data portions to output in any given case.  We call these
# comments and executable code the "non-data" portions.  Autoconf never
# copies any of the non-data portions into its output.
#
# This special exception to the GPL applies to versions of Autoconf
# released by the Free Software Foundation.  When you make and
# distribute a modified version of Autoconf, you may extend this special
# exception to the GPL to apply to your modified version as well, *unless*
# your modified version has the potential to copy into its output some
# of the text that was the non-data portion of the version that you started
# with.  (In other words, unless your change moves or copies text from
# the non-data portions to the data portions.)  If your modification has
# such potential, you must delete any notice of this special exception
# to the GPL from your modified version.
#
# Written by David MacKenzie, with help from
# Franc,ois Pinard, Karl Berry, Richard Pixley, Ian Lance Taylor,
# Roland McGrath, Noah Friedman, david d zuhn, and many others.


## --------------------- ##
## Checks for programs.  ##
## --------------------- ##


# _AC_PROG_ECHO
# -------------
# Check whether to use -n, \c, or newline-tab to separate
# checking messages from result messages.
# Don't try to cache, since the results of this macro are needed to
# display the checking message.  In addition, caching something used once
# has little interest.
# Idea borrowed from dist 3.0.  Use `*c*,', not `*c,' because if `\c'
# failed there is also a new-line to match.
m4_define([_AC_PROG_ECHO],
[case `echo "testing\c"; echo 1,2,3`,`echo -n testing; echo 1,2,3` in
  *c*,-n*) ECHO_N= ECHO_C='
' ECHO_T='	' ;;
  *c*,*  ) ECHO_N=-n ECHO_C= ECHO_T= ;;
  *)      ECHO_N= ECHO_C='\c' ECHO_T= ;;
esac
AC_SUBST(ECHO_C)dnl
AC_SUBST(ECHO_N)dnl
AC_SUBST(ECHO_T)dnl
])# _AC_PROG_ECHO


# AC_PROG_MAKE_SET
# ----------------
# Define SET_MAKE to set ${MAKE} if make doesn't.
AC_DEFUN([AC_PROG_MAKE_SET],
[AC_MSG_CHECKING([whether ${MAKE-make} sets \${MAKE}])
set dummy ${MAKE-make}; ac_make=`echo "$[2]" | sed 'y,./+-,__p_,'`
AC_CACHE_VAL(ac_cv_prog_make_${ac_make}_set,
[cat >conftest.make <<\EOF
all:
	@echo 'ac_maketemp="${MAKE}"'
EOF
# GNU make sometimes prints "make[1]: Entering...", which would confuse us.
eval `${MAKE-make} -f conftest.make 2>/dev/null | grep temp=`
if test -n "$ac_maketemp"; then
  eval ac_cv_prog_make_${ac_make}_set=yes
else
  eval ac_cv_prog_make_${ac_make}_set=no
fi
rm -f conftest.make])dnl
if eval "test \"`echo '$ac_cv_prog_make_'${ac_make}_set`\" = yes"; then
  AC_MSG_RESULT([yes])
  SET_MAKE=
else
  AC_MSG_RESULT([no])
  SET_MAKE="MAKE=${MAKE-make}"
fi
AC_SUBST([SET_MAKE])dnl
])# AC_PROG_MAKE_SET


# AC_PROG_RANLIB
# --------------
AC_DEFUN([AC_PROG_RANLIB],
[AC_CHECK_TOOL(RANLIB, ranlib, :)])


# Check for mawk first since it's generally faster.
AC_DEFUN([AC_PROG_AWK],
[AC_CHECK_PROGS(AWK, mawk gawk nawk awk, )])


# AC_PROG_YACC
# ------------
AC_DEFUN([AC_PROG_YACC],
[AC_CHECK_PROGS(YACC, 'bison -y' byacc, yacc)])


# AC_PROG_LEX
# -----------
# Look for flex or lex.  Set its associated library to LEXLIB.
# Check if lex declares yytext as a char * by default, not a char[].
AC_DEFUN_ONCE([AC_PROG_LEX],
[AC_CHECK_PROGS(LEX, flex lex, :)
if test -z "$LEXLIB"
then
  AC_CHECK_LIB(fl, yywrap, LEXLIB="-lfl",
    [AC_CHECK_LIB(l, yywrap, LEXLIB="-ll")])
fi
AC_SUBST(LEXLIB)
if test "x$LEX" != "x:"; then
  _AC_DECL_YYTEXT
fi])


# _AC_DECL_YYTEXT
# ---------------
# Check if lex declares yytext as a char * by default, not a char[].
m4_define([_AC_DECL_YYTEXT],
[AC_CACHE_CHECK(lex output file root, ac_cv_prog_lex_root,
[# The minimal lex program is just a single line: %%.  But some broken lexes
# (Solaris, I think it was) want two %% lines, so accommodate them.
echo '%%
%%' | $LEX
if test -f lex.yy.c; then
  ac_cv_prog_lex_root=lex.yy
elif test -f lexyy.c; then
  ac_cv_prog_lex_root=lexyy
else
  AC_MSG_ERROR([cannot find output from $LEX; giving up])
fi])
LEX_OUTPUT_ROOT=$ac_cv_prog_lex_root
AC_SUBST(LEX_OUTPUT_ROOT)dnl

AC_CACHE_CHECK(whether yytext is a pointer, ac_cv_prog_lex_yytext_pointer,
[# POSIX says lex can declare yytext either as a pointer or an array; the
# default is implementation-dependent. Figure out which it is, since
# not all implementations provide the %pointer and %array declarations.
ac_cv_prog_lex_yytext_pointer=no
echo 'extern char *yytext;' >>$LEX_OUTPUT_ROOT.c
ac_save_LIBS=$LIBS
LIBS="$LIBS $LEXLIB"
AC_LINK_IFELSE([`cat $LEX_OUTPUT_ROOT.c`], ac_cv_prog_lex_yytext_pointer=yes)
LIBS=$ac_save_LIBS
rm -f "${LEX_OUTPUT_ROOT}.c"
])
dnl
if test $ac_cv_prog_lex_yytext_pointer = yes; then
  AC_DEFINE(YYTEXT_POINTER, 1,
            [Define if `lex' declares `yytext' as a `char *' by default,
             not a `char[]'.])
fi
])# _AC_DECL_YYTEXT


# Require AC_PROG_LEX in case some people were just calling this macro.
AU_DEFUN([AC_DECL_YYTEXT],
[AC_PROG_LEX])


# AC_PROG_INSTALL
# ---------------
AC_DEFUN([AC_PROG_INSTALL],
[AC_REQUIRE([AC_CONFIG_AUX_DIR_DEFAULT])dnl
# Find a good install program.  We prefer a C program (faster),
# so one script is as good as another.  But avoid the broken or
# incompatible versions:
# SysV /etc/install, /usr/sbin/install
# SunOS /usr/etc/install
# IRIX /sbin/install
# AIX /bin/install
# AmigaOS /C/install, which installs bootblocks on floppy discs
# AIX 4 /usr/bin/installbsd, which doesn't work without a -g flag
# AFS /usr/afsws/bin/install, which mishandles nonexistent args
# SVR4 /usr/ucb/install, which tries to use the nonexistent group "staff"
# ./install, which can be erroneously created by make from ./install.sh.
AC_MSG_CHECKING([for a BSD compatible install])
if test -z "$INSTALL"; then
AC_CACHE_VAL(ac_cv_path_install,
[  ac_save_IFS=$IFS; IFS=$ac_path_separator
  for ac_dir in $PATH; do
    IFS=$ac_save_IFS
    # Account for people who put trailing slashes in PATH elements.
    case $ac_dir/ in
    / | ./ | .// | /[cC]/* \
    | /etc/* | /usr/sbin/* | /usr/etc/* | /sbin/* | /usr/afsws/bin/* \
    | /usr/ucb/* ) ;;
    *)
      # OSF1 and SCO ODT 3.0 have their own names for install.
      # Don't use installbsd from OSF since it installs stuff as root
      # by default.
      for ac_prog in ginstall scoinst install; do
        if AS_EXECUTABLE_P(["$ac_dir/$ac_prog"]); then
	  if test $ac_prog = install &&
            grep dspmsg "$ac_dir/$ac_prog" >/dev/null 2>&1; then
	    # AIX install.  It has an incompatible calling convention.
	    :
	  elif test $ac_prog = install &&
	    grep pwplus "$ac_dir/$ac_prog" >/dev/null 2>&1; then
	    # program-specific install script used by HP pwplus--don't use.
	    :
	  else
	    ac_cv_path_install="$ac_dir/$ac_prog -c"
	    break 2
	  fi
	fi
      done
      ;;
    esac
  done
])dnl
  if test "${ac_cv_path_install+set}" = set; then
    INSTALL=$ac_cv_path_install
  else
    # As a last resort, use the slow shell script.  We don't cache a
    # path for INSTALL within a source directory, because that will
    # break other packages using the cache if that directory is
    # removed, or if the path is relative.
    INSTALL=$ac_install_sh
  fi
fi
dnl We do special magic for INSTALL instead of AC_SUBST, to get
dnl relative paths right.
AC_MSG_RESULT([$INSTALL])

# Use test -z because SunOS4 sh mishandles braces in ${var-val}.
# It thinks the first close brace ends the variable substitution.
test -z "$INSTALL_PROGRAM" && INSTALL_PROGRAM='${INSTALL}'
AC_SUBST(INSTALL_PROGRAM)dnl

test -z "$INSTALL_SCRIPT" && INSTALL_SCRIPT='${INSTALL}'
AC_SUBST(INSTALL_SCRIPT)dnl

test -z "$INSTALL_DATA" && INSTALL_DATA='${INSTALL} -m 644'
AC_SUBST(INSTALL_DATA)dnl
])# AC_PROG_INSTALL


# AC_PROG_LN_S
# ------------
AC_DEFUN([AC_PROG_LN_S],
[AC_MSG_CHECKING([whether ln -s works])
AC_SUBST([LN_S], [$as_ln_s])dnl
if test "$LN_S" = "ln -s"; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no, using $LN_S])
fi
])# AC_PROG_LN_S


# AC_RSH
# ------
# I don't know what it used to do, but it no longer does.
AU_DEFUN([AC_RSH],
[AC_DIAGNOSE([obsolete], [$0: is no longer supported.
Remove this warning when you adjust the code.])])



## ------------------------- ##
## Checks for declarations.  ##
## ------------------------- ##


# AC_DECL_SYS_SIGLIST
# -------------------
AC_DEFUN([AC_DECL_SYS_SIGLIST],
[AC_CACHE_CHECK([for sys_siglist declaration in signal.h or unistd.h],
  ac_cv_decl_sys_siglist,
[AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([#include <sys/types.h>
#include <signal.h>
/* NetBSD declares sys_siglist in unistd.h.  */
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
], [char *msg = *(sys_siglist + 1);])],
                   [ac_cv_decl_sys_siglist=yes],
                   [ac_cv_decl_sys_siglist=no])])
if test $ac_cv_decl_sys_siglist = yes; then
  AC_DEFINE(SYS_SIGLIST_DECLARED, 1,
            [Define if `sys_siglist' is declared by <signal.h> or <unistd.h>.])
fi
])# AC_DECL_SYS_SIGLIST




## -------------------------------------- ##
## Checks for operating system services.  ##
## -------------------------------------- ##


# AC_SYS_INTERPRETER
# ------------------
AC_DEFUN([AC_SYS_INTERPRETER],
[AC_CACHE_CHECK(whether @%:@! works in shell scripts, ac_cv_sys_interpreter,
[echo '#! /bin/cat
exit 69
' >conftest
chmod u+x conftest
(SHELL=/bin/sh; export SHELL; ./conftest >/dev/null)
if test $? -ne 69; then
   ac_cv_sys_interpreter=yes
else
   ac_cv_sys_interpreter=no
fi
rm -f conftest])
interpval=$ac_cv_sys_interpreter
])


AU_DEFUN([AC_HAVE_POUNDBANG],
[AC_SYS_INTERPRETER
AC_DIAGNOSE([obsolete],
[$0: Remove this warning when you adjust your code to use
      `AC_SYS_INTERPRETER'.])])


AU_DEFUN([AC_ARG_ARRAY],
[AC_DIAGNOSE([obsolete],
[$0: no longer implemented: don't do unportable things
with arguments. Remove this warning when you adjust your code.])])


# _AC_SYS_LARGEFILE_TEST_INCLUDES
# -------------------------------
m4_define([_AC_SYS_LARGEFILE_TEST_INCLUDES],
[@%:@include <sys/types.h>
 /* Check that off_t can represent 2**63 - 1 correctly.
    We can't simply define LARGE_OFF_T to be 9223372036854775807,
    since some C++ compilers masquerading as C compilers
    incorrectly reject 9223372036854775807.  */
@%:@define LARGE_OFF_T (((off_t) 1 << 62) - 1 + ((off_t) 1 << 62))
  int off_t_is_large[[(LARGE_OFF_T % 2147483629 == 721
		       && LARGE_OFF_T % 2147483647 == 1)
		      ? 1 : -1]];[]dnl
])


# _AC_SYS_LARGEFILE_MACRO_VALUE(C-MACRO, VALUE,
#                               CACHE-VAR,
#                               DESCRIPTION,
#                               [INCLUDES], [FUNCTION-BODY])
# ----------------------------------------------------------
m4_define([_AC_SYS_LARGEFILE_MACRO_VALUE],
[AC_CACHE_CHECK([for $1 value needed for large files], [$3],
[while :; do
  $3=no
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([$5], [$6])],
  		    [break])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([@%:@define $1 $2
$5], [$6])],
  		    [$3=$2; break])
  break
done])
if test "$$3" != no; then
  AC_DEFINE_UNQUOTED([$1], [$$3], [$4])
fi
rm -f conftest*[]dnl
])# _AC_SYS_LARGEFILE_MACRO_VALUE


# AC_SYS_LARGEFILE
# ----------------
# By default, many hosts won't let programs access large files;
# one must use special compiler options to get large-file access to work.
# For more details about this brain damage please see:
# http://www.sas.com/standards/large.file/x_open.20Mar96.html
AC_DEFUN([AC_SYS_LARGEFILE],
[AC_ARG_ENABLE(largefile,
               [  --disable-largefile     omit support for large files])
if test "$enable_largefile" != no; then

  AC_CACHE_CHECK([for special C compiler options needed for large files],
    ac_cv_sys_largefile_CC,
    [ac_cv_sys_largefile_CC=no
     if test "$GCC" != yes; then
       ac_save_CC=$CC
       while :; do
     	 # IRIX 6.2 and later do not support large files by default,
     	 # so use the C compiler's -n32 option if that helps.
         AC_LANG_CONFTEST([AC_LANG_PROGRAM([_AC_SYS_LARGEFILE_TEST_INCLUDES])])
     	 AC_COMPILE_IFELSE([], [break])
     	 CC="$CC -n32"
     	 AC_COMPILE_IFELSE([], [ac_cv_sys_largefile_CC=' -n32'; break])
         break
       done
       CC=$ac_save_CC
       rm -f conftest.$ac_ext
    fi])
  if test "$ac_cv_sys_largefile_CC" != no; then
    CC=$CC$ac_cv_sys_largefile_CC
  fi

  _AC_SYS_LARGEFILE_MACRO_VALUE(_FILE_OFFSET_BITS, 64,
    ac_cv_sys_file_offset_bits,
    [Number of bits in a file offset, on hosts where this is settable.],
    [_AC_SYS_LARGEFILE_TEST_INCLUDES])
  _AC_SYS_LARGEFILE_MACRO_VALUE(_LARGE_FILES, 1,
    ac_cv_sys_large_files,
    [Define for large files, on AIX-style hosts.],
    [_AC_SYS_LARGEFILE_TEST_INCLUDES])
fi
])# AC_SYS_LARGEFILE


# AC_SYS_LONG_FILE_NAMES
# ----------------------
# Security: use a temporary directory as the most portable way of
# creating files in /tmp securely.  Removing them leaves a race
# condition, set -C is not portably guaranteed to use O_EXCL, so still
# leaves a race, and not all systems have the `mktemp' utility.  We
# still test for existence first in case of broken systems where the
# mkdir succeeds even when the directory exists.  Broken systems may
# retain a race, but they probably have other security problems
# anyway; this should be secure on well-behaved systems.  In any case,
# use of `mktemp' is probably inappropriate here since it would fail in
# attempting to create different file names differing after the 14th
# character on file systems without long file names.
AC_DEFUN([AC_SYS_LONG_FILE_NAMES],
[AC_CACHE_CHECK(for long file names, ac_cv_sys_long_file_names,
[ac_cv_sys_long_file_names=yes
# Test for long file names in all the places we know might matter:
#      .		the current directory, where building will happen
#      $prefix/lib	where we will be installing things
#      $exec_prefix/lib	likewise
# eval it to expand exec_prefix.
#      $TMPDIR		if set, where it might want to write temporary files
# if $TMPDIR is not set:
#      /tmp		where it might want to write temporary files
#      /var/tmp		likewise
#      /usr/tmp		likewise
if test -n "$TMPDIR" && test -d "$TMPDIR" && test -w "$TMPDIR"; then
  ac_tmpdirs=$TMPDIR
else
  ac_tmpdirs='/tmp /var/tmp /usr/tmp'
fi
for ac_dir in  . $ac_tmpdirs `eval echo $prefix/lib $exec_prefix/lib` ; do
  test -d $ac_dir || continue
  test -w $ac_dir || continue # It is less confusing to not echo anything here.
  ac_xdir=$ac_dir/cf$$
  (umask 077 && mkdir $ac_xdir 2>/dev/null) || continue
  ac_tf1=$ac_xdir/conftest9012345
  ac_tf2=$ac_xdir/conftest9012346
  (echo 1 >$ac_tf1) 2>/dev/null
  (echo 2 >$ac_tf2) 2>/dev/null
  ac_val=`cat $ac_tf1 2>/dev/null`
  if test ! -f $ac_tf1 || test "$ac_val" != 1; then
    ac_cv_sys_long_file_names=no
    rm -rf $ac_xdir 2>/dev/null
    break
  fi
  rm -rf $ac_xdir 2>/dev/null
done])
if test $ac_cv_sys_long_file_names = yes; then
  AC_DEFINE(HAVE_LONG_FILE_NAMES, 1,
            [Define if you support file names longer than 14 characters.])
fi
])


# AC_SYS_RESTARTABLE_SYSCALLS
# ---------------------------
# If the system automatically restarts a system call that is
# interrupted by a signal, define `HAVE_RESTARTABLE_SYSCALLS'.
AC_DEFUN([AC_SYS_RESTARTABLE_SYSCALLS],
[AC_DIAGNOSE([obsolete],
[$0: System call restartability is now typically set at runtime.
Remove this `AC_SYS_RESTARTABLE_SYSCALLS'
and adjust your code to use `sigaction' with `SA_RESTART' instead.])dnl
AC_REQUIRE([AC_HEADER_SYS_WAIT])dnl
AC_CHECK_HEADERS(unistd.h)
AC_CACHE_CHECK(for restartable system calls, ac_cv_sys_restartable_syscalls,
[AC_RUN_IFELSE([AC_LANG_SOURCE(
[/* Exit 0 (true) if wait returns something other than -1,
   i.e. the pid of the child, which means that wait was restarted
   after getting the signal.  */

#include <sys/types.h>
#include <signal.h>
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

/* Some platforms explicitly require an extern "C" signal handler
   when using C++. */
#ifdef __cplusplus
extern "C" void ucatch (int dummy) { }
#else
void ucatch (dummy) int dummy; { }
#endif

int
main ()
{
  int i = fork (), status;

  if (i == 0)
    {
      sleep (3);
      kill (getppid (), SIGINT);
      sleep (3);
      exit (0);
    }

  signal (SIGINT, ucatch);

  status = wait (&i);
  if (status == -1)
    wait (&i);

  exit (status == -1);
}])],
               [ac_cv_sys_restartable_syscalls=yes],
               [ac_cv_sys_restartable_syscalls=no])])
if test $ac_cv_sys_restartable_syscalls = yes; then
  AC_DEFINE(HAVE_RESTARTABLE_SYSCALLS, 1,
            [Define if system calls automatically restart after interruption
             by a signal.])
fi
])# AC_SYS_RESTARTABLE_SYSCALLS


# AC_SYS_POSIX_TERMIOS
# --------------------
AC_DEFUN([AC_SYS_POSIX_TERMIOS],
[AC_CACHE_CHECK([POSIX termios], ac_cv_sys_posix_termios,
[AC_TRY_LINK([#include <sys/types.h>
#include <unistd.h>
@%:@include <termios.h>],
             [/* SunOS 4.0.3 has termios.h but not the library calls.  */
   tcgetattr(0, 0);],
             ac_cv_sys_posix_termios=yes,
             ac_cv_sys_posix_termios=no)])
])# AC_SYS_POSIX_TERMIOS



## --------------------- ##
## Checks for X window.  ##
## --------------------- ##


# _AC_PATH_X_XMKMF
# ----------------
# Internal subroutine of _AC_PATH_X.
# Set ac_x_includes and/or ac_x_libraries.
m4_define([_AC_PATH_X_XMKMF],
[rm -fr conftest.dir
if mkdir conftest.dir; then
  cd conftest.dir
  # Make sure to not put "make" in the Imakefile rules, since we grep it out.
  cat >Imakefile <<'EOF'
acfindx:
	@echo 'ac_im_incroot="${INCROOT}"; ac_im_usrlibdir="${USRLIBDIR}"; ac_im_libdir="${LIBDIR}"'
EOF
  if (xmkmf) >/dev/null 2>/dev/null && test -f Makefile; then
    # GNU make sometimes prints "make[1]: Entering...", which would confuse us.
    eval `${MAKE-make} acfindx 2>/dev/null | grep -v make`
    # Open Windows xmkmf reportedly sets LIBDIR instead of USRLIBDIR.
    for ac_extension in a so sl; do
      if test ! -f $ac_im_usrlibdir/libX11.$ac_extension &&
         test -f $ac_im_libdir/libX11.$ac_extension; then
        ac_im_usrlibdir=$ac_im_libdir; break
      fi
    done
    # Screen out bogus values from the imake configuration.  They are
    # bogus both because they are the default anyway, and because
    # using them would break gcc on systems where it needs fixed includes.
    case $ac_im_incroot in
	/usr/include) ;;
	*) test -f "$ac_im_incroot/X11/Xos.h" && ac_x_includes=$ac_im_incroot;;
    esac
    case $ac_im_usrlibdir in
	/usr/lib | /lib) ;;
	*) test -d "$ac_im_usrlibdir" && ac_x_libraries=$ac_im_usrlibdir ;;
    esac
  fi
  cd ..
  rm -fr conftest.dir
fi
])# _AC_PATH_X_XMKMF


# _AC_PATH_X_DIRECT
# -----------------
# Internal subroutine of _AC_PATH_X.
# Set ac_x_includes and/or ac_x_libraries.
m4_define([_AC_PATH_X_DIRECT],
[# Standard set of common directories for X headers.
# Check X11 before X11Rn because it is often a symlink to the current release.
ac_x_header_dirs='
/usr/X11/include
/usr/X11R6/include
/usr/X11R5/include
/usr/X11R4/include

/usr/include/X11
/usr/include/X11R6
/usr/include/X11R5
/usr/include/X11R4

/usr/local/X11/include
/usr/local/X11R6/include
/usr/local/X11R5/include
/usr/local/X11R4/include

/usr/local/include/X11
/usr/local/include/X11R6
/usr/local/include/X11R5
/usr/local/include/X11R4

/usr/X386/include
/usr/x386/include
/usr/XFree86/include/X11

/usr/include
/usr/local/include
/usr/unsupported/include
/usr/athena/include
/usr/local/x11r5/include
/usr/lpp/Xamples/include

/usr/openwin/include
/usr/openwin/share/include'

if test "$ac_x_includes" = no; then
  # Guess where to find include files, by looking for Intrinsic.h.
  # First, try using that file with no special directory specified.
  AC_PREPROC_IFELSE([AC_LANG_SOURCE([@%:@include <X11/Intrinsic.h>])],
[# We can compile using X headers with no special include directory.
ac_x_includes=],
[for ac_dir in $ac_x_header_dirs; do
  if test -r "$ac_dir/X11/Intrinsic.h"; then
    ac_x_includes=$ac_dir
    break
  fi
done])
fi # $ac_x_includes = no

if test "$ac_x_libraries" = no; then
  # Check for the libraries.
  # See if we find them without any special options.
  # Don't add to $LIBS permanently.
  ac_save_LIBS=$LIBS
  LIBS="-lXt $LIBS"
  AC_TRY_LINK([@%:@include <X11/Intrinsic.h>], [XtMalloc (0)],
[LIBS=$ac_save_LIBS
# We can link X programs with no special library path.
ac_x_libraries=],
[LIBS=$ac_save_LIBS
for ac_dir in `echo "$ac_x_includes $ac_x_header_dirs" | sed s/include/lib/g`
do
  # Don't even attempt the hair of trying to link an X program!
  for ac_extension in a so sl; do
    if test -r $ac_dir/libXt.$ac_extension; then
      ac_x_libraries=$ac_dir
      break 2
    fi
  done
done])
fi # $ac_x_libraries = no
])# _AC_PATH_X_DIRECT


# _AC_PATH_X
# ----------
# Compute ac_cv_have_x.
AC_DEFUN([_AC_PATH_X],
[AC_CACHE_VAL(ac_cv_have_x,
[# One or both of the vars are not set, and there is no cached value.
ac_x_includes=no ac_x_libraries=no
_AC_PATH_X_XMKMF
_AC_PATH_X_DIRECT
if test "$ac_x_includes" = no || test "$ac_x_libraries" = no; then
  # Didn't find X anywhere.  Cache the known absence of X.
  ac_cv_have_x="have_x=no"
else
  # Record where we found X for the cache.
  ac_cv_have_x="have_x=yes \
	        ac_x_includes=$ac_x_includes ac_x_libraries=$ac_x_libraries"
fi])dnl
])


# AC_PATH_X
# ---------
# If we find X, set shell vars x_includes and x_libraries to the
# paths, otherwise set no_x=yes.
# Uses ac_ vars as temps to allow command line to override cache and checks.
# --without-x overrides everything else, but does not touch the cache.
AC_DEFUN([AC_PATH_X],
[dnl Document the X abnormal options inherited from history.
m4_divert_once([HELP_BEGIN], [
X features:
  --x-includes=DIR    X include files are in DIR
  --x-libraries=DIR   X library files are in DIR])dnl
AC_MSG_CHECKING([for X])

AC_ARG_WITH(x, [  --with-x                use the X Window System])
# $have_x is `yes', `no', `disabled', or empty when we do not yet know.
if test "x$with_x" = xno; then
  # The user explicitly disabled X.
  have_x=disabled
else
  if test "x$x_includes" != xNONE && test "x$x_libraries" != xNONE; then
    # Both variables are already set.
    have_x=yes
  else
    _AC_PATH_X
  fi
  eval "$ac_cv_have_x"
fi # $with_x != no

if test "$have_x" != yes; then
  AC_MSG_RESULT([$have_x])
  no_x=yes
else
  # If each of the values was on the command line, it overrides each guess.
  test "x$x_includes" = xNONE && x_includes=$ac_x_includes
  test "x$x_libraries" = xNONE && x_libraries=$ac_x_libraries
  # Update the cache value to reflect the command line values.
  ac_cv_have_x="have_x=yes \
		ac_x_includes=$x_includes ac_x_libraries=$x_libraries"
  AC_MSG_RESULT([libraries $x_libraries, headers $x_includes])
fi
])# AC_PATH_X



# AC_PATH_XTRA
# ------------
# Find additional X libraries, magic flags, etc.
AC_DEFUN([AC_PATH_XTRA],
[AC_REQUIRE([AC_PATH_X])dnl
if test "$no_x" = yes; then
  # Not all programs may use this symbol, but it does not hurt to define it.
  AC_DEFINE([X_DISPLAY_MISSING], 1,
            [Define if the X Window System is missing or not being used.])
  X_CFLAGS= X_PRE_LIBS= X_LIBS= X_EXTRA_LIBS=
else
  if test -n "$x_includes"; then
    X_CFLAGS="$X_CFLAGS -I$x_includes"
  fi

  # It would also be nice to do this for all -L options, not just this one.
  if test -n "$x_libraries"; then
    X_LIBS="$X_LIBS -L$x_libraries"
dnl FIXME: banish uname from this macro!
    # For Solaris; some versions of Sun CC require a space after -R and
    # others require no space.  Words are not sufficient . . . .
    case `(uname -sr) 2>/dev/null` in
    "SunOS 5"*)
      AC_MSG_CHECKING([whether -R must be followed by a space])
      ac_xsave_LIBS=$LIBS; LIBS="$LIBS -R$x_libraries"
      AC_LINK_IFELSE([AC_LANG_PROGRAM()], ac_R_nospace=yes, ac_R_nospace=no)
      if test $ac_R_nospace = yes; then
	AC_MSG_RESULT([no])
	X_LIBS="$X_LIBS -R$x_libraries"
      else
	LIBS="$ac_xsave_LIBS -R $x_libraries"
	AC_LINK_IFELSE([AC_LANG_PROGRAM()], ac_R_space=yes, ac_R_space=no)
	if test $ac_R_space = yes; then
	  AC_MSG_RESULT([yes])
	  X_LIBS="$X_LIBS -R $x_libraries"
	else
	  AC_MSG_RESULT([neither works])
	fi
      fi
      LIBS=$ac_xsave_LIBS
    esac
  fi

  # Check for system-dependent libraries X programs must link with.
  # Do this before checking for the system-independent R6 libraries
  # (-lICE), since we may need -lsocket or whatever for X linking.

  if test "$ISC" = yes; then
    X_EXTRA_LIBS="$X_EXTRA_LIBS -lnsl_s -linet"
  else
    # Martyn Johnson says this is needed for Ultrix, if the X
    # libraries were built with DECnet support.  And Karl Berry says
    # the Alpha needs dnet_stub (dnet does not exist).
    ac_xsave_LIBS="$LIBS"; LIBS="$LIBS $X_LIBS -lX11"
    AC_TRY_LINK_FUNC(XOpenDisplay, ,
    [AC_CHECK_LIB(dnet, dnet_ntoa, [X_EXTRA_LIBS="$X_EXTRA_LIBS -ldnet"])
    if test $ac_cv_lib_dnet_dnet_ntoa = no; then
      AC_CHECK_LIB(dnet_stub, dnet_ntoa,
	[X_EXTRA_LIBS="$X_EXTRA_LIBS -ldnet_stub"])
    fi])
    LIBS="$ac_xsave_LIBS"

    # msh@cis.ufl.edu says -lnsl (and -lsocket) are needed for his 386/AT,
    # to get the SysV transport functions.
    # Chad R. Larson says the Pyramis MIS-ES running DC/OSx (SVR4)
    # needs -lnsl.
    # The nsl library prevents programs from opening the X display
    # on Irix 5.2, according to T.E. Dickey.
    # The functions gethostbyname, getservbyname, and inet_addr are
    # in -lbsd on LynxOS 3.0.1/i386, according to Lars Hecking.
    AC_CHECK_FUNC(gethostbyname)
    if test $ac_cv_func_gethostbyname = no; then
      AC_CHECK_LIB(nsl, gethostbyname, X_EXTRA_LIBS="$X_EXTRA_LIBS -lnsl")
      if test $ac_cv_lib_nsl_gethostbyname = no; then
        AC_CHECK_LIB(bsd, gethostbyname, X_EXTRA_LIBS="$X_EXTRA_LIBS -lbsd")
      fi
    fi

    # lieder@skyler.mavd.honeywell.com says without -lsocket,
    # socket/setsockopt and other routines are undefined under SCO ODT
    # 2.0.  But -lsocket is broken on IRIX 5.2 (and is not necessary
    # on later versions), says Simon Leinen: it contains gethostby*
    # variants that don't use the nameserver (or something).  -lsocket
    # must be given before -lnsl if both are needed.  We assume that
    # if connect needs -lnsl, so does gethostbyname.
    AC_CHECK_FUNC(connect)
    if test $ac_cv_func_connect = no; then
      AC_CHECK_LIB(socket, connect, X_EXTRA_LIBS="-lsocket $X_EXTRA_LIBS", ,
	$X_EXTRA_LIBS)
    fi

    # Guillermo Gomez says -lposix is necessary on A/UX.
    AC_CHECK_FUNC(remove)
    if test $ac_cv_func_remove = no; then
      AC_CHECK_LIB(posix, remove, X_EXTRA_LIBS="$X_EXTRA_LIBS -lposix")
    fi

    # BSDI BSD/OS 2.1 needs -lipc for XOpenDisplay.
    AC_CHECK_FUNC(shmat)
    if test $ac_cv_func_shmat = no; then
      AC_CHECK_LIB(ipc, shmat, X_EXTRA_LIBS="$X_EXTRA_LIBS -lipc")
    fi
  fi

  # Check for libraries that X11R6 Xt/Xaw programs need.
  ac_save_LDFLAGS=$LDFLAGS
  test -n "$x_libraries" && LDFLAGS="$LDFLAGS -L$x_libraries"
  # SM needs ICE to (dynamically) link under SunOS 4.x (so we have to
  # check for ICE first), but we must link in the order -lSM -lICE or
  # we get undefined symbols.  So assume we have SM if we have ICE.
  # These have to be linked with before -lX11, unlike the other
  # libraries we check for below, so use a different variable.
  # John Interrante, Karl Berry
  AC_CHECK_LIB(ICE, IceConnectionNumber,
    [X_PRE_LIBS="$X_PRE_LIBS -lSM -lICE"], , $X_EXTRA_LIBS)
  LDFLAGS=$ac_save_LDFLAGS

fi
AC_SUBST(X_CFLAGS)dnl
AC_SUBST(X_PRE_LIBS)dnl
AC_SUBST(X_LIBS)dnl
AC_SUBST(X_EXTRA_LIBS)dnl
])# AC_PATH_XTRA



## ------------------------------------ ##
## Checks for not-quite-Unix variants.  ##
## ------------------------------------ ##


# AC_CYGWIN
# ---------
# Check for Cygwin.  This is a way to set the right value for
# EXEEXT.
AU_DEFUN([AC_CYGWIN],
[AC_REQUIRE([AC_CANONICAL_HOST])[]dnl
AC_DIAGNOSE([obsolete],
            [$0 is obsolete: use AC_CANONICAL_HOST and $host_os])dnl
case $host_os in
  *cygwin* ) CYGWIN=yes;;
         * ) CYGWIN=no;;
esac
])# AC_CYGWIN


# AC_EMXOS2
# ---------
# Check for EMX on OS/2.  This is another way to set the right value
# for EXEEXT.
AU_DEFUN([AC_EMXOS2],
[AC_REQUIRE([AC_CANONICAL_HOST])[]dnl
AC_DIAGNOSE([obsolete],
            [$0 is obsolete: use AC_CANONICAL_HOST and $host_os])dnl
case $host_os in
  *emx* ) EMXOS2=yes;;
      * ) EMXOS2=no;;
esac
])# AC_EMXOS2


# AC_MINGW32
# ----------
# Check for mingw32.  This is another way to set the right value for
# EXEEXT.
AU_DEFUN([AC_MINGW32],
[AC_REQUIRE([AC_CANONICAL_HOST])[]dnl
AC_DIAGNOSE([obsolete],
            [$0 is obsolete: use AC_CANONICAL_HOST and $host_os])dnl
case $host_os in
  *mingw32* ) MINGW32=yes;;
          * ) MINGW32=no;;
esac
])# AC_MINGW32




## -------------------------- ##
## Checks for UNIX variants.  ##
## -------------------------- ##


# These are kludges which should be replaced by a single POSIX check.
# They aren't cached, to discourage their use.

# AC_AIX
# ------
AC_DEFUN([AC_AIX],
[AH_VERBATIM([_ALL_SOURCE],
[/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
@%:@ifndef _ALL_SOURCE
@%:@ undef _ALL_SOURCE
@%:@endif])dnl
AC_BEFORE([$0], [AC_COMPILE_IFELSE])dnl
AC_BEFORE([$0], [AC_RUN_IFELSE])dnl
AC_MSG_CHECKING([for AIX])
AC_EGREP_CPP(yes,
[#ifdef _AIX
  yes
#endif
],
[AC_MSG_RESULT([yes])
AC_DEFINE(_ALL_SOURCE)],
[AC_MSG_RESULT([no])])
])# AC_AIX


# AC_MINIX
# --------
AC_DEFUN([AC_MINIX],
[AC_BEFORE([$0], [AC_COMPILE_IFELSE])dnl
AC_BEFORE([$0], [AC_RUN_IFELSE])dnl
AC_CHECK_HEADER(minix/config.h, MINIX=yes, MINIX=)
if test "$MINIX" = yes; then
  AC_DEFINE(_POSIX_SOURCE, 1,
            [Define if you need to in order for `stat' and other things to
             work.])
  AC_DEFINE(_POSIX_1_SOURCE, 2,
            [Define if the system does not provide POSIX.1 features except
             with this defined.])
  AC_DEFINE(_MINIX, 1,
            [Define if on MINIX.])
fi
])# AC_MINIX


# AC_ISC_POSIX
# ------------
AC_DEFUN([AC_ISC_POSIX],
[AC_REQUIRE([AC_PROG_CC])dnl
AC_BEFORE([$0], [AC_COMPILE_IFELSE])dnl
AC_BEFORE([$0], [AC_RUN_IFELSE])dnl
AC_MSG_CHECKING([for POSIXized ISC])
if test -d /etc/conf/kconfig.d &&
   grep _POSIX_VERSION [/usr/include/sys/unistd.h] >/dev/null 2>&1
then
  AC_MSG_RESULT([yes])
  ISC=yes # If later tests want to check for ISC.
  AC_DEFINE(_POSIX_SOURCE, 1,
            [Define if you need to in order for stat and other things to
             work.])
  if test "$GCC" = yes; then
    CC="$CC -posix"
  else
    CC="$CC -Xp"
  fi
else
  AC_MSG_RESULT([no])
  ISC=
fi
])# AC_ISC_POSIX


# AC_XENIX_DIR
# ------------
AU_DEFUN(AC_XENIX_DIR,
[# You shouldn't need to depend upon XENIX.  Remove this test if useless.
AC_MSG_CHECKING([for Xenix])
AC_EGREP_CPP(yes,
[#if defined(M_XENIX) && !defined(M_UNIX)
  yes
@%:@endif],
             [AC_MSG_RESULT([yes]); XENIX=yes],
             [AC_MSG_RESULT([no]); XENIX=])

AC_HEADER_DIRENT[]dnl
])


# AC_DYNIX_SEQ
# ------------
AU_DEFUN([AC_DYNIX_SEQ], [AC_FUNC_GETMNTENT])


# AC_IRIX_SUN
# -----------
AU_DEFUN([AC_IRIX_SUN],
[AC_FUNC_GETMNTENT
AC_CHECK_LIB(sun, getpwnam)])


# AC_SCO_INTL
# -----------
AU_DEFUN([AC_SCO_INTL], [AC_FUNC_STRFTIME])
