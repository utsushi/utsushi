//  filter.cpp -- interface declarations
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

#include "utsushi/filter.hpp"

namespace utsushi {

streamsize
ifilter::marker ()
{
  return io_->marker ();
}

void
ifilter::buffer_size (streamsize size)
{
  buffer_size_ = size;
}

void
ofilter::mark (traits::int_type c, const context& ctx)
{
  output::mark (c, ctx);
  if (traits::is_marker (c)) {
    io_->mark (c, ctx_);
  }
}

void
ofilter::buffer_size (streamsize size)
{
  buffer_size_ = size;
}

decorator<ifilter>::decorator (ptr instance)
  : instance_(instance)
{}

streamsize
decorator<ifilter>::read (octet *data, streamsize n)
{
  return instance_->read (data, n);
}

streamsize
decorator<ifilter>::marker ()
{
  return instance_->marker ();
}

void
decorator<ifilter>::open (io_ptr io)
{
  instance_->open (io);
}

streamsize
decorator<ifilter>::buffer_size () const
{
  return instance_->buffer_size ();
}

void
decorator<ifilter>::buffer_size (streamsize size)
{
  instance_->buffer_size (size);
}

context
decorator<ifilter>::get_context () const
{
  return instance_->get_context ();
}

decorator<ofilter>::decorator (ptr instance)
  : instance_(instance)
{}

streamsize
decorator<ofilter>::write (const octet *data, streamsize n)
{
  return instance_->write (data, n);
}

void
decorator<ofilter>::mark(traits::int_type c, const context& ctx)
{
  instance_->mark (c, ctx);
}

void
decorator<ofilter>::open (io_ptr io)
{
  instance_->open (io);
}

streamsize
decorator<ofilter>::buffer_size () const
{
  return instance_->buffer_size ();
}

void
decorator<ofilter>::buffer_size (streamsize size)
{
  instance_->buffer_size (size);
}

context
decorator<ofilter>::get_context () const
{
  return instance_->get_context ();
}

}       // namespace utsushi
