##
## doc/Makefile.am
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
## You should have received a copy of the GNU General Public License along
## with this program; if not, write to the Free Software Foundation, Inc.
## 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
##

AUTOMAKE_OPTIONS  =    foreign no-dependencies 

man_MANS          =    \
siege.1                \
siege.config.1         \
bombardment.1          \
siege2csv.1

man_SOURCE        =    \
siege.pod              \
siege.config.pod       \
bombardment.pod        \
siege2csv.pod

man_SHELLS        =    \
siege.1.in             \
siege.config.1.in      \
bombardment.1.in       \
siege2csv.1.in         

EXTRA_DIST        =    $(man_SOURCE) $(man_SHELLS) urls.txt siegerc.in

DISTCLEANFILES    =    siegerc $(man_MANS) $(man_SHELLS)

SIEGERC           =    $(sysconfdir)/siegerc

URLSTXT           =    $(sysconfdir)/urls.txt

install-exec-hook: 
	@if test -f $(DESTDIR)$(SIEGERC); then \
	  if cmp -s $(srcdir)/siegerc $(DESTDIR)$(SIEGERC); then echo ""; \
	    else \
	    echo ' $(INSTALL_DATA) $(srcdir)/siegerc $(DESTDIR)$(SIEGERC).new'; \
	    $(INSTALL_DATA) $(srcdir)/siegerc $(DESTDIR)$(SIEGERC).new; \
	    echo "#####################################################"; \
	    echo "WARNING: File $(SIEGERC) already exists."; \
	    echo "         A new resource file has been installed as"; \
	    echo "         $(SIEGERC).new. You may want to"; \
	    echo "         consider using the newer version in order to"; \
	    echo "         take advantage of any new features."; \
	    echo "#####################################################"; \
	  fi; \
	else \
	  $(mkinstalldirs) $(DESTDIR)$(sysconfdir); \
	  $(INSTALL_DATA) $(srcdir)/siegerc $(DESTDIR)$(SIEGERC); \
	fi 
	@if test -f $(DESTDIR)$(URLSTXT); then \
	  if cmp -s $(srcdir)/siegerc $(DESTDIR)$(URLSTXT); then echo ""; \
	    else \
	    echo "WARNING: File $(DESTDIR)$(URLSTXT) already exists."; \
	    echo "         It was NOT replaced with this installation."; \
	  fi; \
	else \
	  $(mkinstalldirs) $(DESTDIR)$(sysconfdir); \
	  $(INSTALL_DATA) $(srcdir)/urls.txt $(DESTDIR)$(URLSTXT); \
	fi 


uninstall:
	rm -f $(DESTDIR)$(SIEGERC)

