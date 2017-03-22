##  Makefile.am -- an automake template for Makefile.in
##  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
##  Copyright (C) 2013  Olaf Meeuwissen
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
if WITH_INCLUDED_LTDL
SUBDIRS += upstream/ltdl
endif
SUBDIRS += lib
SUBDIRS += connexions
SUBDIRS += filters
if with_gtkmm
SUBDIRS += gtkmm
endif
SUBDIRS += drivers
SUBDIRS += drivers/esci
if WITH_SANE
SUBDIRS += sane
endif
SUBDIRS += po
SUBDIRS += src
SUBDIRS += doc
SUBDIRS += tests

EXTRA_DIST  = bootstrap
EXTRA_DIST += config-bot.h
EXTRA_DIST += install-deps
EXTRA_DIST += tools/boost-test-report
EXTRA_DIST += tools/boost-test-report.xsl
EXTRA_DIST += tools/include-boost-src
EXTRA_DIST += upstream/include/no_attribute_directive.hpp
EXTRA_DIST += upstream/tools/vc-list-files
EXTRA_DIST += .gitignore

##  Conform AC_CONFIG_MACRO_DIR in configure.ac.

ACLOCAL_AMFLAGS = -I upstream/aclocal

##  Make sure that we exercise building of the distributed sources to
##  their maximum extent.

DISTCHECK_CONFIGURE_FLAGS  = --with-included-ltdl
DISTCHECK_CONFIGURE_FLAGS += --with-gtkmm
DISTCHECK_CONFIGURE_FLAGS += --with-sane
DISTCHECK_CONFIGURE_FLAGS += --with-magick
DISTCHECK_CONFIGURE_FLAGS += --with-jpeg
DISTCHECK_CONFIGURE_FLAGS += --with-tiff
DISTCHECK_CONFIGURE_FLAGS += --enable-test-reports
DISTCHECK_CONFIGURE_FLAGS += --enable-code-coverage

##  Unprivileged users will not be able to install the SANE component
##  or integrate with the udev activated device set up.  That should
##  not prevent them from running a distcheck.

DISTCHECK_CONFIGURE_FLAGS += --disable-sane-config
DISTCHECK_CONFIGURE_FLAGS += --disable-udev-config

DISTCHECK_CONFIGURE_FLAGS += --with-download-dir=$(DOWNLOAD_DIR)

##  API header files

pattern_headers  = utsushi/pattern/decorator.hpp

stream_headers  = utsushi/octet.hpp
stream_headers += utsushi/context.hpp
stream_headers += utsushi/iobase.hpp
stream_headers += utsushi/device.hpp
stream_headers += utsushi/filter.hpp
stream_headers += utsushi/buffer.hpp
stream_headers += utsushi/stream.hpp
stream_headers += utsushi/pump.hpp

setting_headers  = utsushi/constraint.hpp
setting_headers += utsushi/descriptor.hpp
setting_headers += utsushi/key.hpp
setting_headers += utsushi/media.hpp
setting_headers += utsushi/option.hpp
setting_headers += utsushi/preset.hpp
setting_headers += utsushi/quantity.hpp
setting_headers += utsushi/range.hpp
setting_headers += utsushi/store.hpp
setting_headers += utsushi/string.hpp
setting_headers += utsushi/toggle.hpp
setting_headers += utsushi/tag.hpp
setting_headers += utsushi/value.hpp

test_headers  = utsushi/test/runner.ipp
test_headers += utsushi/test/catch-system-errors.hpp
test_headers += utsushi/test/command-line.hpp
test_headers += utsushi/test/environment.hpp
test_headers += utsushi/test/tools.hpp
test_headers += utsushi/test/memory.hpp
test_headers += utsushi/test/null.hpp

wrapper_headers  =
wrapper_headers += utsushi/array.hpp
wrapper_headers += utsushi/cstdint.hpp
wrapper_headers += utsushi/condition-variable.hpp
wrapper_headers += utsushi/format.hpp
wrapper_headers += utsushi/functional.hpp
wrapper_headers += utsushi/memory.hpp
wrapper_headers += utsushi/mutex.hpp
wrapper_headers += utsushi/regex.hpp
wrapper_headers += utsushi/signal.hpp
wrapper_headers += utsushi/thread.hpp
wrapper_headers += utsushi/type-traits.hpp

