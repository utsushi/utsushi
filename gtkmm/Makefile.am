##  Makefile.am -- an automake template for Makefile.in
##  Copyright (C) 2012, 2014, 2015  SEIKO EPSON CORPORATION
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

gtkmmlibdir  = $(pkglibdir)
gtkmmdatadir = $(pkgdatadir)/gtkmm

gtkmmlib_LTLIBRARIES = libutsushi-gtkmm.la

libutsushi_gtkmm_la_LIBADD  = ../lib/libutsushi.la
libutsushi_gtkmm_la_LIBADD += $(LIBGTKMM_LIBS)
libutsushi_gtkmm_la_LIBADD += $(BOOST_THREAD_LIB)
libutsushi_gtkmm_la_SOURCES  =
libutsushi_gtkmm_la_SOURCES += action-dialog.cpp
libutsushi_gtkmm_la_SOURCES += action-dialog.hpp
libutsushi_gtkmm_la_SOURCES += chooser.cpp
libutsushi_gtkmm_la_SOURCES += chooser.hpp
libutsushi_gtkmm_la_SOURCES += dialog.cpp
libutsushi_gtkmm_la_SOURCES += dialog.hpp
libutsushi_gtkmm_la_SOURCES += dropdown.cpp
libutsushi_gtkmm_la_SOURCES += dropdown.hpp
libutsushi_gtkmm_la_SOURCES += editor.cpp
libutsushi_gtkmm_la_SOURCES += editor.hpp
libutsushi_gtkmm_la_SOURCES += file-chooser.cpp
libutsushi_gtkmm_la_SOURCES += file-chooser.hpp
libutsushi_gtkmm_la_SOURCES += presets.cpp
libutsushi_gtkmm_la_SOURCES += presets.hpp
libutsushi_gtkmm_la_SOURCES += preview.cpp
libutsushi_gtkmm_la_SOURCES += preview.hpp
libutsushi_gtkmm_la_SOURCES += pump.cpp
libutsushi_gtkmm_la_SOURCES += pump.hpp

AM_CXXFLAGS += $(LIBGTKMM_CFLAGS)
AM_LDFLAGS  += $(BOOST_LDFLAGS)

  dist_gtkmmdata_DATA  =
  dist_gtkmmdata_DATA += dialog.rc
  dist_gtkmmdata_DATA += dialog.glade
nodist_gtkmmdata_DATA  = about.xml

CLEANFILES  =
if enable_code_coverage
CLEANFILES += *.gcno
CLEANFILES += *.gcda
CLEANFILES += *.gcov
endif
