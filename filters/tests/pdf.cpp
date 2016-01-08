//  pdf.cpp -- unit tests for the PDF filter implementation
//  Copyright (C) 2012-2014  SEIKO EPSON CORPORATION
//  Copyright (C) 2015  Olaf Meeuwissen
//
//  License: GPL-3.0+
//  Author : EPSON AVASYS CORPORATION
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
#include "../pdf.hpp"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

using namespace utsushi;
using namespace _flt_;

struct fixture
{
  fixture () : name_("pdf.out") {}
  ~fixture () { fs::remove (name_); }

  const std::string name_;
};

BOOST_FIXTURE_TEST_CASE (test_magic, fixture)
{
  context ctx (32, 48);
  shared_ptr<setmem_idevice::generator> gen
    = make_shared< const_generator > (0x50);

  setmem_idevice dev (gen, ctx, 10);
  idevice& idev (dev);

  stream str;
  str.push (make_shared< jpeg::compressor > ());
  str.push (make_shared< pdf > ());
  str.push (make_shared< file_odevice > (name_));

  idev | str;

#if HAVE_LIBMAGIC

  magic_t cookie = magic_open (MAGIC_MIME_TYPE);

  BOOST_REQUIRE_MESSAGE (cookie,
                         "libmagic failed to create the magic cookie ("
                         << strerror (errno) << ")");
  BOOST_REQUIRE_MESSAGE (0 == magic_load (cookie, NULL),
                         "libmagic failed to load its database ("
                         << magic_error (cookie) << ")");

  const char *mime = magic_file (cookie, name_.c_str ());

  BOOST_CHECK_EQUAL ("application/pdf", mime);

  magic_close (cookie);

#endif  /* HAVE_LIBMAGIC */
}

#include "utsushi/test/runner.ipp"