nobase_include_HEADERS  = $(pattern_headers)
nobase_include_HEADERS += $(stream_headers)
nobase_include_HEADERS += $(setting_headers)
nobase_include_HEADERS += utsushi/connexion.hpp
nobase_include_HEADERS += utsushi/device-info.hpp
nobase_include_HEADERS += utsushi/exception.hpp
nobase_include_HEADERS += utsushi/i18n.hpp
nobase_include_HEADERS += utsushi/log.hpp
nobase_include_HEADERS += utsushi/monitor.hpp
nobase_include_HEADERS += utsushi/run-time.hpp
nobase_include_HEADERS += utsushi/scanner.hpp
nobase_include_HEADERS += $(wrapper_headers)

EXTRA_DIST += $(test_headers)
EXTRA_DIST += lib/tag.xml
EXTRA_DIST += lib/tag.xsl
EXTRA_DIST += lib/tag.cpp

BUILT_SOURCES  = $(srcdir)/utsushi/tag.hpp
BUILT_SOURCES += $(srcdir)/lib/tag.cpp

$(srcdir)/utsushi/tag.hpp $(srcdir)/lib/tag.cpp: $(srcdir)/lib/tag.xml \
  $(srcdir)/lib/tag.xsl
	format=`echo $@ | sed 's|.*\.\([^.]*\)$$|\1|'`; \
	sed -n \
	    -e "/^<!--/s/\.xml ::/.$$format --/" \
	    -e '/^<!--/,/-->/{ /-->/d; s|^$$|//|p; s|^....|//|p; }' $< > $@; \
	xsltproc --stringparam format $$format $(srcdir)/lib/tag.xsl $< >> $@
	sed -i 's/SEC_N_("%1%")/"%1%"/' $@

if with_included_boost

all-local: boost-libs

if with_shared_enabled

##  Install those Boost libraries that we (likely) need at run-time.
##  Boost.Test libraries should *not* be needed so we explicitly skip
##  them here.

install-exec-local: $(boostdir)/lib/stamp
	@$(NORMAL_INSTALL)
	@test -z "$(pkglibdir)" || $(MKDIR_P) "$(DESTDIR)$(pkglibdir)"
	@install_lib="$(INSTALL) $(INSTALL_STRIP_FLAGS)"; \
	for lib in `ls $(boostdir)/lib/libboost_*$(LT_MODULE_EXT).*`; do \
	  case $$lib in \
	    */libboost_unit_test_framework$(LT_MODULE_EXT).*) \
	      ;; \
	    */libboost_test_exec_monitor$(LT_MODULE_EXT).*) \
	      ;; \
	    */libboost_prg_exec_monitor$(LT_MODULE_EXT).*) \
	      ;; \
	    *) \
	      cmd="$$install_lib $$lib $(DESTDIR)$(pkglibdir)"; \
	      echo $$cmd; \
	      eval $$cmd; \
	      filename=`echo $$lib | sed 's,.*/,,'`; \
	      linkname=`echo $$filename \
	        | sed 's/$(LT_MODULE_EXT).*$$/$(LT_MODULE_EXT)/'`; \
	      test -e $(DESTDIR)$(pkglibdir)/$$linkname \
	        && rm $(DESTDIR)$(pkglibdir)/$$linkname || true; \
	      $(LN_S) $$filename $(DESTDIR)$(pkglibdir)/$$linkname ; \
	      ;; \
	  esac; \
	done

uninstall-local:
	-rm $(DESTDIR)$(pkglibdir)/libboost_*$(LT_MODULE_EXT)*

endif # with_shared_enabled

endif # with_included_boost

##  The Boost sources take an approach to building that is opposite of
##  that used by autoconf and automake.  Whereas the latter build from
##  a $(builddir) and get sources from a $(srcdir), the Boost sources
##  expect to build from their $(srcdir) and put the build results in
##  a $(builddir).

