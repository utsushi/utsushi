##  Makefile.am -- an automake template for Makefile.in
##  Copyright (C) 2012  SEIKO EPSON CORPORATION
##  Copyright (C) 2015  Olaf Meeuwissen
##
##  License: GPL-3.0+
##  Author : EPSON AVASYS CORPORATION
##
##  This file is part of the 'Utsushi' package.
##  This package is free software: you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation, either version 3 of the License or, at
##  your option, any later version.
##
##    This program is distributed in the hope that it will be useful,
##    but WITHOUT ANY WARRANTY; without even the implied warranty of
##    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##    GNU General Public License for more details.
##
##  You ought to have received a copy of the GNU General Public License
##  along with this package.  If not, see <http://www.gnu.org/licenses/>.

##  Process this file with automake to make a Makefile.in file.

SUBDIRS  = .
SUBDIRS += tests

##  All applications and utilities are installed in $(pkglibexecdir)
##  so that we can easily install multiple flavors on a system.  This
##  is useful when we need to provide a customised one-off or when we
##  want to give our 'trunk' or a branch more exposure.

pkglibexec_PROGRAMS  = main
pkglibexec_PROGRAMS += version
pkglibexec_PROGRAMS += help
pkglibexec_PROGRAMS += list
pkglibexec_PROGRAMS += scan
pkglibexec_PROGRAMS += scan-cli

AM_DEFAULT_SOURCE_EXT = .cpp

## FIXME: remove once the USB connexion has become a proper plugin
AM_CXXFLAGS += $(LIBUSB_CFLAGS)
AM_LDFLAGS  += $(BOOST_LDFLAGS)

LDADD  = ../lib/libutsushi.la $(LIBUTSUSHI_LIBS)
##  FIXME: drop when filters have been turned in to proper plugins
LDADD += ../filters/libflt-all.la
##  FIXME: drop when connexions have been turned into proper plugins
LDADD += ../connexions/libcnx-usb.la
LDADD += ../connexions/libcnx-hexdump.la

if with_gtkmm
pkglibexec_PROGRAMS += scan-gtkmm
scan_gtkmm_CPPFLAGS  = $(AM_CPPFLAGS)
scan_gtkmm_CXXFLAGS  = $(AM_CXXFLAGS)
scan_gtkmm_CXXFLAGS += $(LIBGTKMM_CFLAGS)
scan_gtkmm_LDFLAGS   = $(AM_LDFLAGS)
scan_gtkmm_LDADD     = ../gtkmm/libutsushi-gtkmm.la
scan_gtkmm_LDADD    += $(LIBGTKMM_LIBS)
scan_gtkmm_LDADD    += $(LDADD)
endif

##  A little helper target so our test suite can make sure all the
##  programs it invokes have been built using the latest sources.

.PHONY: check-pkglibexec-PROGRAMS
check-pkglibexec-PROGRAMS: $(pkglibexec_PROGRAMS)

##  Prefix all installed applications and utilities so that we can
##  easily tell what flavor they came from.  This also helps us in
##  telling whether applications and utilities are run in "staged"
##  mode or not.

transform = s,^,$(PACKAGE_TARNAME)-,

##  Make the entry point for all applications and utilities available
##  to user space.  We do this by creating a link in $(bindir) to the
##  $(main) application in $(pkglibexecdir).  When uninstalling, the
##  link is removed.

main = `echo "main$(EXEEXT)" \
	| sed 's,^.*/,,;s/$(EXEEXT)$$//;$(transform);s/$$/$(EXEEXT)/'`

install-exec-hook:
	test -z "$(bindir)" || $(MKDIR_P) "$(DESTDIR)$(bindir)"
	cd $(DESTDIR)$(bindir) && \
	  (rm -f $(DESTDIR)$(bindir)/$(PACKAGE_TARNAME)$(EXEEXT); \
	   $(LN_S) $(pkglibexecdir)/$(main) \
	           $(DESTDIR)$(bindir)/$(PACKAGE_TARNAME)$(EXEEXT))

uninstall-hook:
	cd $(DESTDIR)$(bindir) && rm -f $(PACKAGE_TARNAME)$(EXEEXT)

##  Test `help` and `version` commands for all installed programs and
##  utilities.  Invocations use both $(pkglibexecdir)/$(main) and the
##  user space link in $(bindir).
##  Code based on the GNU standard options installcheck code.

installcheck-local: $(pkglibexec_PROGRAMS)
	bad=0; pid=$$$$; \
	if test -n "$(DESTDIR)"; then \
	  echo "WARN: $@ not supported for DESTDIR installs" 1>&2; \
	else \
	  list="$(pkglibexec_PROGRAMS)"; \
	  for p in $$list; do \
	    case ' $(AM_INSTALLCHECK_STD_OPTIONS_EXEMPT) ' in \
	      *" $$p "* | *" $(srcdir)/$$p "*) continue;; \
	    esac; \
	    s=`echo "$$p" | sed 's,^.*/,,;s/$(EXEEXT)$$//;s/^main$$//'`; \
	    f=`echo "$$p" \
	       | sed 's,^.*/,,;s/$(EXEEXT)$$//;$(transform);s/$$/$(EXEEXT)/'`; \
	    for exe in \
	        "$(pkglibexecdir)/$(main)" \
	        "$(bindir)/$(PACKAGE_TARNAME)$(EXEEXT)"; do \
	      for cmd in help version; do \
	        "$(pkglibexecdir)/$$f" --$$cmd >expect$${pid}_.out \
	            2>expect$${pid}_.err </dev/null; \
	        if "$$exe" $$cmd $$s >result$${pid}_.out \
	                   2>result$${pid}_.err </dev/null \
	            && test -n "`cat result$${pid}_.out`" \
	            && test -z "`cat result$${pid}_.err`"; then \
	          if cmp -s expect$${pid}_.out result$${pid}_.out \
	             && cmp -s expect$${pid}_.err result$${pid}_.err; \
	          then \
	            :; \
	          else \
	            echo "FAIL: $$exe $$cmd $$s not spec compliant" 1>&2; \
	            diff -u expect$${pid}_.out result$${pid}_.out 1>&2; \
	            diff -u expect$${pid}_.err result$${pid}_.err 1>&2; \
	            bad=1; \
	          fi; \
	        else \
	          echo "FAIL: $$exe $$cmd $$s" 1>&2; \
	          bad=1; \
	        fi; \
	      done; \
	    done; \
	  done; \
	  rm -f expect$${pid}_.??? result$${pid}_.???; \
	fi; \
	exit $$bad
