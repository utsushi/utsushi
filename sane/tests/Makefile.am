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

BACKEND_NAME = utsushi

TESTS_ENVIRONMENT =
TESTS = $(test_runners)

test_runners  = value.utr

check_PROGRAMS  = backend.utr
check_PROGRAMS += $(test_runners)

AM_CPPFLAGS += -DBACKEND_NAME=$(BACKEND_NAME)
AM_LDFLAGS  += ../libsane-$(BACKEND_NAME).la

AM_CPPFLAGS += $(INCLTDL)
AM_LDFLAGS  += $(LIBLTDL)

LDADD  = ../../lib/libutsushi.la $(LIBUTSUSHI_LIBS)
##  FIXME: drop when filters have been turned into proper plugins
LDADD += ../../filters/libflt-all.la
##  FIXME: drop when connexions have been turned into proper plugins
LDADD += ../../connexions/libcnx-usb.la

endif # enable_boost_unit_test_framework

EXTRA_DIST   = sane-api.ipp
CLEANFILES   =

include $(top_srcdir)/include/boost-test.am
