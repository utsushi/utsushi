//  scanner.cpp -- unit tests for the utsushi::scanner API
//  Copyright (C) 2012  SEIKO EPSON CORPORATION
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

#include <boost/test/unit_test.hpp>

#include "utsushi/scanner.hpp"

using namespace utsushi;

BOOST_AUTO_TEST_CASE (udi_validation)
{
  const std::string digit = "0123456789";
  const std::string punct = "!\"#$%&'()~=~|-^\\`{+*}<>?_@[;:],./";

  BOOST_CHECK_NO_THROW (scanner::id ("", "", "if::"));
  BOOST_CHECK_NO_THROW (scanner::id ("", "", "if:drv:"));
  BOOST_CHECK_NO_THROW (scanner::id ("", "", "if::path"));
  BOOST_CHECK_NO_THROW (scanner::id ("", "", "if:drv:path"));
  BOOST_CHECK_NO_THROW (scanner::id ("", "", "if::" + digit));
  BOOST_CHECK_NO_THROW (scanner::id ("", "", "if::" + punct));

  BOOST_CHECK_THROW (scanner::id ("", "", "::"),
                     std::invalid_argument);
  BOOST_CHECK_THROW (scanner::id ("", "", ":drv:"),
                     std::invalid_argument);
  BOOST_CHECK_THROW (scanner::id ("", "", ":drv:path"),
                     std::invalid_argument);
  BOOST_CHECK_THROW (scanner::id ("", "", "if\\8::"),
                     std::invalid_argument);
  BOOST_CHECK_THROW (scanner::id ("", "", "if\\8:drv:"),
                     std::invalid_argument);
  BOOST_CHECK_THROW (scanner::id ("", "", "if\\8:drv:path"),
                     std::invalid_argument);
  BOOST_CHECK_THROW (scanner::id ("", "", "if:drv\\8:"),
                     std::invalid_argument);
  BOOST_CHECK_THROW (scanner::id ("", "", "if:drv\\8:path"),
                     std::invalid_argument);

  BOOST_CHECK_THROW (scanner::id ("", "", "path", ""),
                     std::invalid_argument);
  BOOST_CHECK_THROW (scanner::id ("", "", "path", "if:"),
                     std::invalid_argument);
  BOOST_CHECK_THROW (scanner::id ("", "", "path", "if", "d3r"),
                     std::invalid_argument);
  BOOST_CHECK_THROW (scanner::id ("", "", "path", "if", "d:r"),
                     std::invalid_argument);
}

BOOST_AUTO_TEST_CASE (fragment_splitting)
{
  scanner::id id ("", "", "virtual:mock:office");

  BOOST_CHECK_EQUAL ("virtual", id.iftype ());
  BOOST_CHECK_EQUAL ("mock", id.driver ());
  BOOST_CHECK_EQUAL ("office", id.path ());
}

BOOST_AUTO_TEST_CASE (driver_splicing)
{
  scanner::id id ("", "", "if::");

  std::string path = id.path ();

  id.driver ("drv");
  BOOST_CHECK_EQUAL ("drv", id.driver ());
  id.driver ("mock");
  BOOST_CHECK_EQUAL ("mock", id.driver ());
  BOOST_CHECK_EQUAL (path, id.path());
}

#include "utsushi/test/runner.ipp"
