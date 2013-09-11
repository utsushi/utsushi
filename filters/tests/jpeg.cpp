//  jpeg.cpp -- unit tests for the JPEG filter implementation
//  Copyright (C) 2012, 2013  SEIKO EPSON CORPORATION
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

#include <list>

#include <boost/assign/list_inserter.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test.hpp>

#include <utsushi/file.hpp>
#include <utsushi/stream.hpp>
#include <utsushi/test/memory.hpp>
#include <utsushi/test/tools.hpp>

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

class jpeg_idevice
  : public file_idevice
{
  int cnt_;

public:
  jpeg_idevice (const fs::path& name, int width, int height, int cnt = 1)
    : file_idevice (name)
    , cnt_(cnt)
  {
    test_magic (name, "image/jpeg");

    ctx_ = context (width, height, context::RGB8);
    ctx_.resolution (300, 300);
    ctx_.content_type ("image/jpeg");
  }
  bool is_consecutive () const { return true; }
  bool obtain_media () { return cnt_-- > 0; }
};

BOOST_AUTO_TEST_SUITE (compressor);

BOOST_FIXTURE_TEST_CASE (mediatype, fixture)
{
  istream istr;
  ostream ostr;

  istr.push (make_shared< rawmem_idevice > (context (32, 32, 3, 8)));
  ostr.push (make_shared< jpeg::compressor > ());
  ostr.push (make_shared< file_odevice > (name_));

  istr | ostr;

  test_magic (name_, "image/jpeg");
}

BOOST_AUTO_TEST_SUITE_END (/* compressor */);

BOOST_AUTO_TEST_SUITE (decompressor);

BOOST_FIXTURE_TEST_CASE (mediatype, fixture)
{
  istream istr;
  ostream ostr;

  istr.push (make_shared< rawmem_idevice > (context (32, 32, 3, 8)));
  ostr.push (make_shared< jpeg::compressor > ());
  ostr.push (make_shared< jpeg::decompressor > ());
  ostr.push (make_shared< pnm > ());
  ostr.push (make_shared< file_odevice > (name_));

  istr | ostr;

  test_magic (name_, "image/x-portable-pixmap");
}

BOOST_AUTO_TEST_SUITE_END (/* decompressor */);

struct file_spec
{
  file_spec (const fs::path& input_file, uintmax_t width, uintmax_t height,
             int count = 1)
    : input_file_(input_file)
    , width_(width)
    , height_(height)
    , count_(count)
  {
    expected_  = 3 * width * height;
    expected_ += 3;             // "P6 "
    do { ++expected_; } while (width /= 10);
    ++expected_;
    do { ++expected_; } while (height /= 10);
    ++expected_;
    expected_ += 4;             // "255\n"
    expected_ *= count;
  }

  fs::path  input_file_;
  uintmax_t width_;
  uintmax_t height_;
  int count_;
  uintmax_t expected_;
};

static void
test_decompressor (file_spec t)
{
  utsushi::test::change_test_case_name
    ("decompressor_" + t.input_file_.filename ().string ());

  const fs::path output_file (t.input_file_.stem ().replace_extension ("pnm"));

  istream istr;
  ostream ostr;

  istr.push (make_shared< jpeg_idevice > (t.input_file_, t.width_, t.height_,
                                          t.count_));
  ostr.push (make_shared< jpeg::decompressor > ());
  ostr.push (make_shared< pnm > ());
  ostr.push (make_shared< file_odevice > (output_file));

  istr | ostr;

  test_magic (output_file, "image/x-portable-pixmap");

  BOOST_CHECK_EQ (t.expected_, file_size (output_file));

  remove (output_file);
}

static void
test_idecompressor (file_spec t)
{
  utsushi::test::change_test_case_name
    ("idecompressor_" + t.input_file_.filename ().string ());

  const fs::path output_file (t.input_file_.stem ().replace_extension ("pnm"));

  istream istr;
  ostream ostr;

  istr.push (make_shared< jpeg::idecompressor > ());
  istr.push (make_shared< jpeg_idevice > (t.input_file_, t.width_, t.height_,
                                          t.count_));
  ostr.push (make_shared< pnm > ());
  ostr.push (make_shared< file_odevice > (output_file));

  istr | ostr;

  test_magic (output_file, "image/x-portable-pixmap");

  BOOST_CHECK_EQ (t.expected_, file_size (output_file));

  remove (output_file);
}

bool
init_test_runner ()
{
  namespace but = ::boost::unit_test;

  fs::path srcdir (getenv ("srcdir"));

  std::list< file_spec > args;
  boost::assign::push_back (args)
    // // single image scan sequence tests
    // (srcdir / "data/A4-max-x-max.jpg", 2550, 3513)
    // (srcdir / "data/A4-max-x-300.jpg", 2550,  300)
    // (srcdir / "data/A4-300-x-max.jpg",  300, 3489)
    // (srcdir / "data/A4-300-x-300.jpg",  300,  300)
    // // multi image scan sequence tests
    // (srcdir / "data/A4-max-x-max.jpg", 2550, 3513, 2)
    // (srcdir / "data/A4-max-x-300.jpg", 2550,  300, 3)
    // (srcdir / "data/A4-300-x-max.jpg",  300, 3489, 4)
    // (srcdir / "data/A4-300-x-300.jpg",  300,  300, 5)
    ;

  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (test_decompressor,
                                 args.begin (), args.end ()));
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (test_idecompressor,
                                 args.begin (), args.end ()));
  return true;
}

#include "utsushi/test/runner.ipp"
