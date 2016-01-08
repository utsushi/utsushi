##  Makefile.am -- an automake template for Makefile.in
##  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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
TESTS  = setter.utr
TESTS += grammar-formats.utr
TESTS += grammar.utr
TESTS += grammar-mechanics.utr
TESTS += udev-rules.utr

check_PROGRAMS  = setter.utr
check_PROGRAMS += grammar-formats.utr
check_PROGRAMS += grammar.utr
check_PROGRAMS += grammar-mechanics.utr
check_PROGRAMS += udev-rules.utr

AM_CPPFLAGS += -DESCI_GRAMMAR_TRACE=1
AM_LDFLAGS  += $(BOOST_LDFLAGS)

LDADD = ../libdrv-esci.la $(LIBUTSUSHI_LIBS)
grammar_formats_utr_LDADD   = $(LDADD) $(BOOST_FILESYSTEM_LIB)
grammar_utr_LDADD           = $(LDADD) $(BOOST_FILESYSTEM_LIB)
## FIXME: remove once usb connexion is a proper plugin
setter_utr_LDADD    = $(LDADD) ../../../connexions/libcnx-usb.la
grammar_formats_utr_LDADD   += ../../../connexions/libcnx-usb.la
grammar_utr_LDADD           += ../../../connexions/libcnx-usb.la
grammar_mechanics_utr_LDADD  = $(LDADD) ../../../connexions/libcnx-usb.la
udev_rules_utr_LDADD  = $(BOOST_UNIT_TEST_FRAMEWORK_LIB)
udev_rules_utr_LDADD += $(BOOST_FILESYSTEM_LIB)
udev_rules_utr_LDADD += $(BOOST_REGEX_LIB)

endif

EXTRA_DIST  = grammar-formats.tcs
EXTRA_DIST += grammar.tcs
CLEANFILES  =

include $(top_srcdir)/include/boost-test.am
