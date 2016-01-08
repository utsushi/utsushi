//  null.cpp -- unit tests for the null device implementations
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

#include <boost/test/parameterized_test.hpp>
#include <boost/test/unit_test.hpp>

#include "utsushi/test/null.hpp"

using namespace utsushi;

/*!  The device_impl_base classes are heavily templated and subclass
 *   API issues are only noticed when you actually try to use one of
 *   its instances.  This test basically makes sure that null device
 *   objects can be created.
 */
BOOST_AUTO_TEST_CASE (instantiation)
{
  null_idevice idev;
  null_odevice odev;
}

/*!  Verify that null_idevice::read() keeps returning traits::eof(),
 *   even when called repeatedly.
 */
static
void
read_repeatedly (unsigned count)
{
  const streamsize default_buffer_size = 8192;

  octet buffer[default_buffer_size];
  null_idevice dev;

  for (unsigned j = 0; j < count; ++j) {
    dev.read (buffer, default_buffer_size);
  }
  streamsize rv = dev.read (buffer, default_buffer_size);
  BOOST_CHECK_EQUAL (traits::eof (), rv);
}

/*!  Verify that null_odevice::write() always consumes all the octets
     we requested to be written.
     \note  We blindly ignore memory out-of-bounds issues in order to
            test write requests for absurdly large octet counts.
 */
static
void
write_sizes (streamsize size)
{
  const streamsize default_buffer_size = 8192;

  octet buffer[default_buffer_size];
  null_odevice dev;

  streamsize rv = dev.write (buffer, size);
  BOOST_CHECK_EQUAL (size, rv);
}

#define ARRAY_END(a)  (a) + sizeof (a) / sizeof (a[0])

bool
init_test_runner ()
{
  namespace but = boost::unit_test;

  unsigned count[] = { 0, 1, 3, 7, 15, 31 };
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (read_repeatedly, count, ARRAY_END (count)));

  streamsize size[] = {
    0, 1, 2, 16, 64, 256, 512,
    8 << 10,                    // 8 KB
    1 << 20,                    // 1 MB
    1 << 30,                    // 1 GB
    std::numeric_limits<streamsize>::max()
  };
  but::framework::master_test_suite ()
    .add (BOOST_PARAM_TEST_CASE (write_sizes, size, ARRAY_END (size)));

  return true;
}

#include "utsushi/test/runner.ipp"
