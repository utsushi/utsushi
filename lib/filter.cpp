//  filter.cpp -- interface declarations
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

#include "utsushi/filter.hpp"

namespace utsushi {

void
filter::mark (traits::int_type c, const context& ctx)
{
  output::mark (c, ctx);
  if (traits::is_marker (c)) {
    output_->mark (c, ctx_);
  }
}

void
filter::open (output::ptr output)
{
  output_ = output;
}

void
filter::buffer_size (streamsize size)
{
  buffer_size_ = size;
}

decorator<filter>::decorator (ptr instance)
  : instance_(instance)
{}

streamsize
decorator<filter>::write (const octet *data, streamsize n)
{
  return instance_->write (data, n);
}

void
decorator<filter>::mark(traits::int_type c, const context& ctx)
{
  instance_->mark (c, ctx);
}

void
decorator<filter>::open (output::ptr output)
{
  instance_->open (output);
}

streamsize
decorator<filter>::buffer_size () const
{
  return instance_->buffer_size ();
}

void
decorator<filter>::buffer_size (streamsize size)
{
  instance_->buffer_size (size);
}

context
decorator<filter>::get_context () const
{
  return instance_->get_context ();
}

}       // namespace utsushi