##  To make the whole affair even more cumbersome, some parts (did you
##  just say bjam?) cannot be built from a read-only $(srcdir) as they
##  like to scribble in that directory.  Of course, a `make distcheck`
##  makes the whole source tree read-only, so ... we have a workaround
##  that basically copies the sources into the build tree, makes sure
##  the build can scribble, builds and removes all sources again as if
##  nothing out of the ordinary happened.

boostdir = upstream/boost

boost-libs: $(boostdir)/lib/stamp
$(boostdir)/lib/stamp: Boost.Build
	@echo "Building Boost libraries"; \
	if test $(srcdir) = $(builddir); then \
	  cmd="cd $(boostdir); ./bin/$(BOOST_BUILD) \
	    --build-dir=. --stagedir=."; \
	else \
	  dir1=$(srcdir)/$(boostdir); \
	  dir2=$(boostdir); \
	  $(am__relativize); \
	  cmd="cd $(srcdir)/$(boostdir); $$reldir/bin/$(BOOST_BUILD) \
	    --build-dir=$$reldir --stagedir=$$dir2"; \
	fi; \
	cmd="($$cmd variant=debug $(boost_build_link_properties))"; \
	echo $$cmd; \
	eval $$cmd && touch $@

BOOST_BUILD = bjam

Boost.Build: $(boostdir)/bin/$(BOOST_BUILD)
$(boostdir)/bin/$(BOOST_BUILD):
	echo "Building Boost.Build ($(BOOST_BUILD))"; \
	if test $(srcdir) = $(builddir); then \
	  (cd $(boostdir)/tools/build/v2; \
	   ./bootstrap.sh \
	     && ./$(BOOST_BUILD) install --prefix=../../..); \
	else \
	  test -d $(boostdir)/tools/build \
	    || $(MKDIR_P) $(boostdir)/tools/build; \
	  cp -a $(srcdir)/$(boostdir)/tools/build/v2 \
	    $(boostdir)/tools/build; \
	  find $(boostdir) -type d -exec chmod u+w {} ';'; \
	  (cd $(boostdir)/tools/build/v2; \
	   ./bootstrap.sh \
	     && ./$(BOOST_BUILD) install --prefix=../../..) \
	    && rm -rf $(boostdir)/tools/build/v2; \
	  rmdir $(boostdir)/tools/build || true; \
	  rmdir $(boostdir)/tools || true; \
	fi

##  What a target for the bcp utility *could* look like.  We haven't
##  been able to get something to work (using boost-1.46.1 on Ubuntu
##  10.04 LTS) with a read-only source tree, getting bcp to link w/o
##  errors and install somewhere below $(boostdir).
##  For the time being we will rely on an existing bcp utility.
##
##  $(boostdir)/bcp: $(boostdir)/bjam
##	@dir1=$(srcdir)/$(boostdir); \
##	dir2=$(boostdir); \
##	$(am__relativize); \
##	cmd="(cd $(srcdir)/$(boostdir); $$reldir/bjam \
##	  --build-dir=$$reldir tools/bcp)"; \
##	echo $$cmd; \
##	eval $$cmd

mostlyclean-local:
	-rm -r $(boostdir)/boost/bin.v2
	-rm $(boostdir)/bootstrap.log

clean-local:
	-rm -r $(boostdir)/lib
	-rm $(boostdir)/bjam

distclean-local:

maintainer-clean-local:
	-rm -r $(srcdir)/$(boostdir)/../downloads/boost*
	-rm -r $(srcdir)/$(boostdir)

##  This target should *not* be conditionalized on with_included_boost
##  as that only affects build-related actions.  Shipping the upstream
##  Boost sources is independent from that.

dist-hook:
	(cd $(distdir); tools/include-boost-src --source=$(DOWNLOAD_DIR))

##  If successful, the distcheck target removes the whole $(distdir)
##  and takes all build results with it.  That includes all coverage
##  data which is highly undesirable if you want to perform analysis
##  after a `make distcheck`.
##  Set the 'dc_precious_dir' variable on the command-line to create
##  a full copy of the $(top_builddir) for later analysis.

installcheck-local:
	if test -n "$(dc_precious_dir)"; then \
	    test -d "$(dc_precious_dir)" || $(MKDIR_P) "$(dc_precious_dir)"; \
	    cp -pR $(top_builddir) "$(dc_precious_dir)"; \
	fi
