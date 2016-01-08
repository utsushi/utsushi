//  buffer.cpp -- unit tests for the utsushi::buffer API
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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

#include <cstring>

#include <string>

#include <boost/test/unit_test.hpp>

#include "utsushi/buffer.hpp"
#include "utsushi/file.hpp"
#include "utsushi/test/memory.hpp"

#include <boost/filesystem.hpp>

using namespace utsushi;

using boost::filesystem::file_size;
using boost::filesystem::exists;
using boost::filesystem::remove;
using boost::filesystem::path;

struct fixture
{
  const streamsize octets;
  const streamsize size;
  octet *data;

  const std::string name;

  fixture () : octets ((5 * default_buffer_size) / 2),
               size   (     default_buffer_size  / 3),
               data   (new octet[size]),
               name   ("buffer.out")
  {}
  ~fixture ()
  {
    delete [] data;
    exists (name) && remove (name);
  }

  void run_filter_test (filter::ptr flt)
  {
    streamsize count = 0;
    context    ctx;

    flt->mark (traits::bos (), ctx);
    flt->mark (traits::boi (), ctx);
    while (count != octets) {
      count += flt->write (data, std::min (size, octets - count));
    }
    flt->mark (traits::eoi (), ctx);
    flt->mark (traits::eos (), ctx);

    BOOST_CHECK_EQUAL (octets, file_size (name));
  }
};

BOOST_FIXTURE_TEST_CASE (buffered_device_write, fixture)
{
  odevice::ptr dev = make_shared< file_odevice > (name);
  buffer buf;
  context ctx;

  streamsize count = 0;

  buf.open (dev);
  buf.mark (traits::bos (), ctx);
  buf.mark (traits::boi (), ctx);
  while (count != octets) {
    count += buf.write (data, std::min (size, octets - count));
  }
  buf.mark (traits::eoi (), ctx);
  buf.mark (traits::eos (), ctx);

  BOOST_CHECK_EQUAL (octets, file_size (name));
  remove (name);
}

BOOST_FIXTURE_TEST_CASE (filtered_device_write, fixture)
{
  odevice::ptr dev = make_shared< file_odevice > (name);
  buffer::ptr  buf = make_shared< buffer > ();
  filter::ptr  flt = make_shared< thru_filter > ();

  buf->open (dev);
  flt->open (buf);

  run_filter_test (flt);
}

BOOST_FIXTURE_TEST_CASE (doubly_filtered_device_write, fixture)
{
  odevice::ptr dev  = make_shared< file_odevice > (name);
  buffer::ptr  buf0 = make_shared< buffer > ();
  filter::ptr  flt0 = make_shared< thru_filter > ();
  buffer::ptr  buf  = make_shared< buffer > ();
  filter::ptr  flt  = make_shared< thru_filter > ();

  buf0->open (dev);
  flt0->open (buf0);
  buf->open (flt0);
  flt->open (buf);

  run_filter_test (flt);
}

struct fixture_one_odevice
{
  class one_odevice : public odevice
  {
    octet *p_;

  public:
    one_odevice (octet *data)
     : p_ (data)
    {}

    streamsize write (const octet *data, streamsize n)
    {
      if (1 > n) return 0;
      *(p_++) = data[0];
      return 1;
    }
  };
};

//! \todo parameterize this test
BOOST_FIXTURE_TEST_CASE (unprocessed_octets_preserved, fixture_one_odevice)
{
  const streamsize dat_size = 8; // total size of the test data
  const streamsize chu_size = 4; // maximum number of octets to write at once
  const streamsize buf_size = 3; // buffer size

  octet *in_data = new octet[dat_size];
  octet *out_data = new octet[dat_size];

  for (streamsize i=0; i<dat_size; ++i) traits::assign (in_data[i], i);
  traits::assign (out_data, dat_size, dat_size);

  odevice::ptr dev = make_shared< one_odevice > (out_data);
  buffer::ptr  buf = make_shared< buffer > (buf_size);

  buf->open (dev);

  streamsize count = 0;
  while (count < dat_size)
  {
    streamsize s = dat_size - count;
    if (s > chu_size) s = chu_size;
    count += buf->write (in_data + count, s);
  }

  buf->mark (traits::eoi (), context ());

  BOOST_CHECK_EQUAL_COLLECTIONS (in_data , in_data  + dat_size,
                                 out_data, out_data + dat_size);

  delete [] in_data;
  delete [] out_data;
}

#include "utsushi/test/runner.ipp"
