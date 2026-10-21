# Check for a working shell.

# Copyright (C) 2000, 2001, 2007 Free Software Foundation, Inc.
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# AC_PROG_SHELL
# -------------
# Check for a working (i.e. POSIX-compatible) shell.
# Written by Paul Eggert <eggert@twinsun.com>,
# from an idea suggested by Albert Chin-A-Young <china@thewrittenword.com>.
AC_DEFUN([AC_PROG_SHELL],
  [AC_MSG_CHECKING(for a POSIX-compliant shell)
   AC_CACHE_VAL(ac_cv_path_shell,
     [ac_cv_path_shell=no
      IFS="${IFS=      }"; ac_save_ifs="$IFS"; IFS=":"
      ac_dummy=/bin:/usr/bin:/usr/bin/posix:/usr/xpg4/bin:$PATH
      for ac_dir in $ac_dummy; do
       for ac_base in sh bash ksh sh5; do
         case "$ac_dir" in
         /*)
           if ("$ac_dir/$ac_base" -c '

                 # Test the noclobber option,
                 # using the portable POSIX.2 syntax.
                 set -C
                 rm -f conftest.c || exit
                 >conftest.c || exit
                 >|conftest.c || exit
                 !>conftest.c || exit

               ') 2>/dev/null; then
             ac_cv_path_shell="$ac_dir/$ac_base"
             break
           fi
           ;;
         esac
       done
       if test "$ac_cv_path_shell" != no; then
         break
       fi
      done
      IFS="$ac_save_ifs"])
   AC_MSG_RESULT($ac_cv_path_shell)
   SHELL=$ac_cv_path_shell
   if test "$SHELL" = no; then
     SHELL=/bin/sh
     AC_MSG_WARN(Using $SHELL, even though it is not POSIX-compliant)
   fi
   AC_SUBST(SHELL)])
