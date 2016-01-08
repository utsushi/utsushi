//  null.hpp -- device and filter implementations
//  Copyright (C) 2012, 2014, 2015  SEIKO EPSON CORPORATION
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

#ifndef utsushi_test_null_hpp_
#define utsushi_test_null_hpp_

#include "../device.hpp"
#include "../filter.hpp"

namespace utsushi {

//!  Devices that do not produce any images whatsoever
class null_idevice : public idevice
{
public:
  streamsize read (octet *data, streamsize n)
  { return traits::eof (); }
};

//!  Devices that discard any and all images
class null_odevice : public odevice
{
public:
  streamsize write (const octet *data, streamsize n) { return n; }
};

//!  Filters that discard all image data
class null_filter : public filter
{
public:
  streamsize write (const octet *data, streamsize n)
  { return n; }
};

}       // namespace utsushi

#endif  /* utsushi_test_null_hpp_ */
