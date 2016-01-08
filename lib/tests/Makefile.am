##  Makefile.am -- an automake template for Makefile.in
##  Copyright (C) 2012, 2013  SEIKO EPSON CORPORATION
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

if enable_boost_unit_test_framework

TESTS_ENVIRONMENT =
TESTS = $(test_runners)

streams  = octet.utr
streams += null.utr
streams += memory.utr
streams += device.utr
streams += buffer.utr
streams += stream.utr
streams += file.utr

settings  = descriptor.utr
settings += quantity.utr
settings += string.utr
settings += value.utr
settings += option.utr

test_runners  = log-checking.utr
test_runners += log-no-check.utr
test_runners += run-time.utr
test_runners += scanner.utr
test_runners += $(streams)
test_runners += $(settings)
test_runners += monitor.utr

check_PROGRAMS  = $(test_runners)
check_PROGRAMS += connexion.utr
check_PROGRAMS += ipc-cnx

log_checking_utr_SOURCES   = log.cpp
log_checking_utr_CPPFLAGS  = $(AM_CPPFLAGS)
log_checking_utr_CPPFLAGS += -DUTSUSHI_LOG_ARGUMENT_COUNT_CHECK_ENABLED=true
log_no_check_utr_SOURCES   = log.cpp
log_no_check_utr_CPPFLAGS  = $(AM_CPPFLAGS)
log_no_check_utr_CPPFLAGS += -DUTSUSHI_LOG_ARGUMENT_COUNT_CHECK_ENABLED=false

AM_CPPFLAGS += -DLOCALEDIR="\"$(localedir)\""

LDADD  = ../libutsushi.la $(LIBUTSUSHI_LIBS)
##  FIXME: drop once usb connexion is a proper plugin
LDADD += ../../connexions/libcnx-usb.la

if have_libtiff
test_runners      += tiff.utr
tiff_utr_CXXFLAGS  = $(AM_CXXFLAGS)
tiff_utr_CXXFLAGS += $(LIBTIFF_CFLAGS)
tiff_utr_LDADD     = $(LDADD)
tiff_utr_LDADD    += $(LIBTIFF_LIBS)
if have_libmagic
tiff_utr_CXXFLAGS += $(LIBMAGIC_CFLAGS)
tiff_utr_LDADD    += $(LIBMAGIC_LIBS)
endif
endif

endif # enable_boost_unit_test_framework

EXTRA_DIST  = value.hpp
CLEANFILES  =

include $(top_srcdir)/include/boost-test.am

if enable_code_coverage
CLEANFILES += ../../outputs/*.gcno
CLEANFILES += ../../outputs/*.gcda
CLEANFILES += ../../outputs/*.gcov
endif
