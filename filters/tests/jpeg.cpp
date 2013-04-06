//  jpeg.cpp -- unit tests for the JPEG filter implementation
//  Copyright (C) 2012  SEIKO EPSON CORPORATION
//
//  License: GPL-3.0+
//  Author : AVASYS CORPORATION
//
//  This file is part of the 'Utsushi' package.
//  This package is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License or, at
//  your option, any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//  You ought to have received a copy of the GNU General Public License
//  along with this package.  If not, see <http://www.gnu.org/licenses/>.


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_LIBMAGIC
#include <magic.h>
#include <cerrno>
#include <cstring>
#endif

#include <boost/test/unit_test.hpp>

#include <utsushi/file.hpp>
#include <utsushi/stream.hpp>
#include <utsushi/test/memory.hpp>

#include "../jpeg.hpp"
#include "../pnm.hpp"

using namespace utsushi;
using namespace _flt_;

struct fixture
{
  fixture () : name_("jpeg.out") {}
  ~fixture () { remove (name_); }

  const fs::path name_;
};

static void
test_magic (const fs::path& name, const char *type)
{
#if HAVE_LIBMAGIC

  magic_t cookie = magic_open (MAGIC_MIME_TYPE);

  BOOST_REQUIRE_MESSAGE (cookie,
                         "libmagic failed to create the magic cookie ("
                         << strerror (errno) << ")");
  BOOST_REQUIRE_MESSAGE (0 == magic_load (cookie, NULL),
                         "libmagic failed to load its database ("
                         << magic_error (cookie) << ")");

  const char *mime = magic_file (cookie, name.string ().c_str ());

  BOOST_CHECK_EQUAL (type, mime);

  magic_close (cookie);

#endif  /* HAVE_LIBMAGIC */
}

BOOST_AUTO_TEST_SUITE (compressor);

BOOST_FIXTURE_TEST_CASE (mediatype, fixture)
{
  istream istr;
  ostream ostr;

  istr.push (idevice::ptr (new rawmem_idevice (context (32, 32, 3, 8))));
  ostr.push (ofilter::ptr (new jpeg::compressor));
  ostr.push (odevice::ptr (new file_odevice (name_)));

  istr | ostr;

  test_magic (name_, "image/jpeg");
}

BOOST_AUTO_TEST_SUITE_END (/* compressor */);

BOOST_AUTO_TEST_SUITE (decompressor);

BOOST_FIXTURE_TEST_CASE (mediatype, fixture)
{
  istream istr;
  ostream ostr;

  istr.push (idevice::ptr (new rawmem_idevice (context (32, 32, 3, 8))));
  ostr.push (ofilter::ptr (new jpeg::compressor));
  ostr.push (ofilter::ptr (new jpeg::decompressor));
  ostr.push (ofilter::ptr (new pnm));
  ostr.push (odevice::ptr (new file_odevice (name_)));

  istr | ostr;

  test_magic (name_, "image/x-portable-pixmap");
}

BOOST_AUTO_TEST_SUITE_END (/* decompressor */);

#include "utsushi/test/runner.ipp"
