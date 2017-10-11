##  Makefile.am -- an automake template for Makefile.in
##  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
##  Copyright (C) 2009  Olaf Meeuwissen
##
##  License: GPL-3.0+
##  Author : EPSON AVASYS CORPORATION
##  Author : Olaf Meeuwissen
##  Origin : FreeRISCI
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

AM_CPPFLAGS += -DFUSION_MAX_VECTOR_SIZE=22
AM_CPPFLAGS += -DESCI_GRAMMAR_TRACE=1
AM_CPPFLAGS += -DPKGLIBDIR="\"$(pkglibdir)\""
AM_LDFLAGS  += $(BOOST_LDFLAGS)

driver = esci

driverlibdir    = $(pkglibdir)
driverdatadir   = $(pkgdatadir)/drivers/esci
driver_ldflags  = $(AM_LDFLAGS)
driver_ldflags += -export-dynamic
driver_ldflags += -export-symbols-regex libdrv_$(driver)_LTX_factory

driverlib_LTLIBRARIES = libdrv-esci.la

libdrv_esci_la_LDFLAGS  = $(driver_ldflags)
libdrv_esci_la_LIBADD   = ../../lib/libutsushi.la
libdrv_esci_la_SOURCES  = action.hpp
libdrv_esci_la_SOURCES += buffer.hpp
libdrv_esci_la_SOURCES += bounding-box.hpp
libdrv_esci_la_SOURCES += capture-scanner.cpp
libdrv_esci_la_SOURCES += capture-scanner.hpp
libdrv_esci_la_SOURCES += chunk.hpp
libdrv_esci_la_SOURCES += code-point.hpp
libdrv_esci_la_SOURCES += code-token.hpp
libdrv_esci_la_SOURCES += command.hpp
libdrv_esci_la_SOURCES += compound.cpp
libdrv_esci_la_SOURCES += compound.hpp
libdrv_esci_la_SOURCES += compound-scanner.cpp
libdrv_esci_la_SOURCES += compound-scanner.hpp
libdrv_esci_la_SOURCES += compound-tweaks.cpp
libdrv_esci_la_SOURCES += compound-tweaks.hpp
libdrv_esci_la_SOURCES += constant.hpp
libdrv_esci_la_SOURCES += exception.hpp
libdrv_esci_la_SOURCES += extended-scanner.cpp
libdrv_esci_la_SOURCES += extended-scanner.hpp
libdrv_esci_la_SOURCES += extended-tweaks.cpp
libdrv_esci_la_SOURCES += extended-tweaks.hpp
libdrv_esci_la_SOURCES += get-command-parameters.cpp
libdrv_esci_la_SOURCES += get-command-parameters.hpp
libdrv_esci_la_SOURCES += get-extended-identity.cpp
libdrv_esci_la_SOURCES += get-extended-identity.hpp
libdrv_esci_la_SOURCES += get-extended-status.cpp
libdrv_esci_la_SOURCES += get-extended-status.hpp
libdrv_esci_la_SOURCES += get-focus-position.cpp
libdrv_esci_la_SOURCES += get-focus-position.hpp
libdrv_esci_la_SOURCES += get-hardware-property.cpp
libdrv_esci_la_SOURCES += get-hardware-property.hpp
libdrv_esci_la_SOURCES += get-identity.cpp
libdrv_esci_la_SOURCES += get-identity.hpp
libdrv_esci_la_SOURCES += get-push-button-status.cpp
libdrv_esci_la_SOURCES += get-push-button-status.hpp
libdrv_esci_la_SOURCES += get-scan-parameters.cpp
libdrv_esci_la_SOURCES += get-scan-parameters.hpp
libdrv_esci_la_SOURCES += get-scanner-status.cpp
libdrv_esci_la_SOURCES += get-scanner-status.hpp
libdrv_esci_la_SOURCES += getter.hpp
libdrv_esci_la_SOURCES += grammar-automatic-feed.cpp
libdrv_esci_la_SOURCES += grammar-automatic-feed.hpp
libdrv_esci_la_SOURCES += grammar-automatic-feed.ipp
libdrv_esci_la_SOURCES += grammar-capabilities.cpp
libdrv_esci_la_SOURCES += grammar-capabilities.hpp
libdrv_esci_la_SOURCES += grammar-capabilities.ipp
libdrv_esci_la_SOURCES += grammar-formats.cpp
libdrv_esci_la_SOURCES += grammar-formats.hpp
libdrv_esci_la_SOURCES += grammar-formats.ipp
libdrv_esci_la_SOURCES += grammar-information.cpp
libdrv_esci_la_SOURCES += grammar-information.hpp
libdrv_esci_la_SOURCES += grammar-information.ipp
libdrv_esci_la_SOURCES += grammar-mechanics.cpp
libdrv_esci_la_SOURCES += grammar-mechanics.hpp
libdrv_esci_la_SOURCES += grammar-mechanics.ipp
libdrv_esci_la_SOURCES += grammar-parameters.cpp
libdrv_esci_la_SOURCES += grammar-parameters.hpp
libdrv_esci_la_SOURCES += grammar-parameters.ipp
libdrv_esci_la_SOURCES += grammar-status.cpp
libdrv_esci_la_SOURCES += grammar-status.hpp
libdrv_esci_la_SOURCES += grammar-status.ipp
libdrv_esci_la_SOURCES += grammar-tracer.hpp
libdrv_esci_la_SOURCES += grammar.cpp
libdrv_esci_la_SOURCES += grammar.hpp
libdrv_esci_la_SOURCES += grammar.ipp
libdrv_esci_la_SOURCES += initialize.cpp
libdrv_esci_la_SOURCES += initialize.hpp
libdrv_esci_la_SOURCES += matrix.hpp
libdrv_esci_la_SOURCES += point.hpp
libdrv_esci_la_SOURCES += release-scanner.cpp
libdrv_esci_la_SOURCES += release-scanner.hpp
libdrv_esci_la_SOURCES += scan-parameters.cpp
libdrv_esci_la_SOURCES += scan-parameters.hpp
libdrv_esci_la_SOURCES += scanner-control.cpp
libdrv_esci_la_SOURCES += scanner-control.hpp
libdrv_esci_la_SOURCES += scanner-inquiry.hpp
libdrv_esci_la_SOURCES += scanner.cpp
libdrv_esci_la_SOURCES += scanner.hpp
libdrv_esci_la_SOURCES += set-color-matrix.cpp
libdrv_esci_la_SOURCES += set-color-matrix.hpp
libdrv_esci_la_SOURCES += set-dither-pattern.cpp
libdrv_esci_la_SOURCES += set-dither-pattern.hpp
libdrv_esci_la_SOURCES += set-gamma-table.cpp
libdrv_esci_la_SOURCES += set-gamma-table.hpp
libdrv_esci_la_SOURCES += set-resolution.cpp
libdrv_esci_la_SOURCES += set-resolution.hpp
libdrv_esci_la_SOURCES += set-scan-area.cpp
libdrv_esci_la_SOURCES += set-scan-area.hpp
libdrv_esci_la_SOURCES += set-scan-parameters.cpp
libdrv_esci_la_SOURCES += set-scan-parameters.hpp
libdrv_esci_la_SOURCES += set-zoom.cpp
libdrv_esci_la_SOURCES += set-zoom.hpp
libdrv_esci_la_SOURCES += setter.hpp
libdrv_esci_la_SOURCES += start-extended-scan.cpp
libdrv_esci_la_SOURCES += start-extended-scan.hpp
libdrv_esci_la_SOURCES += start-scan.hpp
libdrv_esci_la_SOURCES += start-standard-scan.cpp
libdrv_esci_la_SOURCES += start-standard-scan.hpp
libdrv_esci_la_SOURCES += vector.hpp

