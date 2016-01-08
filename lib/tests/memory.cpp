//  memory.cpp -- unit tests for the memory device implementations
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

#include <boost/static_assert.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test.hpp>

#include "utsushi/iobase.hpp"
#include "utsushi/test/memory.hpp"
#include "utsushi/test/null.hpp"

using namespace utsushi;

static
void
image_count (unsigned images)
{
  const streamsize size = 2048;
  octet buffer[size];

  const streamsize octets = size / 2;
  BOOST_STATIC_ASSERT (size >= octets);

  rawmem_idevice dev (octets, images);
  streamsize     rv;

  rv = dev.read (buffer, size);
  BOOST_CHECK_EQUAL (traits::bos (), rv);
  for (unsigned i = 0; i < images; ++i) {
    rv = dev.read (buffer, size);
    BOOST_CHECK_EQUAL (traits::boi (), rv);
    rv = dev.read (buffer, size);
    BOOST_CHECK_EQUAL (octets, rv);
    rv = dev.read (buffer, size);
    BOOST_CHECK_EQUAL (traits::eoi (), rv);
  }
  rv = dev.read (buffer, size);
  BOOST_CHECK_EQUAL (traits::eos (), rv);
}

static
void
octet_count (streamsize octets)
{
  const streamsize size = 8192;
  octet buffer[size];

  const unsigned images = 2;

  BOOST_REQUIRE (size >= octets);

  rawmem_idevice dev (octets, images);
  streamsize     rv;

  rv = dev.read (buffer, size);
  BOOST_CHECK_EQUAL (traits::bos (), rv);
  for (unsigned i = 0; i < images; ++i) {
    rv = dev.read (buffer, size);
    BOOST_CHECK_EQUAL (traits::boi (), rv);
    rv = dev.read (buffer, size);
    BOOST_CHECK_EQUAL (octets, rv);
    rv = dev.read (buffer, size);
    BOOST_CHECK_EQUAL (traits::eoi (), rv);
  }
  rv = dev.read (buffer, size);
  BOOST_CHECK_EQUAL (traits::eos (), rv);
}

/*!  Tests repeated reads on an infinitely large image.
 */
static
void
multi_read (unsigned reads)
{
  const streamsize size = 4096;
  octet buffer[size];

  rawmem_idevice dev;
  streamsize     rv;

  rv = dev.read (buffer, size);
  BOOST_CHECK_EQUAL (traits::bos (), rv);
  rv = dev.read (buffer, size);
  BOOST_CHECK_EQUAL (traits::boi (), rv);
  for (unsigned i = 0; i < reads; ++i) {
    rv = dev.read (buffer, size);
    BOOST_CHECK_EQUAL (size, rv);
  }
}

/*!  Tests single image acquisition for a number of image sizes.
 */
static
void
image_acquisition (streamsize octets)
{
  const streamsize size = 1024;
  octet buffer[size];

  rawmem_idevice dev (octets);
  streamsize     rv;

  rv = dev.read (buffer, size);
  BOOST_CHECK_EQUAL (traits::bos (), rv);
  rv = dev.read (buffer, size);
  BOOST_CHECK_EQUAL (traits::boi (), rv);

  streamsize octets_left = octets;
  rv = 0;
  while (traits::eoi () != rv) {
    octets_left -= rv;
    rv = dev.read (buffer, size);
  }
  BOOST_CHECK_EQUAL (0, octets_left);
}

static
void
constant_octets (octet value)
{
  const streamsize size = 8192;
  octet buffer[size];

  const streamsize margin = 10;
  const octet nul = 0x00;
  traits::assign (buffer, size, nul);
  BOOST_MESSAGE ("value: " << value );

  setmem_idevice dev (shared_ptr<setmem_idevice::generator>
                      (new const_generator (value)));
  dev.marker ();
  dev.marker ();
  dev.read (buffer + margin, size - 2 * margin);

  const octet *p = buffer;
  streamsize   n = 0;
  while (n < margin) {
    BOOST_CHECK_EQUAL (nul, *p);
    ++n, ++p;
  }
  while (n < size - margin) {
    BOOST_CHECK_EQUAL (value, *p);
    ++n, ++p;
  }
  while (n < size) {
    BOOST_CHECK_EQUAL (nul, *p);
    ++n, ++p;
  }
}

struct raw_fixture
{
  const streamsize octet_count;
  const unsigned   image_count;
  const unsigned   sequence_count;

  rawmem_idevice idev;
  null_odevice   odev;

  raw_fixture ()
    : octet_count (40 * 8192), image_count (3), sequence_count (9),
      idev (octet_count, image_count) {}
};

BOOST_FIXTURE_TEST_SUITE (raw, raw_fixture);

/*! Tests that a sequence can be read correctly multiple times
 */
BOOST_AUTO_TEST_CASE (multi_sequence)
{
  for (unsigned int i = 0; i < sequence_count; ++i) {
    unsigned count = 0;
    streamsize rv = 0;

    idev.reset ();

    rv = idev.marker ();
    BOOST_CHECK_EQUAL (traits::bos (), rv);
    while (count < image_count) {
      rv = idev >> odev;
      BOOST_CHECK_EQUAL (traits::eoi (), rv);
      count++;
    }
    rv = idev.marker ();
    BOOST_CHECK_EQUAL (traits::eos (), rv);
    BOOST_CHECK_EQUAL (image_count, count);
  }
}

BOOST_AUTO_TEST_SUITE_END ();

#define ARRAY_END(a)  (a) + sizeof (a) / sizeof (a[0])

bool
init_test_runner ()
{
  namespace but = boost::unit_test;

  unsigned images[] = { 32, 16, 8, 4, 2, 1 };
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (image_count,
                                 images, ARRAY_END (images)));

  streamsize octets[] = { 4096, 2048, 1024, 512, 256, 128, 64,
                            32,   16,    8,   4,   2,   1 };
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (octet_count,
                                 octets, ARRAY_END (octets)));

  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (multi_read,
                                 images, ARRAY_END (images)));

  streamsize img_size[] = { 32 * 1024, 16 * 1024, 8192, 4096, 2048, 1024,
                            512, 256, 128, 64, 32, 16, 8, 4, 2, 1 };
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (image_acquisition,
                                 img_size, ARRAY_END (img_size)));

  octet value[] = {
    octet (0x05),
    octet (0x13),
    octet (0xBA),
    octet (0x9C)
  };
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (constant_octets,
                                 value, ARRAY_END (value)));

  return true;
}

#include "utsushi/test/runner.ipp"
