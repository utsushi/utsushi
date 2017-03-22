##  Makefile.am -- an automake template for Makefile.in
##  Copyright (C) 2017  SEIKO EPSON CORPORATION
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

AM_LDFLAGS  += $(BOOST_LDFLAGS)

driver = combo

driverlibdir    = $(pkglibdir)
driverdatadir   = $(pkgsysconfdir)
driver_ldflags  = $(AM_LDFLAGS)
driver_ldflags += -export-dynamic
driver_ldflags += -export-symbols-regex libdrv_$(driver)_LTX_factory

driverlib_LTLIBRARIES = libdrv-combo.la

libdrv_combo_la_LDFLAGS  = $(driver_ldflags)
libdrv_combo_la_LIBADD   = ../lib/libutsushi.la
libdrv_combo_la_SOURCES  = combo.cpp
libdrv_combo_la_SOURCES += combo.hpp

driverdata_DATA  =
driverdata_DATA += combo.conf

EXTRA_DIST = $(driverdata_DATA)
