//  jpeg.cpp -- unit tests for the JPEG filter implementation
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

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

using namespace utsushi;
using namespace _flt_;

struct fixture
{
  fixture () : name_("jpeg.out") {}
  ~fixture () { fs::remove (name_); }

  const std::string name_;
};

static void
test_magic (const std::string& name, const char *type)
{
#if HAVE_LIBMAGIC

  magic_t cookie = magic_open (MAGIC_MIME_TYPE);

  BOOST_REQUIRE_MESSAGE (cookie,
                         "libmagic failed to create the magic cookie ("
                         << strerror (errno) << ")");
  BOOST_REQUIRE_MESSAGE (0 == magic_load (cookie, NULL),
                         "libmagic failed to load its database ("
                         << magic_error (cookie) << ")");

  const char *mime = magic_file (cookie, name.c_str ());

  BOOST_CHECK_EQUAL (type, mime);

  magic_close (cookie);

#endif  /* HAVE_LIBMAGIC */
}

class jpeg_idevice
  : public file_idevice
{
  int cnt_;

public:
  jpeg_idevice (const std::string& name, int width, int height, int cnt = 1)
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
  rawmem_idevice dev (context (32, 32));
  idevice& idev (dev);

  stream str;
  str.push (make_shared< jpeg::compressor > ());
  str.push (make_shared< file_odevice > (name_));

  idev | str;

  test_magic (name_, "image/jpeg");
}

BOOST_AUTO_TEST_SUITE_END (/* compressor */);

BOOST_AUTO_TEST_SUITE (decompressor);

BOOST_FIXTURE_TEST_CASE (mediatype, fixture)
{
  rawmem_idevice dev (context (32, 32));
  idevice& idev (dev);

  stream str;
  str.push (make_shared< jpeg::compressor > ());
  str.push (make_shared< jpeg::decompressor > ());
  str.push (make_shared< pnm > ());
  str.push (make_shared< file_odevice > (name_));

  idev | str;

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

  const std::string output_file (t.input_file_.stem ()
                                 .replace_extension ("pnm").string ());

  jpeg_idevice dev (t.input_file_.string (), t.width_, t.height_, t.count_);
  idevice& idev (dev);

  stream str;
  str.push (make_shared< jpeg::decompressor > ());
  str.push (make_shared< pnm > ());
  str.push (make_shared< file_odevice > (output_file));

  idev | str;

  test_magic (output_file, "image/x-portable-pixmap");

  BOOST_CHECK_EQ (t.expected_, fs::file_size (output_file));

  fs::remove (output_file);
}

bool
init_test_runner ()
{
  namespace but = ::boost::unit_test;

  fs::path srcdir (getenv ("srcdir"));

  std::list< file_spec > args;
  boost::assign::push_back (args)
    // single image scan sequence tests
    (srcdir / "data" / "A4-max-x-max.jpg", 2550, 3513)
    (srcdir / "data" / "A4-max-x-300.jpg", 2550,  300)
    (srcdir / "data" / "A4-300-x-max.jpg",  300, 3489)
    (srcdir / "data" / "A4-300-x-300.jpg",  300,  300)
    // multi image scan sequence tests
    (srcdir / "data" / "A4-max-x-max.jpg", 2550, 3513, 2)
    (srcdir / "data" / "A4-max-x-300.jpg", 2550,  300, 3)
    (srcdir / "data" / "A4-300-x-max.jpg",  300, 3489, 4)
    (srcdir / "data" / "A4-300-x-300.jpg",  300,  300, 5)
    ;

  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (test_decompressor,
                                 args.begin (), args.end ()));
  return true;
}

#include "utsushi/test/runner.ipp"
