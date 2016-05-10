##  Makefile.am -- an automake template for Makefile.in
##  Copyright (C) 2012-2014  SEIKO EPSON CORPORATION
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

filterlibdir    = $(pkglibdir)
filterdatadir   = $(pkgdatadir)
filter_ldflags  = $(AM_LDFLAGS)
filter_ldflags += -export-dynamic

dist_pkglibexec_SCRIPTS =
pkglibexec_PROGRAMS     =

AM_INSTALLCHECK_STD_OPTIONS_EXEMPT  = $(pkglibexec_PROGRAMS)
AM_INSTALLCHECK_STD_OPTIONS_EXEMPT += $(dist_pkglibexec_SCRIPTS)

##  FIXME: split each filter into its own plugin
filterlib_LTLIBRARIES  = libflt-all.la

libflt_all_la_CPPFLAGS  = $(AM_CPPFLAGS)
libflt_all_la_CXXFLAGS  = $(AM_CXXFLAGS)
libflt_all_la_LDFLAGS   = $(filter_ldflags)
libflt_all_la_LIBADD    =
libflt_all_la_SOURCES   =
libflt_all_la_SOURCES  += g3fax.cpp
libflt_all_la_SOURCES  += g3fax.hpp
libflt_all_la_SOURCES  += image-skip.cpp
libflt_all_la_SOURCES  += image-skip.hpp
libflt_all_la_SOURCES  += padding.cpp
libflt_all_la_SOURCES  += padding.hpp
libflt_all_la_SOURCES  += pnm.cpp
libflt_all_la_SOURCES  += pnm.hpp
libflt_all_la_SOURCES  += shell-pipe.cpp
libflt_all_la_SOURCES  += shell-pipe.hpp
libflt_all_la_SOURCES  += threshold.cpp
libflt_all_la_SOURCES  += threshold.hpp
libflt_all_la_SOURCES  += $(pdf_filter)

if have_libjpeg
libflt_all_la_CXXFLAGS += $(LIBJPEG_CFLAGS)
libflt_all_la_LIBADD   += $(LIBJPEG_LIBS)
libflt_all_la_SOURCES  += jpeg.cpp
libflt_all_la_SOURCES  += jpeg.hpp
endif

if have_magick
libflt_all_la_SOURCES  += magick.cpp
libflt_all_la_SOURCES  += magick.hpp
endif				# have_magick

if have_libmagick_pp
pkglibexec_PROGRAMS += doc-locate

doc_locate_CPPFLAGS  = $(AM_CPPFLAGS)
doc_locate_CPPFLAGS += $(LIBMAGICK_PP_CFLAGS)
doc_locate_LDFLAGS   = $(LIBMAGICK_PP_LIBS) -lm

doc_locate_SOURCES = doc-locate.cpp

libflt_all_la_SOURCES   += autocrop.cpp
libflt_all_la_SOURCES   += autocrop.hpp
libflt_all_la_SOURCES   += deskew.cpp
libflt_all_la_SOURCES   += deskew.hpp
endif				# have_libmagick_pp

libflt_all_la_CPPFLAGS  += -DPKGLIBEXECDIR="\"$(pkglibexecdir)\""
libflt_all_la_SOURCES   += reorient.cpp
libflt_all_la_SOURCES   += reorient.hpp
dist_pkglibexec_SCRIPTS += get-text-orientation

pdf_filter  =
pdf_filter += pdf.cpp
pdf_filter += pdf.hpp
pdf_filter += pdf/array.cpp
pdf_filter += pdf/array.hpp
pdf_filter += pdf/dictionary.cpp
pdf_filter += pdf/dictionary.hpp
pdf_filter += pdf/object.cpp
pdf_filter += pdf/object.hpp
pdf_filter += pdf/primitive.cpp
pdf_filter += pdf/primitive.hpp
pdf_filter += pdf/writer.cpp
pdf_filter += pdf/writer.hpp

.PHONY: $(dist_pkglibexec_SCRIPTS)
