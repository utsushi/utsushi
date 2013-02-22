//  stream.cpp -- interface declarations
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

#include "utsushi/stream.hpp"

namespace utsushi {

streamsize
istream::read (octet *data, streamsize n)
{
  return io_bottom_->read (data, n);
}

streamsize
istream::marker ()
{
  return io_bottom_->marker ();
}

context
istream::get_context () const
{
  return io_bottom_->get_context ();
}

void
istream::push (idevice::ptr device)
{
  push (device, device);
  device_ = device;
}

void
istream::push (ifilter::ptr filter)
{
  push (filter, filter);
  filter_ = filter;
}

streamsize
ostream::write (const octet *data, streamsize n)
{
  return io_bottom_->write (data, n);
}

void
ostream::mark (traits::int_type c, const context& ctx)
{
  io_bottom_->mark (c, ctx);
}

void
ostream::push (odevice::ptr device)
{
  push (device, device);
  device_ = device;
}

void
ostream::push (ofilter::ptr filter)
{
  push (filter, filter);
  filter_ = filter;
}

}       // namespace utsushi
