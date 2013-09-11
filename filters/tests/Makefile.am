##  Makefile.am -- an automake template for Makefile.in
##  Copyright (C) 2012  SEIKO EPSON CORPORATION
##
##  License: GPL-3.0+
##  Author : AVASYS CORPORATION
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
check_PROGRAMS += pnm.utr
check_PROGRAMS += lut.utr
check_PROGRAMS += threshold.utr

LDADD  = ../libflt-all.la
LDADD += ../../lib/libutsushi.la $(LIBUTSUSHI_LIBS)
##  FIXME: drop once usb connexion is a proper plugin
LDADD += ../../connexions/libcnx-usb.la

if have_libjpeg
check_PROGRAMS    += jpeg.utr
check_PROGRAMS    += pdf.utr
jpeg_utr_CXXFLAGS  = $(AM_CXXFLAGS)
jpeg_utr_CXXFLAGS += $(LIBJPEG_CFLAGS)
jpeg_utr_LDADD     = $(LDADD)
jpeg_utr_LDADD    += $(LIBJPEG_LIBS)
pdf_utr_CXXFLAGS   = $(AM_CXXFLAGS)
pdf_utr_CXXFLAGS  += $(LIBJPEG_CFLAGS)
pdf_utr_LDADD      = $(LDADD)
pdf_utr_LDADD     += $(LIBJPEG_LIBS)
if have_libmagic
jpeg_utr_CXXFLAGS += $(LIBMAGIC_CFLAGS)
jpeg_utr_LDADD    += $(LIBMAGIC_LIBS)
pdf_utr_CXXFLAGS += $(LIBMAGIC_CFLAGS)
pdf_utr_LDADD    += $(LIBMAGIC_LIBS)
endif
endif

endif # enable_boost_unit_test_framework

EXTRA_DIST  =
## EXTRA_DIST += data/A4-300-x-300.jpg
## EXTRA_DIST += data/A4-300-x-max.jpg
## EXTRA_DIST += data/A4-max-x-300.jpg
## EXTRA_DIST += data/A4-max-x-max.jpg

CLEANFILES  =

include $(top_srcdir)/include/boost-test.am
