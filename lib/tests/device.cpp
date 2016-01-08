//  device.cpp -- unit tests for the utsushi::device API
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

#include <exception>

#include <boost/mpl/list.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/test/unit_test.hpp>

#include "utsushi/device.hpp"
#include "utsushi/functional.hpp"
#include "utsushi/test/memory.hpp"
#include "utsushi/test/null.hpp"

using namespace utsushi;

//!  Provide access to the device template's constructor for testing
template <typename io_device>
class test_device : public io_device
{
public:
  test_device () : io_device () {}
  streamsize write (const octet *data, streamsize n) { return n; };
};

typedef boost::mpl::list<idevice,odevice> io_interfaces;

BOOST_AUTO_TEST_CASE_TEMPLATE (default_buffer_capacity, IO, io_interfaces)
{
  test_device<IO> dev;

  BOOST_CHECK_EQUAL (default_buffer_size, dev.buffer_size ());
}

BOOST_AUTO_TEST_CASE_TEMPLATE (buffer_size_changes, IO, io_interfaces)
{
  streamsize buffer_size  = default_buffer_size / 3;

  BOOST_REQUIRE (buffer_size != default_buffer_size);
  BOOST_REQUIRE (buffer_size > 2);

  test_device<IO> dev;

  do {
    dev.buffer_size (buffer_size);
    BOOST_CHECK_EQUAL (buffer_size, dev.buffer_size ());
    buffer_size /= 2;
  }
  while (buffer_size > 2);
}

struct null_fixture
{
  null_idevice idev;
  null_odevice odev;
};

BOOST_FIXTURE_TEST_SUITE (null, null_fixture)

BOOST_AUTO_TEST_CASE (input_operator)
{
  streamsize rv = idev.marker ();
  BOOST_CHECK_EQUAL (traits::eof (), rv);
  rv = idev >> odev;
  BOOST_CHECK_EQUAL (traits::eof (), rv);
}

BOOST_AUTO_TEST_CASE (pipe_operator)
{
  streamsize rv = idev | odev;
  BOOST_CHECK_EQUAL (traits::eof (), rv);
}

BOOST_AUTO_TEST_SUITE_END ();

struct signal_counter
{
  int bos_, boi_, eoi_, eos_;

  signal_counter ()
    : bos_(0), boi_(0), eoi_(0), eos_(0)
  {}

  void operator() (traits::int_type c)
  {
    if (traits::is_marker (c)) {
      if (traits::bos () == c) ++bos_;
      if (traits::boi () == c) ++boi_;
      if (traits::eoi () == c) ++eoi_;
      if (traits::eos () == c) ++eos_;
    }
  }
};

struct raw_fixture
{
  const streamsize octet_count;
  const unsigned   image_count;

  rawmem_idevice idev;
  null_odevice   odev;
  signal_counter counter;

  raw_fixture ()
    : octet_count (40 * 8192), image_count (3),
      idev (octet_count, image_count), counter () {}
};

BOOST_FIXTURE_TEST_SUITE (raw, raw_fixture);

BOOST_AUTO_TEST_CASE (input_operator)
{
  streamsize rv = idev.marker ();
  BOOST_CHECK_EQUAL (traits::bos (), rv);
  rv = idev >> odev;
  BOOST_CHECK_EQUAL (traits::eoi (), rv);
}

BOOST_AUTO_TEST_CASE (pipe_operator)
{
  streamsize rv = idev | odev;
  BOOST_CHECK_EQUAL (traits::eos (), rv);
}

BOOST_AUTO_TEST_CASE (counting_images)
{
  unsigned count = 0;
  streamsize rv  = idev.marker ();

  while (traits::eos () != rv) {
    rv = idev >> odev;
    if (traits::eoi () == rv) ++count;
  }
  BOOST_CHECK_EQUAL (count, image_count);
}

BOOST_AUTO_TEST_CASE (counting_signals)
{
  idev.connect_marker (ref (counter));
  odev.connect_marker (ref (counter));

  streamsize rv = idev | odev;
  BOOST_CHECK_EQUAL (traits::eos (), rv);

  BOOST_CHECK_EQUAL (counter.bos_, 2);
  BOOST_CHECK_EQUAL (counter.boi_, 2 * image_count);
  BOOST_CHECK_EQUAL (counter.eoi_, 2 * image_count);
  BOOST_CHECK_EQUAL (counter.eos_, 2);
}

BOOST_AUTO_TEST_SUITE_END ();

#include "utsushi/test/runner.ipp"
