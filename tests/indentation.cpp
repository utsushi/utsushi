//  indentation.cpp -- conformance test suite
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2013  Olaf Meeuwissen
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

#include <cassert>              // FIXME: drop this
#include <cstdlib>

#include <algorithm>
#include <iterator>
#include <vector>

#include <boost/filesystem.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>

#include "utsushi/regex.hpp"
#include "utsushi/test/catch-system-errors.hpp"
#include "utsushi/test/command-line.hpp"

namespace fs = boost::filesystem;
namespace ut = boost::unit_test;

using utsushi::regex;
using utsushi::regex_match;
using utsushi::test::catch_system_errors_no;
using utsushi::test::command_line;

namespace {

BOOST_GLOBAL_FIXTURE (catch_system_errors_no);

static
void
test_indentation_conformance (const fs::path& p)
{
  BOOST_REQUIRE (getenv ("srcdir"));

  fs::path config (getenv ("srcdir"));
  config /= "uncrustify.cfg";

  BOOST_REQUIRE (fs::is_regular_file (config));

  command_line indent ("uncrustify -c " + config.string ()
                       + " -f " + p.string ()
                       + " -l CPP | diff -q "
                       + p.string () + " -");
  if (EXIT_SUCCESS != indent.execute ())
    {
      BOOST_WARN_MESSAGE (false, p.string () << " is not conformant");
      BOOST_TEST_MESSAGE (indent.out ().rdbuf ());
    }
}

static
bool
is_not_source_file (const fs::path& p)
{
  static const regex re (".*\\.[chi]pp");

  return !regex_match (p.string (), re);
}

static
bool
init_test_runner ()
{
  using namespace std;

  assert (getenv ("srcdir"));

  fs::path srcdir (getenv ("srcdir"));
  fs::path top_srcdir (srcdir / "..");
  fs::path vc_list_files (top_srcdir / "upstream/tools/vc-list-files");
  fs::path vc_dist_files (srcdir / "vc-dist-files");

  assert (fs::is_regular_file (vc_dist_files));

  command_line file_list (vc_list_files.string ()
                          + " -C " + top_srcdir.string ()
                          + " || sed 's|^\\\\.\\\\./|" + top_srcdir.string ()
                          + "/|' " + vc_dist_files.string ());
  vector< fs::path > source_file;

  assert (EXIT_SUCCESS == file_list.execute ());

  remove_copy_if (istream_iterator< fs::path > (file_list.out ()),
                  istream_iterator< fs::path > (),
                  back_inserter (source_file), is_not_source_file);

  sort (source_file.begin (), source_file.end ());

  assert (!source_file.empty ());

  ut::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (test_indentation_conformance,
                                 source_file.begin (), source_file.end ()));

  return true;
}

} // namespace

#include "utsushi/test/runner.ipp"
