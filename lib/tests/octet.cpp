//  octet.cpp -- unit tests for utsushi octets
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
#include <utility>
#include <vector>

#include <boost/static_assert.hpp>
#include <boost/test/unit_test.hpp>

#include "utsushi/octet.hpp"

using utsushi::octet;
using utsushi::traits;

struct fixture
{
  typedef std::vector< std::pair <traits::int_type, std::string> > seq;

  const octet min;
  const octet max;
  seq seq_marker;

  static bool ne_int_type (traits::int_type i1,
                           traits::int_type i2)
  { return !traits::eq_int_type (i1, i2); }

  static bool ne_marker (traits::int_type i)
  { return !traits::is_marker (i); }

  fixture ()
    : min (std::numeric_limits<octet>::min ()),
      max (std::numeric_limits<octet>::max ())
  {
    seq_marker.push_back (std::make_pair (traits::eof (), "traits::eof ()"));
    seq_marker.push_back (std::make_pair (traits::eos (), "traits::eos ()"));
    seq_marker.push_back (std::make_pair (traits::eoi (), "traits::eoi ()"));
    seq_marker.push_back (std::make_pair (traits::boi (), "traits::boi ()"));
    seq_marker.push_back (std::make_pair (traits::bos (), "traits::bos ()"));
    seq_marker.push_back (std::make_pair (traits::bof (), "traits::bof ()"));
  }
};

//  compile time sanity tests, assuming sign occupies one bit
BOOST_STATIC_ASSERT (std::numeric_limits<octet>::is_specialized);
BOOST_STATIC_ASSERT (   (!std::numeric_limits<octet>::is_signed
                         && (8 == std::numeric_limits<octet>::digits))
                     || ( std::numeric_limits<octet>::is_signed
                         && (7 == std::numeric_limits<octet>::digits)));

BOOST_FIXTURE_TEST_CASE (mutual_sequence_marker_inequality, fixture)
{
  seq::const_iterator it, jt;

  for (it = seq_marker.begin (); it < seq_marker.end (); ++it) {
    for (jt = it + 1; jt < seq_marker.end (); ++jt) {
      BOOST_CHECK_MESSAGE (it->first != jt->first,
                           it->second << " != " << jt->second <<
                           " [ " << it->first << " != " << jt->first << " ]");
    }
  }
}

BOOST_FIXTURE_TEST_CASE (sequence_marker_query, fixture)
{
  seq::const_iterator it;

  for (it = seq_marker.begin (); it < seq_marker.end (); ++it) {
    BOOST_CHECK_MESSAGE (traits::is_marker (it->first),
                         "traits::is_marker (" << it->second << ")");
  }
}

BOOST_FIXTURE_TEST_CASE (not_marker_from_sequence_marker, fixture)
{
  seq::const_iterator it;

  for (it = seq_marker.begin (); it < seq_marker.end (); ++it) {
    BOOST_CHECK_MESSAGE (!traits::is_marker (traits::not_marker (it->first)),
                         "!traits::is_marker (traits::not_marker ("
                         << it->second << ")");
  }
}

BOOST_FIXTURE_TEST_CASE (sequence_marker_not_in_octet_range, fixture)
{
  BOOST_STATIC_ASSERT (std::numeric_limits<octet>::is_bounded);

  seq::const_iterator it;

  for (it = seq_marker.begin (); it < seq_marker.end (); ++it) {
    BOOST_CHECK_MESSAGE ((   (it->first < traits::to_int_type (min))
                          || (it->first > traits::to_int_type (max))),
                         it->second << " not in [octet::min, octet::max]");
  }
}

BOOST_FIXTURE_TEST_CASE (sequence_marker_octet_inequality, fixture)
{
  BOOST_STATIC_ASSERT (std::numeric_limits<octet>::is_bounded);

  seq::const_iterator it;

  for (it = seq_marker.begin (); it < seq_marker.end (); ++it) {
    BOOST_TEST_MESSAGE ("-- testing " << it->second);
    octet val = min;
    do {
      BOOST_CHECK_PREDICATE (ne_int_type,
                             (it->first)(traits::to_int_type (val)));
      ++val;
    } while (val < max);
    BOOST_CHECK_PREDICATE (ne_int_type,
                           (it->first)(traits::to_int_type (val)));
  }
}

BOOST_FIXTURE_TEST_CASE (octet_is_not_a_marker, fixture)
{
  BOOST_STATIC_ASSERT (std::numeric_limits<octet>::is_bounded);

  octet val = min;
  do {
    BOOST_CHECK_PREDICATE (ne_marker, (traits::to_int_type (val)));
    ++val;
  } while (val < max);
  BOOST_CHECK_PREDICATE (ne_marker, (traits::to_int_type (val)));
}

BOOST_FIXTURE_TEST_CASE (not_marker_from_octet_range, fixture)
{
  BOOST_STATIC_ASSERT (std::numeric_limits<octet>::is_bounded);

  traits::int_type val = traits::to_int_type (min);
  do {
    BOOST_CHECK_EQUAL (val, traits::not_marker (val));
    ++val;
  } while (val < traits::to_int_type (max));
  BOOST_CHECK_EQUAL (val, traits::not_marker (val));
}

#include "utsushi/test/runner.ipp"
