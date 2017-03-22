##  Makefile.am -- an automake template for Makefile.in
##  Copyright (C) 2012-2014  SEIKO EPSON CORPORATION
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

EXTRA_DIST =

BACKEND_NAME = utsushi
SANE_VERSION = $(SANE_MAJOR):$(SANE_MINOR):$(SANE_REVISION)

##  Install below $(pkglibdir) rather than in $(SANE_BACKENDDIR) to
##  support scenarios that configure for installation in a writable
##  location but lack write support to the SANE locations, e.g. for
##  people that want to install below $HOME.  They can disable SANE
##  component activation with the `--disable-sane-config` option to
##  `./configure` and install without any problems.  It is assumed
##  that such people have other means to make their SANE setup take
##  note of the backend.

exec_sane_backenddir = $(pkglibdir)/sane

exec_sane_backend_LTLIBRARIES = libsane-$(BACKEND_NAME).la
libsane___BACKEND_NAME__la_SOURCES  = backend.cpp
libsane___BACKEND_NAME__la_SOURCES += backend.hpp
libsane___BACKEND_NAME__la_SOURCES += version.hpp
libsane___BACKEND_NAME__la_SOURCES += guard.hpp
libsane___BACKEND_NAME__la_SOURCES += handle.cpp
libsane___BACKEND_NAME__la_SOURCES += handle.hpp
libsane___BACKEND_NAME__la_SOURCES += device.cpp
libsane___BACKEND_NAME__la_SOURCES += device.hpp
libsane___BACKEND_NAME__la_SOURCES += value.cpp
libsane___BACKEND_NAME__la_SOURCES += value.hpp
libsane___BACKEND_NAME__la_SOURCES += log.hpp
libsane___BACKEND_NAME__la_LDFLAGS  = -export-dynamic
libsane___BACKEND_NAME__la_LDFLAGS += -export-symbols sane-api.sym
libsane___BACKEND_NAME__la_LDFLAGS += -version-number $(SANE_VERSION)
libsane___BACKEND_NAME__la_LDFLAGS += -weak libsane
libsane___BACKEND_NAME__la_LIBADD   = ../lib/libutsushi.la
##  FIXME: drop when filters have been turned into proper plugins
libsane___BACKEND_NAME__la_LIBADD  += ../filters/libflt-all.la
##  FIXME: drop when connexions have been turned into proper plugins
libsane___BACKEND_NAME__la_LIBADD  += ../connexions/libcnx-usb.la
libsane___BACKEND_NAME__la_LIBADD  += ../connexions/libcnx-hexdump.la
EXTRA_libsane___BACKEND_NAME__la_DEPENDENCIES  = sane-api.sym

AM_CPPFLAGS += -DBACKEND_NAME=$(BACKEND_NAME)
AM_CPPFLAGS += -DSANE_MAJOR=$(SANE_MAJOR)
AM_CPPFLAGS += -DSANE_MINOR=$(SANE_MINOR)

sane-api.sym: $(srcdir)/backend.hpp
	$(AWK) '/^#define sane_/{ print $$2 }' $< \
	    | sed '/_$$/d' \
	    | sed -n 'p; s/sane_/sane_$(BACKEND_NAME)_/p' > $@

CLEANFILES = sane-api.sym

##  Minimal sanity checks on the distributed backend.

SANE_BACKEND_SANITY_CHECKS  =
if with_shared_enabled
SANE_BACKEND_SANITY_CHECKS += check-soname
endif

check-local: $(SANE_BACKEND_SANITY_CHECKS)

