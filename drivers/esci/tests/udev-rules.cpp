//  udev-rules.cpp -- unit tests for udev rules files
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

#include <utsushi/regex.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/throw_exception.hpp>

#include <ios>
#include <sstream>
#include <string>

namespace bfs = boost::filesystem;
namespace but = boost::unit_test;

struct fixture
{
  bfs::ifstream file;
  std::string   line;

  fixture ()
  {
    bfs::path path (getenv ("srcdir"));
    path /= "..";
    path /= "utsushi-esci.rules";

    file.open (path);

    if (!file.is_open ())
      {
        std::stringstream ss;
        ss << "\n"
           << __FILE__ << ":"
           << __LINE__ << ": "
           << "failure opening: " << path;

        BOOST_THROW_EXCEPTION (std::ios_base::failure (ss.str ()));
      }
  }
};

BOOST_FIXTURE_TEST_CASE (lowercase_usb_product_ids, fixture)
{
  using namespace utsushi;

  regex re ("ATTRS\\{idProduct\\}==\"([0-9a-fA-F]{4})\"");
  smatch m;

  while (!getline (file, line).eof ())
    {
      if (!regex_search (line, m, re)) continue;

      BOOST_CHECK_MESSAGE (regex_match (m[1].str(), regex ("[0-9a-f]{4}")),
                           m.str ());
    }
}

#include "utsushi/test/runner.ipp"