EXTRA_DIST = utsushi-esci.rules

if enable_udev_config

##  Hook into the udev device recognition/configuration machinery.
##  Note that configuration data is machine specific, hence we install
##  the udev rules as part of the install-exec target.  The `automake`
##  rules do the same with data meant for $(sysconfdir).

exec_udev_rulesdir = $(UDEV_SYSCONFDIR)/rules.d

exec_udev_rules_DATA = utsushi-esci.rules

endif # enable_udev_config

driverexeclibdir = $(pkglibexecdir)

AM_INSTALLCHECK_STD_OPTIONS_EXEMPT = $(driverexeclib_PROGRAMS)

# WARNING: The esci-interpreter cannot link with any of the Utsushi
#          libraries, directly or indirectly, for licensing reasons.

driverexeclib_PROGRAMS = esci-interpreter

esci_interpreter_CPPFLAGS  = $(AM_CPPFLAGS)
esci_interpreter_CPPFLAGS += $(BOOST_CPPFLAGS)
esci_interpreter_CPPFLAGS += $(LTDLINCL)
esci_interpreter_CXXFLAGS  = $(AM_CXXFLAGS)
esci_interpreter_CXXFLAGS += $(LIBUSB_CFLAGS)
esci_interpreter_CXXFLAGS += $(LIBUDEV_CFLAGS)
esci_interpreter_LDADD     =
esci_interpreter_LDADD    += $(LIBLTDL)
esci_interpreter_LDADD    += $(LIBUSB_LIBS)
esci_interpreter_LDADD    += $(LIBUDEV_LIBS)
esci_interpreter_SOURCES   =
esci_interpreter_SOURCES  += interpreter.cpp
esci_interpreter_SOURCES  += interpreter.hpp
esci_interpreter_SOURCES  += connexion.cpp
esci_interpreter_SOURCES  += connexion.hpp
esci_interpreter_SOURCES  += machine.cpp
esci_interpreter_SOURCES  += machine.hpp
esci_interpreter_SOURCES  += usb.cpp
esci_interpreter_SOURCES  += usb.hpp
esci_interpreter_SOURCES  += udev.cpp
esci_interpreter_SOURCES  += udev.hpp

