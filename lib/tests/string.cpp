//  string.cpp -- unit tests for the utsushi::string API
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

#include <boost/test/unit_test.hpp>

#include "utsushi/string.hpp"

using namespace utsushi;

BOOST_AUTO_TEST_CASE (equality_comparison_literal)
{
  string s ("literal");

  //BOOST_CHECK ("literal" == s);
  BOOST_CHECK (s == "literal");

  BOOST_CHECK_EQUAL ("literal", s);
  BOOST_CHECK_EQUAL (s, "literal");

}

BOOST_AUTO_TEST_CASE (equality_comparison_char_ptr)
{
  const char *c_ptr ("char *");
  string s (c_ptr);

  //BOOST_CHECK (c_ptr == s);
  BOOST_CHECK (s == c_ptr);

  //BOOST_CHECK_EQUAL (c_ptr, s);
  BOOST_CHECK_EQUAL (s, c_ptr);
}

BOOST_AUTO_TEST_CASE (equality_comparison_std_string)
{
  std::string std_s ("std::tring");
  string s (std_s);

  //BOOST_CHECK (std_s == s);
  BOOST_CHECK (s == std_s);

  //BOOST_CHECK_EQUAL (std_s, s);
  BOOST_CHECK_EQUAL (s, std_s);
}

#include "utsushi/test/runner.ipp"
