##
## utils/Makefile.am
##
## Copyright (C) 2000-2007 by
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

AUTOMAKE_OPTIONS  =    foreign no-dependencies

WARN_CFLAGS = @WARN_CFLAGS@
AM_CFLAGS = $(WARN_CFLAGS)

SIEGE_UTILITIES   =    bombardment siege2csv.pl siege.config

DISTCLEANFILES    =    $(SIEGE_UTILITIES)
 
EXTRA_DIST        =    \
bootstrap              \
config.guess           \
config.sub             \
install-sh             \
ltmain.sh              \
mdate-sh               \
missing                \
mkinstalldirs          \
mkstamp                \
siege.config.in        \
bombardment.in         \
siege2csv.in

install-exec-hook:
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(SIEGE_UTILITIES)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(LIBTOOL)  --mode=install $(INSTALL_SCRIPT) $$p $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	    $(LIBTOOL)  --mode=install $(INSTALL_SCRIPT) $$p $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'| sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	else :; fi; \
	done 

uninstall:
	@list='$(SIEGE_UTILITIES)'; for p in $$list; do \
	rm -f $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	done 

                              