##  Note: Although a fair amount of effort has gone into writing this
##  test to work on all platfroms, it probably fails on quite a few.
##  This is most likely caused in the attempt to extract the library's
##  soname.  If this test fails on your platform and you know how to
##  get at the library's soname (or otherwise fix this test), we are
##  happy to accommodate patches.
##
check-soname: libsane-$(BACKEND_NAME).la
	eval `$(LIBTOOL) --config \
	      | sed -n '/^objdir=/p; \
	                /^shrext_cmds=/p; \
	                /^soname_spec=/p; \
	                /^libname_spec=/p; \
	                /^library_names_spec=/p'`; \
	test -f libsane-$(BACKEND_NAME).la || exit 5; \
	eval `sed -n '/^library_names=/p' libsane-$(BACKEND_NAME).la`; \
	sofile=$$objdir/`echo $$library_names | $(AWK) '{print $$1}'`; \
	test -f $$sofile || exit 4; \
	shared_ext="$$shrext_cmds"; \
	case $(host) in \
	    *cygwin* | mingw* | *cegcc*)  major=-$(SANE_MAJOR);; \
	    *)	                          major=.$(SANE_MAJOR);; \
	esac; \
	name=sane; \
	eval libname="$$libname_spec"; \
	eval soname_expect="$$soname_spec"; \
	case $(host) in \
	    *) \
	       soname=`$(OBJDUMP) -p $$sofile \
                       | $(AWK) '/SONAME/ {print $$2}'` \
	       ;; \
	esac; \
	if test "$$soname" != "$$soname_expect"; then \
	    echo "$@: $$sofile: $$soname_expect != $$soname"; \
	    exit 3; \
	fi;

##  Distributed patches for ltmain.sh that try to make sure we meet
##  the special soname requirements on SANE backend libraries (that
##  check-soname attempts to verify for the backend we provide).
##  Patches can be updated with the ltmain.sh.diff target.  Patches
##  for other ltmain.sh version may be added in which case you have
##  to update $(top_srcdir)/bootstrap too.

EXTRA_DIST += ltmain.sh-2.4.6.diff
EXTRA_DIST += ltmain.sh-2.4.2.diff
EXTRA_DIST += ltmain.sh-2.2.6b.diff

##  Convenience target to maintain our SANE specific libtool patch.
##  The target will only update the patch if `./bootstrap`ping has
##  resulted in a backup of libtool's original ltmain.sh.  Absence
##  of a backup indicates the current patch applied cleanly.
##  Note, both `cmp` and `diff` return an exit status of 1 in case
##  there are any differences and 2 in case of trouble.
##
auxdir = $(top_srcdir)/upstream
ltmain.sh.diff: $(auxdir)/ltmain.sh.orig $(auxdir)/ltmain.sh
	test -f $< || exit 0
	cmp -s $^; \
	if test 1 == $$?; then \
	    eval `sed -n '/^VERSION=/{ p; q }' $<`; \
	    > $@; \
	    echo "#  `basename $@` -- SANE backend library soname handling fixes" >> $@; \
	    echo "#  Copyright (C) 2012  SEIKO EPSON CORPORATION" >> $@; \
	    echo "#  Copyright (C) 2007  EPSON AVASYS CORPORATION" >> $@;\
	    echo "#" >> $@; \
	    echo "#  License: GPL-2.0+ (same as ltmain.sh)" >> $@; \
	    echo "#  Author : EPSON AVASYS CORPORATION" >> $@; \
	    echo "#" >> $@; \
	    echo "#  Patch relative to: GNU libtool $$VERSION" >> $@; \
	    echo "" >> $@; \
	    diff -U 5 $^ >> $@; \
	    test 1 == $$? && true; \
	fi

##  Handle missing prerequisite for ltmain.sh.diff target
$(top_srcdir)/upstream/ltmain.sh.orig:
	-exit 0

if enable_sane_config

##  Hook the backend into the SANE configuration setup.

##  Create/remove links from the $(SANE_BACKENDDIR) to all backend
##  libraries that were installed below $(exec_sane_backenddir).
##  Care is taken to remove only the links that were created.  All
##  combinations of shared and static builds ought to do the Right
##  Thing.

if with_static_enabled
STATIC_BACKEND_LIB = libsane-$(BACKEND_NAME).a
endif

