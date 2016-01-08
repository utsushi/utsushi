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

connexionlibdir    = $(pkglibdir)
connexiondatadir   = $(pkgdatadir)
connexion_ldflags  = $(AM_LDFLAGS)
connexion_ldflags += -export-dynamic
connexion_ldflags += -export-symbols-regex

connexionlib_LTLIBRARIES  = libcnx-usb.la
connexionlib_LTLIBRARIES += libcnx-hexdump.la

libcnx_usb_la_CPPFLAGS  = $(AM_CPPFLAGS)
libcnx_usb_la_CXXFLAGS  = $(AM_CXXFLAGS)
libcnx_usb_la_CXXFLAGS += $(LIBUSB_CFLAGS)
libcnx_usb_la_LDFLAGS   = $(connexion_ldflags) libcnx_usb_LTX_factory
libcnx_usb_la_LIBADD    = $(LIBUSB_LIBS)
libcnx_usb_la_LIBADD   += ../lib/libutsushi.la
libcnx_usb_la_SOURCES   = usb.cpp
libcnx_usb_la_SOURCES  += usb.hpp

libcnx_hexdump_la_LDFLAGS  = $(connexion_ldflags) libcnx_hexdump_LTX_factory
libcnx_hexdump_la_SOURCES  = hexdump.cpp
libcnx_hexdump_la_SOURCES += hexdump.hpp
libcnx_hexdump_la_LIBADD   = ../lib/libutsushi.la

CLEANFILES  =
if enable_code_coverage
CLEANFILES += *.gcno
CLEANFILES += *.gcda
CLEANFILES += *.gcov
endif
