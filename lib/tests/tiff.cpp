//  tiff.cpp -- unit tests for the TIFF output implementation
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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

#include "../../outputs/tiff.hpp"

#include <boost/filesystem.hpp>

using namespace utsushi;

using _out_::tiff_odevice;

using boost::filesystem::remove;

struct fixture
{
  shared_ptr<setmem_idevice::generator> gen_;
  idevice::ptr iptr_;
  stream str_;

  fixture ()
    : gen_ (new const_generator (octet(0xac)))
    , iptr_ ()
    , str_ ()
#if !HAVE_LIBMAGIC
  {}
#else
    , cookie_ (magic_open (MAGIC_MIME_TYPE))
  {
    BOOST_REQUIRE_MESSAGE (cookie_,
                           "libmagic failed to create the magic cookie ("
                           << strerror (errno) << ")");
    BOOST_REQUIRE_MESSAGE (0 == magic_load (cookie_, NULL),
                           "libmagic failed to load its database ("
                           << magic_error (cookie_) << ")");
  }

  ~fixture ()
  {
    magic_close (cookie_);
  }

  magic_t cookie_;
#endif  /* HAVE_LIBMAGIC */
};

BOOST_FIXTURE_TEST_CASE (test_magic, fixture)
{
  context ctx (643, 487, context::RGB8);
  const std::string name ("tiff.out");

  iptr_ = make_shared< setmem_idevice > (gen_, ctx);
  str_.push (make_shared< tiff_odevice > (name));

  *iptr_ | str_;

#if HAVE_LIBMAGIC
  const char *mime = magic_file (cookie_, name.c_str ());
  BOOST_CHECK_EQUAL ("image/tiff", mime);
#endif  /* HAVE_LIBMAGIC */

  remove (name);
}

BOOST_FIXTURE_TEST_CASE (test_magic_multipage, fixture)
{
  context ctx (643, 487, context::MONO);
  path_generator pathgen ("tiff-%3i.out");
  const unsigned images = 11;

  iptr_ = make_shared< setmem_idevice > (gen_, ctx, images);
  str_.push (make_shared< tiff_odevice > (pathgen));

  *iptr_ | str_;

  for (unsigned i = 0; i < images; ++i)
    {
      std::string p = pathgen ();

#if HAVE_LIBMAGIC
      const char *mime = magic_file (cookie_, p.c_str ());
      BOOST_CHECK_EQUAL ("image/tiff", mime);
#endif  /* HAVE_LIBMAGIC */

      remove (p);
    }
}

#include "utsushi/test/runner.ipp"
