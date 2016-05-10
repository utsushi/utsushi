##  Makefile.am -- an automake template for Makefile.in
##  Copyright (C) 2012, 2014, 2015  SEIKO EPSON CORPORATION
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

EXTRA_DIST  = $(indent_config)

if enable_boost_unit_test_framework

TESTS_ENVIRONMENT =
TESTS = $(check_PROGRAMS)

check_PROGRAMS  =
if enable_uncrustify_test
check_PROGRAMS += indentation.utr
endif

AM_LDFLAGS  += $(BOOST_LDFLAGS)

LDADD  =
LDADD += $(BOOST_FILESYSTEM_LIB)
LDADD += $(BOOST_IOSTREAMS_LIB)
LDADD += $(BOOST_REGEX_LIB)

##  Create all test inputs before running any of the tests.

all-local: $(indent_config) $(srcdir)/vc-dist-files

endif # enable_boost_unit_test_framework

CLEANFILES  =

include $(top_srcdir)/include/boost-test.am

##  List files that are under version control.  The list is determined
##  from the information of your working copy if possible or a shipped
##  list otherwise.  This list can be used to automatically keep files
##  in sync with policies or subject them to (policy conformance) test
##  suites.  Files are listed relative to $(builddir).

vc_list_files = \
	$(top_srcdir)/upstream/tools/vc-list-files \
	  -C $(top_srcdir) 2>/dev/null \
	  || sed 's|^\.\./|$(top_srcdir)/|' $(srcdir)/vc-dist-files

##  Create a list of distributed files that are under version control.
##  This list can be used in the absence of a version control system,
##  such as when you are working from a distributed source archive.
##  Note that files are listed relative to $(srcdir).

distdir ?= $(srcdir)
$(srcdir)/vc-dist-files:
	-@top_srcdir=`echo $(top_srcdir) | sed 's|\\.|\\\\.|g'`; \
	for file in `$(vc_list_files)`; do \
	  file=`echo $$file | sed "s|^$$top_srcdir/|../|"`; \
	  test -e $(distdir)/$$file \
	    && echo $$file; \
	done > $(@F).tmp
	@if cmp $@ $(@F).tmp >/dev/null 2>&1; then \
	  rm $(@F).tmp; \
	else \
	  if test -s $(@F).tmp; then \
	    mv $(@F).tmp $@; \
	  else \
	    rm $(@F).tmp; \
	  fi; \
	fi

dist-hook: $(srcdir)/vc-dist-files
	cp $< $(distdir)/vc-dist-files

clean-local:
	-@test "$(srcdir)" != "$(builddir)" && rm vc-dist-files

.PHONY: $(srcdir)/vc-dist-files

##  C++ source code indentation policy

indent_config = $(srcdir)/uncrustify.cfg

$(indent_config):
	uncrustify -c /dev/null --update-config > $@
create-indent-config: $(indent_config)
update-indent-config: $(indent_config)
	uncrustify -c $< --update-config > $<.new && mv $<.new $<

indent: $(indent_config)
	for file in `$(vc_list_files) | sed -n '/.*\.[chi]pp/p'`; do \
	  uncrustify -c $< -l CPP --replace --no-backup $$file; \
	done

.PHONY: indent create-indent-config update-indent-config
