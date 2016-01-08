//  reorient.cpp -- unit tests for the reorient filter implementation
//  Copyright (C) 2015  SEIKO EPSON CORPORATION
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

#include "../reorient.hpp"

#include <utsushi/file.hpp>
#include <utsushi/run-time.hpp>
#include <utsushi/stream.hpp>
#include <utsushi/test/tools.hpp>

#include <boost/assign/list_inserter.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test.hpp>

#include <list>
#include <string>
#include <utility>

namespace fs = boost::filesystem;

using namespace utsushi;
using _flt_::reorient;

static void
test_context_orientation (std::pair< context::orientation_type,
                          const std::string > tc)
{
  context::orientation_type expected (tc.first);
  fs::path name (tc.second);

  utsushi::test::suffix_test_case_name (name.stem ().string ());

  fs::path srcdir (getenv ("srcdir"));
  fs::path input  (srcdir / "data" / name);
  fs::path output (name.replace_extension ("out"));

  file_idevice dev (input.string ());
  idevice& idev (dev);

  BOOST_REQUIRE (context::undefined == idev.get_context ().orientation ());

  filter::ptr fp (make_shared< reorient > ());
  (*fp->options ())["rotate"] = "Auto";

  BOOST_REQUIRE (context::undefined == fp->get_context ().orientation ());

  stream  str;
  str.push (fp);
  str.push (make_shared< file_odevice > (output.string ()));

  idev | str;

  BOOST_CHECK (expected == fp->get_context ().orientation ());
  BOOST_CHECK_EQUAL (fs::file_size (input), fs::file_size (output));

  fs::remove (output);
}

bool
init_test_runner ()
{
  namespace but = ::boost::unit_test;

  std::list< std::pair< context::orientation_type, std::string > > args;
  boost::assign::push_back (args)
    (context::top_left    , "top-left.png")
    (context::left_bottom , "left-bottom.png")
    (context::bottom_right, "bottom-right.png")
    (context::right_top   , "right-top.png")
    ;

  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (test_context_orientation,
                                 args.begin (), args.end ()));

  const char *argv[] = { "test" };
  run_time rt (sizeof (argv) / sizeof (*argv), argv);

  return true;
}

#include "utsushi/test/runner.ipp"
