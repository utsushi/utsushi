//  main-failure.cpp -- tests of the entry point to the software
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
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

#include <cstdlib>

#include <string>

#include <boost/filesystem.hpp>

#include <boost/test/unit_test.hpp>

#include "utsushi/test/catch-system-errors.hpp"
#include "utsushi/test/command-line.hpp"

namespace fs = boost::filesystem;
namespace ut = boost::unit_test;

namespace {

using utsushi::test::catch_system_errors_no;

BOOST_GLOBAL_FIXTURE (catch_system_errors_no);

using std::string;

typedef std::istream::traits_type traits;

struct program
  : utsushi::test::command_line
{
  program ()
    : command_line ((fs::path ("..") / "main").string ())
  {
    fs::path p (ut::framework::master_test_suite ().argv[0]);
    string ext = p.extension ().string ();

    if (".utr" != ext) command_ += ext;
  }
};

BOOST_FIXTURE_TEST_CASE (unsupported_command, program)
{
  const string option ("unsupported-command");
  *this += option;

  BOOST_CHECK_EQUAL (EXIT_FAILURE, execute ());

  BOOST_CHECK_MESSAGE (traits::eof () == out ().peek (),
                       "empty stdout");
  BOOST_CHECK_MESSAGE (traits::eof () != err ().peek (),
                       "non-empty stderr");
}

} // namespace

#include "utsushi/test/runner.ipp"