install-exec-hook:
	@$(NORMAL_INSTALL)
	test -z "$(DESTDIR)$(SANE_BACKENDDIR)" \
	  || test -d "$(DESTDIR)$(SANE_BACKENDDIR)" \
	  || $(MKDIR_P) "$(DESTDIR)$(SANE_BACKENDDIR)"
	eval `sed -n '/^library_names=/p' libsane-$(BACKEND_NAME).la`; \
	library_names="$$library_names libsane-$(BACKEND_NAME).la"; \
	library_names="$$library_names $(STATIC_BACKEND_LIB)"; \
	dir1="$(SANE_BACKENDDIR)"; \
	dir2="$(exec_sane_backenddir)"; \
	$(am__relativize); \
	for p in $$library_names; do \
	  test -e $(DESTDIR)$(SANE_BACKENDDIR)/$$p \
	    && rm -f $(DESTDIR)$(SANE_BACKENDDIR)/$$p; \
	  $(LN_S) $$dir2/$$p $(DESTDIR)$(SANE_BACKENDDIR); \
	done

uninstall-hook-exec:
	@$(NORMAL_UNINSTALL)
	-eval `sed -n '/^library_names=/p' libsane-$(BACKEND_NAME).la`; \
	library_names="$$library_names libsane-$(BACKEND_NAME).la"; \
	library_names="$$library_names $(STATIC_BACKEND_LIB)"; \
	for p in $$library_names; do \
	  rm -f $(DESTDIR)$(SANE_BACKENDDIR)/$$p; \
	done

##  If the build system uses dll.d, we prefer that and just drop a
##  file named after the backend there with the backend name in it.
##  Otherwise, we edit dll.conf and add or uncomment the backend.
##  When uninstalling, we reverse these actions but note that the
##  backend is not removed from dll.conf (because we cannot check
##  whether we added it or not).  It is commented out, though.

install-data-hook:
	@$(NORMAL_INSTALL)
	@if test -z "$(SANE_SYSCONFDIR)"; then \
	  echo $(ECHO_N) "WARNING: SANE configuration directory not set,"; \
	  echo " skipping component activation"; \
	  exit 1; \
	fi
	if test -d "$(SANE_SYSCONFDIR)/dll.d"; then \
	  dirname="$(DESTDIR)$(SANE_SYSCONFDIR)/dll.d"; \
	  file=$$dirname/$(BACKEND_NAME); \
	  test -d $$dirname || $(MKDIR_P) $$dirname; \
	  echo "#  $(BACKEND_NAME) -- dll.conf snippet" > $$file; \
	  echo "" >> $$file; \
	  echo "$(BACKEND_NAME)" >> $$file; \
	else \
	  dirname="$(DESTDIR)$(SANE_SYSCONFDIR)"; \
	  file=$$dirname/dll.conf; \
	  if test -f $$file; then \
	    if test -n "`$(GREP) '^[ \t]*#[ \t#]*$(BACKEND_NAME)' $$file`"; \
	    then \
	      sed -i 's,^[ \t]*#[ \t#]*\($(BACKEND_NAME)\),\1,' $$file; \
	    elif test -z "`$(GREP) $(BACKEND_NAME) $$file`"; then\
	      echo $(BACKEND_NAME) >> $$file; \
	    fi; \
	  else \
	    echo $(ECHO_N) "WARNING: $$file not found,"; \
	    echo " cannot activate SANE component"; \
	  fi; \
	fi

uninstall-hook-data:
	@$(NORMAL_UNINSTALL)
	@if test -z "$(SANE_SYSCONFDIR)"; then \
	  echo $(ECHO_N) "WARNING: SANE configuration directory not set,"; \
	  echo " skipping component deactivation"; \
	  exit 1; \
	fi
	-if test -d "$(SANE_SYSCONFDIR)/dll.d"; then \
	  rm -f "$(DESTDIR)$(SANE_SYSCONFDIR)/dll.d/$(BACKEND_NAME)"; \
	else \
	  dirname="$(DESTDIR)$(SANE_SYSCONFDIR)"; \
	  file=$$dirname/dll.conf; \
	  if test -f $$file; then \
	    if test -n "`$(GREP) '^[ \t]*$(BACKEND_NAME)' $$file`"; then \
	      sed -i 's,^[ \t]*\($(BACKEND_NAME)\),#\1,' $$file; \
	    fi; \
	  else \
	    echo $(ECHO_N) "WARNING: $$file not found,"; \
	    echo " cannot deactivate SANE component"; \
	  fi; \
	fi

uninstall-hook: uninstall-hook-exec uninstall-hook-data

endif # enable_sane_config
