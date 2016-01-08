//  grammar-mechanics.cpp -- unit tests for ESC/I grammar-mechanics API
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

#include <iterator>

#include <boost/test/unit_test.hpp>

#include <utsushi/test/tools.hpp>

#include "../grammar-mechanics.hpp"
#include "../buffer.hpp"

namespace esci = utsushi::_drv_::esci;

using esci::byte_buffer;
using esci::hardware_request;

namespace {

namespace encoding {

using namespace esci::encoding;
using namespace esci::code_token::mechanic;

class mechanics
  : public basic_grammar_mechanics< default_iterator_type >
{
  typedef basic_grammar_mechanics< default_iterator_type > base_;
public:
  using base_::trace;
};

BOOST_AUTO_TEST_CASE (adf_load)
{
  mechanics generator;
  byte_buffer payload;
  hardware_request mech;

  mech.adf = adf::LOAD;

  bool r = generator.hardware_control_(std::back_inserter (payload), mech);

  BOOST_REQUIRE_MESSAGE (r, generator.trace ());
  BOOST_CHECK_EQUAL (payload, byte_buffer ("#ADFLOAD"));
}

BOOST_AUTO_TEST_CASE (auto_focus)
{
  mechanics generator;
  byte_buffer payload;
  hardware_request mech;

  mech.fcs = hardware_request::focus ();

  bool r = generator.hardware_control_(std::back_inserter (payload), mech);

  BOOST_REQUIRE_MESSAGE (r, generator.trace ());
  BOOST_CHECK_EQUAL (payload, byte_buffer ("#FCSAUTO"));
};

BOOST_AUTO_TEST_CASE (manual_focus)
{
  mechanics generator;
  byte_buffer payload;
  hardware_request mech;

  mech.fcs = hardware_request::focus (64);

  bool r = generator.hardware_control_(std::back_inserter (payload), mech);

  BOOST_REQUIRE_MESSAGE (r, generator.trace ());
  BOOST_CHECK_EQUAL (payload, byte_buffer ("#FCSMANUd064"));
};

BOOST_AUTO_TEST_CASE (reinitialize)
{
  mechanics generator;
  byte_buffer payload;
  hardware_request mech;

  mech.ini = true;

  bool r = generator.hardware_control_(std::back_inserter (payload), mech);

  BOOST_REQUIRE_MESSAGE (r, generator.trace ());
  BOOST_CHECK_EQUAL (payload, byte_buffer ("#INI"));
}

BOOST_AUTO_TEST_CASE (auto_all)
{
  mechanics generator;
  byte_buffer payload;
  hardware_request mech;

  mech.adf = adf::CLEN;
  mech.fcs = hardware_request::focus ();
  mech.ini = true;

  bool r = generator.hardware_control_(std::back_inserter (payload), mech);

  BOOST_REQUIRE_MESSAGE (r, generator.trace ());
  BOOST_CHECK_EQUAL (payload, byte_buffer ("#ADFCLEN#FCSAUTO#INI"));
}

BOOST_AUTO_TEST_CASE (manual_all)
{
  mechanics generator;
  byte_buffer payload;
  hardware_request mech;

  mech.adf = adf::CLEN;
  mech.fcs = hardware_request::focus (-64);
  mech.ini = true;

  bool r = generator.hardware_control_(std::back_inserter (payload), mech);

  BOOST_REQUIRE_MESSAGE (r, generator.trace ());
  BOOST_CHECK_EQUAL (payload, byte_buffer ("#ADFCLEN#FCSMANUi-000064#INI"));
}
}       // namespace encoding

}       // namespace

#include "utsushi/test/runner.ipp"
