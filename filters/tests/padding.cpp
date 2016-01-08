//  padding.cpp -- unit tests for the padding filter implementation
//  Copyright (C) 2014, 2015  SEIKO EPSON CORPORATION
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

#include <string>

#include <boost/test/unit_test.hpp>

#include <utsushi/file.hpp>
#include <utsushi/stream.hpp>
#include <utsushi/test/memory.hpp>

#include "../padding.hpp"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

using namespace utsushi;
using _flt_::padding;

struct fixture
{
  fixture ()
    : name_("padding.out")
  {
  }

  ~fixture () { fs::remove (name_); }

  context::size_type expected_size () const
  {
    return ctx_.scan_size ();
  }

  context ctx_;
  std::string name_;
};

BOOST_FIXTURE_TEST_CASE (mono_width_height, fixture)
{
  context ctx (425, 700, context::MONO);
  ctx.width (425, 74);
  ctx.height (700, 4);

  BOOST_REQUIRE_NE (ctx.scan_size (), ctx.octets_per_image ());
  BOOST_REQUIRE_NE (ctx.scan_width (), ctx.octets_per_line ());
  BOOST_REQUIRE_NE (ctx.scan_height (), ctx.lines_per_image ());

  rawmem_idevice dev (ctx);
  idevice& idev (dev);

  stream str;
  str.push (make_shared< padding > ());
  str.push (make_shared< file_odevice > (name_));

  idev | str;

  BOOST_CHECK_EQUAL (ctx.scan_size (), fs::file_size (name_));
}

BOOST_FIXTURE_TEST_CASE (gray8_width_height, fixture)
{
  context ctx (425, 700, context::GRAY8);
  ctx.width (425, 74);
  ctx.height (700, 4);

  BOOST_REQUIRE_NE (ctx.scan_size (), ctx.octets_per_image ());
  BOOST_REQUIRE_NE (ctx.scan_width (), ctx.octets_per_line ());
  BOOST_REQUIRE_NE (ctx.scan_height (), ctx.lines_per_image ());

  rawmem_idevice dev (ctx);
  idevice& idev (dev);

  stream str;
  str.push (make_shared< padding > ());
  str.push (make_shared< file_odevice > (name_));

  idev | str;

  BOOST_CHECK_EQUAL (ctx.scan_size (), fs::file_size (name_));
}

BOOST_FIXTURE_TEST_CASE (gray16_width_height, fixture)
{
  context ctx (425, 700, context::GRAY16);
  ctx.width (425, 74);
  ctx.height (700, 4);

  BOOST_REQUIRE_NE (ctx.scan_size (), ctx.octets_per_image ());
  BOOST_REQUIRE_NE (ctx.scan_width (), ctx.octets_per_line ());
  BOOST_REQUIRE_NE (ctx.scan_height (), ctx.lines_per_image ());

  rawmem_idevice dev (ctx);
  idevice& idev (dev);

  stream str;
  str.push (make_shared< padding > ());
  str.push (make_shared< file_odevice > (name_));

  idev | str;

  BOOST_CHECK_EQUAL (ctx.scan_size (), fs::file_size (name_));
}

BOOST_FIXTURE_TEST_CASE (rgb8_width_height, fixture)
{
  context ctx (425, 700, context::RGB8);
  ctx.width (425, 74);
  ctx.height (700, 4);

  BOOST_REQUIRE_NE (ctx.scan_size (), ctx.octets_per_image ());
  BOOST_REQUIRE_NE (ctx.scan_width (), ctx.octets_per_line ());
  BOOST_REQUIRE_NE (ctx.scan_height (), ctx.lines_per_image ());

  rawmem_idevice dev (ctx);
  idevice& idev (dev);

  stream str;
  str.push (make_shared< padding > ());
  str.push (make_shared< file_odevice > (name_));

  idev | str;

  BOOST_CHECK_EQUAL (ctx.scan_size (), fs::file_size (name_));
}

BOOST_FIXTURE_TEST_CASE (rgb16_width_height, fixture)
{
  context ctx (425, 700, context::RGB16);
  ctx.width (425, 74);
  ctx.height (700, 4);

  BOOST_REQUIRE_NE (ctx.scan_size (), ctx.octets_per_image ());
  BOOST_REQUIRE_NE (ctx.scan_width (), ctx.octets_per_line ());
  BOOST_REQUIRE_NE (ctx.scan_height (), ctx.lines_per_image ());

  rawmem_idevice dev (ctx);
  idevice& idev (dev);

  stream str;
  str.push (make_shared< padding > ());
  str.push (make_shared< file_odevice > (name_));

  idev | str;

  BOOST_CHECK_EQUAL (ctx.scan_size (), fs::file_size (name_));
}

#include "utsushi/test/runner.ipp"
