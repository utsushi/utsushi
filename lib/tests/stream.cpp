//  stream.cpp -- unit tests for the utsushi::stream API
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

#include "utsushi/device.hpp"
#include "utsushi/filter.hpp"
#include "utsushi/stream.hpp"
#include "utsushi/test/memory.hpp"
#include "utsushi/test/null.hpp"

using namespace utsushi;

struct null_fixture
{
  istream istr;
  ostream ostr;

  null_fixture () {
    istr.push (idevice::ptr (new null_idevice ()));
    ostr.push (odevice::ptr (new null_odevice ()));
  }
};

BOOST_FIXTURE_TEST_SUITE (null, null_fixture);

BOOST_AUTO_TEST_CASE (input_operator)
{
  streamsize rv = istr.marker ();
  BOOST_CHECK_EQUAL (traits::eof (), rv);
  rv = istr >> ostr;
  BOOST_CHECK_EQUAL (traits::eof (), rv);
}

BOOST_AUTO_TEST_CASE (pipe_operator)
{
  streamsize rv = istr | ostr;
  BOOST_CHECK_EQUAL (traits::eof (), rv);
}

BOOST_AUTO_TEST_SUITE_END ();

struct raw_fixture
{
  const streamsize octet_count;
  const unsigned   image_count;

  istream istr;
  ostream ostr;

  raw_fixture ()
    : octet_count (40 * 8192), image_count (3)
  {
    istr.push (idevice::ptr (new rawmem_idevice (octet_count, image_count)));
    ostr.push (odevice::ptr (new null_odevice ()));
  }
};

BOOST_FIXTURE_TEST_SUITE (raw, raw_fixture);

BOOST_AUTO_TEST_CASE (input_operator)
{
  streamsize rv = istr.marker ();
  BOOST_CHECK_EQUAL (traits::bos (), rv);
  rv = istr >> ostr;
  BOOST_CHECK_EQUAL (traits::eoi (), rv);
}

BOOST_AUTO_TEST_CASE (pipe_operator)
{
  streamsize rv = istr | ostr;
  BOOST_CHECK_EQUAL (traits::eos (), rv);
}

BOOST_AUTO_TEST_CASE (counting_images)
{
  unsigned count = 0;
  streamsize rv  = istr.marker ();

  while (traits::eos () != rv) {
    rv = istr >> ostr;
    if (traits::eoi () == rv) ++count;
  }
  BOOST_CHECK_EQUAL (count, image_count);
}

BOOST_AUTO_TEST_SUITE_END ();

struct filter_fixture
{
  const streamsize octet_count;
  const unsigned   image_count;

  istream istr;
  ostream ostr;

  filter_fixture ()
    : octet_count (30 * 8192), image_count (2)
  {
    istr.push (ifilter::ptr (new thru_ifilter));
    istr.push (idevice::ptr (new rawmem_idevice (octet_count, image_count)));
    ostr.push (ofilter::ptr (new thru_ofilter));
    ostr.push (odevice::ptr (new null_odevice));
  }
};

BOOST_FIXTURE_TEST_SUITE (filter, filter_fixture);

BOOST_AUTO_TEST_CASE (counting_images)
{
  unsigned count = 0;
  streamsize rv  = istr.marker ();

  while (traits::eos () != rv) {
    rv = istr >> ostr;
    if (traits::eoi () == rv) ++count;
  }
  BOOST_CHECK_EQUAL (count, image_count);
}

BOOST_AUTO_TEST_SUITE_END ();

#include "utsushi/test/runner.ipp"
