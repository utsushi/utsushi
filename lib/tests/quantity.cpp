//  quantity.cpp -- unit tests for the utsushi::quantity API
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

#include <limits>
#include <list>
#include <sstream>
#include <utility>

#include <boost/assign/list_inserter.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test.hpp>

#include "utsushi/quantity.hpp"

using utsushi::quantity;

typedef quantity::integer_type     integer_type;
typedef quantity::non_integer_type non_integer_type;

BOOST_AUTO_TEST_SUITE (SANE_compatibility);

BOOST_AUTO_TEST_CASE (SANE_Int_requirements)
{
  BOOST_REQUIRE (std::numeric_limits< integer_type >::is_integer);
  BOOST_REQUIRE (std::numeric_limits< integer_type >::is_signed);

  if (std::numeric_limits< integer_type >::is_bounded)
    {
      BOOST_CHECK_GE ((-2147483647-1),  // -2^31
                      std::numeric_limits< integer_type >::min ());
      BOOST_CHECK_LE (( 2147483647  ),  //  2^31 - 1
                      std::numeric_limits< integer_type >::max ());
    }
}

BOOST_AUTO_TEST_CASE (SANE_Fixed_requirements)
{
  BOOST_REQUIRE (!std::numeric_limits< non_integer_type >::is_integer);
  BOOST_REQUIRE ( std::numeric_limits< non_integer_type >::is_signed);

  if (std::numeric_limits< non_integer_type >::is_bounded)
    {
      BOOST_CHECK_GE (-32768,
                      -std::numeric_limits< non_integer_type >::max ());
      BOOST_CHECK_LE ( 32768,
                       std::numeric_limits< non_integer_type >::max ());
    }

  non_integer_type resolution (1);
  resolution /= (1L << 16);

  BOOST_CHECK_LE (std::numeric_limits< non_integer_type >::epsilon (),
                  resolution);
}

BOOST_AUTO_TEST_SUITE_END (/* SANE_compatibility */);

void
test_addition (const std::pair< double, double >& arg)
{
  quantity lhs (arg.first);
  quantity rhs (arg.second);
  quantity result (arg.first + arg.second);

  BOOST_CHECK_EQUAL (result, lhs + rhs);
}

void
test_substraction (const std::pair< double, double >& arg)
{
  quantity lhs (arg.first);
  quantity rhs (arg.second);
  quantity result (arg.first - arg.second);

  BOOST_CHECK_EQUAL (result, lhs - rhs);
}

void
test_multiplication (const std::pair< double, double >& arg)
{
  quantity lhs (arg.first);
  quantity rhs (arg.second);
  quantity result (arg.first * arg.second);

  BOOST_CHECK_EQUAL (result, lhs * rhs);
}

BOOST_AUTO_TEST_CASE (promoting_multiplication)
{
  const quantity zahl (2);
  const quantity real (2.3);
  const quantity expect (4.6);

  BOOST_CHECK_EQUAL (expect, zahl * real);
  BOOST_CHECK_EQUAL (expect, real * zahl);

  quantity qz (zahl);
  BOOST_CHECK_EQUAL (expect, qz *= real);

  quantity qr (real);
  BOOST_CHECK_EQUAL (expect, qr *= zahl);
}

BOOST_AUTO_TEST_CASE (promoting_division)
{
  const quantity zahl (2);
  const quantity real (0.8);
  const quantity expect_zr (2.5);
  const quantity expect_rz (0.4);

  BOOST_CHECK_EQUAL (expect_zr, zahl / real);
  BOOST_CHECK_EQUAL (expect_rz, real / zahl);

  quantity qz (zahl);
  BOOST_CHECK_EQUAL (expect_zr, qz /= real);

  quantity qr (real);
  BOOST_CHECK_EQUAL (expect_rz, qr /= zahl);
}

void
test_division (const std::pair< double, double >& arg)
{
  quantity lhs (arg.first);
  quantity rhs (arg.second);
  quantity result (arg.first / arg.second);

  BOOST_CHECK_EQUAL (result, lhs / rhs);
}

BOOST_AUTO_TEST_CASE (unary_nil_negation)
{
  quantity q_nil;

  BOOST_CHECK_EQUAL (+q_nil, -q_nil);
}

