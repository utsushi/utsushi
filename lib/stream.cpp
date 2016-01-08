//  stream.cpp -- interface declarations
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "utsushi/stream.hpp"

namespace utsushi {

streamsize
stream::write (const octet *data, streamsize n)
{
  return out_bottom_->write (data, n);
}

void
stream::mark (traits::int_type c, const context& ctx)
{
  out_bottom_->mark (c, ctx);
}

void
stream::push (odevice::ptr device)
{
  push (device, device);
  device_ = device;
}

void
stream::push (filter::ptr filter)
{
  push (filter, filter);
  filter_ = filter;
}

streamsize
stream::buffer_size () const
{
  return get_device ()->buffer_size ();
}

odevice::ptr
stream::get_device () const
{
  return static_pointer_cast< odevice > (dev_bottom_);
}

void
stream::attach (output::ptr out, device_ptr device,
                output::ptr buf, buffer::ptr buffer)
{
  if (buffer) {
    buffer ->open (out);
    filter_->open (buf);
  } else {
    out_bottom_ = out;
    dev_bottom_ = device;
  }
}

}       // namespace utsushi