if enable_boost_unit_test_framework

noinst_PROGRAMS  = verify

verify_SOURCES  = verify.cpp
verify_SOURCES += verify.hpp
verify_SOURCES += verify-compound-protocol.cpp
verify_SOURCES += verify-compound-scanning.cpp

verify_CPPFLAGS  = $(AM_CPPFLAGS)
verify_CPPFLAGS += -DBOOST_TEST_NO_MAIN
if with_shared_enabled
verify_CPPFLAGS += -DBOOST_TEST_DYN_LINK
endif
if with_static_enabled
verify_CPPFLAGS += -DBOOST_TEST_ALTERNATIVE_INIT_API
endif

verify_LDADD  = libdrv-esci.la
verify_LDADD += ../../lib/libutsushi.la $(LIBUTSUSHI_LIBS)
verify_LDADD += $(BOOST_UNIT_TEST_FRAMEWORK_LIB)
## FIXME: remove once usb and hexdump connexions are proper plugins
verify_LDADD += ../../connexions/libcnx-usb.la
verify_LDADD += ../../connexions/libcnx-hexdump.la

endif # enable_boost_unit_test_framework

##  Data files for use by the driver and verify utility

driverdata_DATA  =
driverdata_DATA += data/DS-40.dat
driverdata_DATA += data/DS-310.dat
driverdata_DATA += data/DS-320.dat
driverdata_DATA += data/DS-360W.dat
driverdata_DATA += data/DS-510.dat
driverdata_DATA += data/DS-520.dat
driverdata_DATA += data/DS-560.dat
driverdata_DATA += data/DS-530.dat
driverdata_DATA += data/DS-535.dat
driverdata_DATA += data/DS-570W.dat
driverdata_DATA += data/DS-575W.dat
driverdata_DATA += data/DS-760.dat
driverdata_DATA += data/DS-770.dat
driverdata_DATA += data/DS-775.dat
driverdata_DATA += data/DS-860.dat
driverdata_DATA += data/DS-1610.dat
driverdata_DATA += data/DS-1630.dat
driverdata_DATA += data/DS-1660W.dat
driverdata_DATA += data/DS-5500.dat
driverdata_DATA += data/DS-6500.dat
driverdata_DATA += data/DS-7500.dat
driverdata_DATA += data/DS-50000.dat
driverdata_DATA += data/DS-60000.dat
driverdata_DATA += data/DS-70000.dat
driverdata_DATA += data/ES-200.dat
driverdata_DATA += data/ES-300W.dat
driverdata_DATA += data/ES-400.dat
driverdata_DATA += data/ES-500W.dat
driverdata_DATA += data/PX-M7050.dat
driverdata_DATA += data/PX-M7050FX.dat
driverdata_DATA += data/PX-M860F.dat
driverdata_DATA += data/WF-6590.dat

