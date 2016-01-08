//  magick.cpp -- unit tests for the magick filter implementation
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

#include "../magick.hpp"

#include <utsushi/context.hpp>
#include <utsushi/file.hpp>
#include <utsushi/stream.hpp>
#include <utsushi/test/memory.hpp>
#include <utsushi/test/tools.hpp>

#include <boost/assign/list_inserter.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test.hpp>

#include <string>

namespace fs = boost::filesystem;

using namespace utsushi;
using _flt_::magick;

struct resample_argv
{
  resample_argv (context::size_type i, context::size_type o, std::string n)
    : i_res (i)
    , o_res (o)
    , name (n)
  {}

  context::size_type i_res;
  context::size_type o_res;
  std::string name;
};

//! Resample a square inch gray image
void
test_resample (const resample_argv& arg)
{
  test::suffix_test_case_name (arg.name);

  context ctx (arg.i_res, arg.i_res, context::GRAY8);
  ctx.resolution (arg.i_res);
  rawmem_idevice dev (ctx);
  idevice& idev (dev);

  filter::ptr flt = make_shared< magick > ();
  (*flt->options ())["resolution-x"] = quantity::integer_type (arg.o_res);
  (*flt->options ())["resolution-y"] = quantity::integer_type (arg.o_res);

  stream str;
  std::string output ("magick-resample.out");

  str.push (flt);
  str.push (make_shared< file_odevice > (output));

  idev | str;

  ctx.width (arg.o_res);
  ctx.height (arg.o_res);
  ctx.resolution (arg.o_res);

  BOOST_CHECK (fs::exists (output));
  BOOST_CHECK_EQUAL (fs::file_size (output), ctx.octets_per_image ());

  fs::remove (output);
}

BOOST_AUTO_TEST_CASE (independent_resolutions)
{
  context ctx (200, 300, context::GRAY8);
  ctx.resolution (200, 300);

  rawmem_idevice dev (ctx);
  idevice& idev (dev);

  filter::ptr flt = make_shared< magick > ();
  (*flt->options ())["resolution-x"] = quantity::integer_type (400);
  (*flt->options ())["resolution-y"] = quantity::integer_type (500);

  stream str;
  std::string output ("magick-independent-resolutions.out");

  str.push (flt);
  str.push (make_shared< file_odevice > (output));

  idev | str;

  ctx.width (400);
  ctx.height (500);
  ctx.resolution (400, 500);

  BOOST_CHECK (fs::exists (output));
  BOOST_CHECK_EQUAL (fs::file_size (output), ctx.octets_per_image ());

  fs::remove (output);
}

BOOST_AUTO_TEST_CASE (force_extent)
{
  context ctx (200, 300, context::GRAY8);
  ctx.resolution (200, 300);

  rawmem_idevice dev (ctx);
  idevice& idev (dev);

  filter::ptr flt = make_shared< magick > ();
  (*flt->options ())["resolution-x"] = quantity::integer_type (400);
  (*flt->options ())["resolution-y"] = quantity::integer_type (500);
  (*flt->options ())["force-extent"] = toggle (true);
  (*flt->options ())["width"]  = quantity::non_integer_type (500./400);
  (*flt->options ())["height"] = quantity::non_integer_type (600./500);

  stream str;
  std::string output ("magick-force-extent.out");

  str.push (flt);
  str.push (make_shared< file_odevice > (output));

  idev | str;

  ctx.width (500);
  ctx.height (600);
  ctx.resolution (400, 500);

  BOOST_CHECK (fs::exists (output));
  BOOST_CHECK_EQUAL (fs::file_size (output), ctx.octets_per_image ());

  fs::remove (output);
}

BOOST_AUTO_TEST_CASE (auto_orient)
{
  context ctx (200, 300, context::GRAY8);
  ctx.orientation (context::right_top);

  rawmem_idevice dev (ctx);
  idevice& idev (dev);

  filter::ptr flt = make_shared< magick > ();
  (*flt->options ())["auto-orient"] = toggle (true);
  (*flt->options ())["image-format"] = "PNM";

  stream str;
  std::string output ("magick-auto-orient.pnm");

  str.push (flt);
  str.push (make_shared< file_odevice > (output));

  idev | str;

  BOOST_REQUIRE (fs::exists (output));

  fs::ifstream img (output);
  std::string line;

  BOOST_REQUIRE (!getline (img, line).eof ()); // PNM magic "P?"
  BOOST_REQUIRE (!getline (img, line).eof ()); // PNM size
  BOOST_CHECK_EQUAL ("300 200", line);

  fs::remove (output);
}

bool
init_test_runner ()
{
  namespace but = ::boost::unit_test;

  std::list< resample_argv > args;
  boost::assign::push_back (args)
    (200, 300, "up")
    (300, 200, "down")
    ;

  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (test_resample,
                                 args.begin (), args.end ()));

  return true;
}

#include "utsushi/test/runner.ipp"
