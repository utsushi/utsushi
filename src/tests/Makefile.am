##  Makefile.am -- an automake template for Makefile.in
##  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
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
TESTS = $(check_PROGRAMS)

check_PROGRAMS  =
check_PROGRAMS += command.utr
check_PROGRAMS += main.utr
check_PROGRAMS += main-failure.utr

AM_LDFLAGS  += $(BOOST_LDFLAGS)

LDADD  =
LDADD += $(BOOST_FILESYSTEM_LIB)
LDADD += $(BOOST_IOSTREAMS_LIB)

##  During development it is not uncommon to make some changes to the
##  source code of a utility and then fire off a `make -k check` from
##  the `tests/` directory.  This target makes sure that each utility
##  has been built using the latest source code before running tests.

all-local:
	cd .. && $(MAKE) $(AM_MAKEFLAGS) check-pkglibexec-PROGRAMS

endif # enable_boost_unit_test_framework

CLEANFILES  =
EXTRA_DIST  =

include $(top_srcdir)/include/boost-test.am
