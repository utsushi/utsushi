//  iobase.cpp -- common aspects of image data I/O
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

#include "utsushi/iobase.hpp"

namespace utsushi {

input::input (const context& ctx)
  : buffer_size_(default_buffer_size), ctx_(ctx)
{}

input::~input ()
{}

streamsize
input::buffer_size () const
{
  return buffer_size_;
}

context
input::get_context () const
{
  return ctx_;
}

output::output ()
  : buffer_size_(default_buffer_size)
{}

output::~output ()
{}

void
output::mark (traits::int_type c, const context& ctx)
{
  if (traits::is_marker (c)) {
    if (traits::bos () == c) bos (ctx);
    if (traits::boi () == c) boi (ctx);
    if (traits::eoi () == c) eoi (ctx);
    if (traits::eos () == c) eos (ctx);
    if (traits::eof () == c) eof (ctx);
  }
}
streamsize
output::buffer_size () const
{
  return buffer_size_;
}

context
output::get_context () const
{
  return ctx_;
}

void
output::bos (const context& ctx)
{}

void
output::boi (const context& ctx)
{}

void
output::eoi (const context& ctx)
{}

void
output::eos (const context& ctx)
{}

void
output::eof (const context& ctx)
{}

streamsize
operator| (input& iref, output& oref)
{
  streamsize rv = iref.marker ();
  if (traits::bos () != rv) return rv;

  oref.mark (traits::bos (), iref.get_context ());
  while (   traits::eos () != rv
         && traits::eof () != rv)
    {
      rv = iref >> oref;
    }
  oref.mark (rv, iref.get_context ());
  return rv;
}

streamsize
operator>> (input& iref, output& oref)
{
  streamsize n = iref.marker ();
  if (traits::boi () != n) return n;

  streamsize buffer_size = std::max (iref.buffer_size (),
                                     oref.buffer_size ());

  octet *data = new octet[buffer_size];

  oref.mark (traits::boi (), iref.get_context ());
  n = iref.read (data, buffer_size);
  while (   traits::eoi () != n
         && traits::eof () != n)
    {
      const octet *p = data;
      streamsize m;

      while (0 < n) {
        m = oref.write (p, n);
        p += m;
        n -= m;
      }
      n = iref.read (data, buffer_size);
    }
  oref.mark (n, iref.get_context ());
  delete [] data;
  return n;
}

}       // namespace utsushi