BOOST_AUTO_TEST_CASE (simple_unary_negation)
{
  quantity q_pos ( 5.3);
  quantity q_neg (-5.3);

  BOOST_CHECK_EQUAL (-q_pos,  q_neg);
  BOOST_CHECK_EQUAL ( q_pos, -q_neg);
}

BOOST_AUTO_TEST_CASE (double_unary_negation)
{
  quantity q_pos ( 2.5);
  quantity q_neg (-2.5);

  BOOST_CHECK_EQUAL (-(-q_pos), q_pos);
  BOOST_CHECK_EQUAL (-(-q_neg), q_neg);
}

BOOST_AUTO_TEST_CASE (integral_query)
{
  quantity q (0);

  BOOST_CHECK (q.is_integral ());
}

BOOST_AUTO_TEST_CASE (non_integral_query)
{
  quantity q (0.);

  BOOST_CHECK (!q.is_integral ());
}

void
test_ostream_operator (const std::pair< quantity, std::string >& args)
{
  quantity    q  (args.first);
  std::string s  (args.second);
  std::string sq ("not a quantity");

  std::ostringstream os;
  os << q;
  sq = os.str ();

  if (q.is_integral ())
    BOOST_CHECK_EQUAL (std::string::npos, sq.find ('.'));
  else
    BOOST_CHECK_NE (std::string::npos, sq.find ('.'));

  BOOST_CHECK_EQUAL (s, sq);
}

void
test_istream_operator (const std::pair< std::string, quantity >& args)
{
  std::string s (args.first);
  quantity    q (args.second);
  quantity    qs (-123456);

  if (qs == q) qs += 3;         // just make sure they're different

  std::stringstream ss;
  ss << s;
  ss >> qs;

  if (std::string::npos == s.find ('.'))
    BOOST_CHECK ( qs.is_integral ());
  else
    BOOST_CHECK (!qs.is_integral ());

  BOOST_CHECK_EQUAL (q, qs);
}

BOOST_AUTO_TEST_CASE (no_quantity_on_istream)
{
  std::stringstream ss;
  quantity q (1.5);

  ss << "this ain't no quantity";

  BOOST_CHECK_THROW (ss >> q, std::exception);
  BOOST_CHECK_EQUAL (q, 1.5);
}

bool
init_test_runner ()
{
  namespace but = ::boost::unit_test;

  std::list< std::pair< double, double > > args;
  //  any pair of non-trivial amounts will do
  args.push_back (std::pair< double, double > ( 5.20,  3.33));
  args.push_back (std::pair< double, double > ( 5.20, -3.33));
  args.push_back (std::pair< double, double > (-5.20,  3.33));
  args.push_back (std::pair< double, double > (-5.20, -3.33));

  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (test_addition,
                                 args.begin (), args.end ()));
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (test_substraction,
                                 args.begin (), args.end ()));
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (test_multiplication,
                                 args.begin (), args.end ()));
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (test_division,
                                 args.begin (), args.end ()));

  std::list< std::pair< quantity, std::string > > o_args;
  boost::assign::push_back (o_args)
    ( 5,  "5")
    (-5, "-5")
    ( 0,  "0")
    ( 5. ,  "5.0")
    (-5. , "-5.0")
    ( 5.5,  "5.5")
    (-5.5, "-5.5")
    (  .5,  "0.5")
    ( -.5, "-0.5")
    ( 0. ,  "0.0")
    ;
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (test_ostream_operator,
                                 o_args.begin (), o_args.end ()));

  std::list< std::pair< std::string, quantity > > i_args;
  boost::assign::push_back (i_args)
    ( "5",  5)
    (" 5",  5)
    ("+5", +5)
    ("-5", -5)
    ( "0",  0)
    ("+0", +0)
    ("-0", -0)
    ( "5." ,  5. )
    ("+5." , +5. )
    ("-5." , -5. )
    ( "5.5",  5.5)
    ("+5.5", +5.5)
    ("-5.5", -5.5)
    (  ".5",   .5)
    ( "+.5",  +.5)
    ( "-.5",  -.5)
    ( "0." ,  0. )
    ("+0." , +0. )
    ("-0." , -0. )
    ( "0.0",  0.0)
    ("+0.0", +0.0)
    ("-0.0", -0.0)
    (  ".0",   .0)
    ( "+.0",  +.0)
    ( "-.0",  -.0)
    ;
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (test_istream_operator,
                                 i_args.begin (), i_args.end ()));

  return true;
}

#include "utsushi/test/runner.ipp"
