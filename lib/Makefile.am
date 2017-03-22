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

SUBDIRS  = .
SUBDIRS += tests

EXTRA_DIST  =
EXTRA_DIST += devices.conf

streams  = octet.cpp
streams += context.cpp
streams += iobase.cpp
streams += device.cpp
streams += filter.cpp
streams += buffer.cpp
streams += stream.cpp
streams += pump.cpp

settings  = constraint.cpp
settings += descriptor.cpp
settings += key.cpp
settings += media.cpp
settings += option.cpp
settings += preset.cpp
settings += quantity.cpp
settings += range.cpp
settings += store.cpp
settings += string.cpp
settings += tag.cpp
settings += toggle.cpp
settings += value.cpp

##  The outputs listed here are candidates for removal from the core
##  library and packaged as plugins.
outputs  =
outputs += file.cpp
outputs += ../utsushi/file.hpp

if have_libtiff
outputs += ../outputs/tiff.cpp
outputs += ../outputs/tiff.hpp
endif

pkglib_LTLIBRARIES = libutsushi.la
libutsushi_la_LIBADD   = $(LIBUTSUSHI_LIBS)
libutsushi_la_LIBADD  += $(LIBLTDL)
libutsushi_la_LIBADD  += $(LTLIBINTL)
libutsushi_la_LIBADD  += $(LIBUDEV_LIBS)
libutsushi_la_SOURCES  = connexion.cpp
libutsushi_la_SOURCES += device-info.cpp
libutsushi_la_SOURCES += exception.cpp
libutsushi_la_SOURCES += log.cpp
libutsushi_la_SOURCES += monitor.cpp
libutsushi_la_SOURCES += run-time.cpp
libutsushi_la_SOURCES += run-time.ipp
libutsushi_la_SOURCES += scanner.cpp
libutsushi_la_SOURCES += $(streams)
libutsushi_la_SOURCES += $(settings)
libutsushi_la_SOURCES += $(outputs)

if have_libtiff
libutsushi_la_LIBADD  += $(LIBTIFF_LIBS)
endif

if have_libudev
libutsushi_la_SOURCES += udev.cpp
libutsushi_la_SOURCES += udev.hpp
endif

AM_CPPFLAGS += $(LTDLINCL)
## FIXME: These *DIR defines should be removed.  The run-time should
## get all these locations from a configuration file so stuff can be
## moved around without the need for recompilation.
AM_CPPFLAGS += -DPKGLIBEXECDIR="\"$(pkglibexecdir)\""
AM_CPPFLAGS += -DPKGLIBDIR="\"$(pkglibdir)\""
AM_CPPFLAGS += -DPKGDATADIR="\"$(pkgdatadir)\""
AM_CPPFLAGS += -DLOCALEDIR="\"$(localedir)\""
AM_CPPFLAGS += -DPKGSYSCONFDIR="\"$(pkgsysconfdir)\""
AM_CPPFLAGS += -DPKGCONFFILE="\"$(PACKAGE_TARNAME).conf\""
AM_CPPFLAGS += -DCOMBOCONFFILE="\"combo.conf\""
AM_CXXFLAGS += $(LIBUDEV_CFLAGS)
## FIXME: remove once the USB connexion has become a proper plugin
AM_CXXFLAGS += $(LIBUSB_CFLAGS)
AM_CXXFLAGS += $(LIBTIFF_CFLAGS)