EXTRA_DIST += $(driverdata_DATA)

##  A recipe to clone data files from an existing file.  Note that the
##  $$src and $$dst variables need to have the same length for cloning
##  to work.  Clones will have their firmware version numbers wiped to
##  indicate the fact that they aren't based on any hardware out there
##  in the wild.

clone_dat = \
	src=`echo $< | sed 's,^.*/,,; s,\.dat$$,,'`; \
	dst=`echo $@ | sed 's,^.*/,,; s,\.dat$$,,'`; \
	sed -e "s,\(\#PRDh...\)$$src,\1$$dst," \
	    -e 's,\([^\#]\)IPL ....,\1IPL     ,' \
	    -e 's,\([^\#]\)ADF ....,\1ADF     ,' \
	    -e 's,\([^\#]\)FB  ....,\1FB      ,' \
	    -e 's,\([^\#]\)TPU ....,\1TPU     ,' \
	    $< > $@

$(srcdir)/data/DS-310.dat: $(srcdir)/data/DS-360W.dat
	sed -e 's,DS-360W,DS-310 ,' $< > $@

$(srcdir)/data/DS-320.dat: $(srcdir)/data/DS-360W.dat
	sed -e 's,DS-360W,DS-320 ,' $< > $@

$(srcdir)/data/DS-520.dat: $(srcdir)/data/DS-510.dat
	$(clone_dat)

$(srcdir)/data/DS-560.dat: $(srcdir)/data/DS-510.dat
	$(clone_dat)

$(srcdir)/data/DS-570W.dat: $(srcdir)/data/DS-530.dat
	sed -e 's,DS-530 ,DS-570W,' $< > $@

$(srcdir)/data/DS-760.dat: $(srcdir)/data/DS-860.dat
	$(clone_dat)

$(srcdir)/data/DS-770.dat: $(srcdir)/data/DS-530.dat
	$(clone_dat)

$(srcdir)/data/DS-1610.dat: $(srcdir)/data/DS-1630.dat
	$(clone_dat)

$(srcdir)/data/DS-1660W.dat: $(srcdir)/data/DS-1630.dat
	sed -e 's,DS-1630 ,DS-1660W,' $< > $@

$(srcdir)/data/DS-6500.dat: $(srcdir)/data/DS-7500.dat
	$(clone_dat)

$(srcdir)/data/DS-60000.dat: $(srcdir)/data/DS-70000.dat
	$(clone_dat)

$(srcdir)/data/ES-200.dat: $(srcdir)/data/DS-360W.dat
	sed -e 's,DS-360W,ES-200 ,' $< > $@

$(srcdir)/data/ES-300W.dat: $(srcdir)/data/DS-360W.dat
	$(clone_dat)

$(srcdir)/data/ES-400.dat: $(srcdir)/data/DS-530.dat
	$(clone_dat)

$(srcdir)/data/ES-500W.dat: $(srcdir)/data/DS-530.dat
	sed -e 's,DS-530 ,ES-500W,' $< > $@

$(srcdir)/data/PX-M7050FX.dat: $(srcdir)/data/PX-M7050.dat
	sed -e 's,PID 08BC,PID 08CC,' $< > $@

$(srcdir)/data/WF-6590.dat: $(srcdir)/data/PX-M860F.dat
	sed -e 's,PID 08CE,PID 08CF,' $< > $@

$(srcdir)/data/DS-535.dat: $(srcdir)/data/DS-530.dat
	$(clone_dat)

$(srcdir)/data/DS-775.dat: $(srcdir)/data/DS-530.dat
	$(clone_dat)

$(srcdir)/data/DS-575W.dat: $(srcdir)/data/DS-530.dat
	sed -e 's,DS-530 ,DS-575W,' $< > $@
