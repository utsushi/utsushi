//  buffer.cpp -- image data for speedy I/O transfers
//  Copyright (C) 2012, 2013  SEIKO EPSON CORPORATION
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

#include "utsushi/buffer.hpp"

namespace utsushi {

ibuffer::ibuffer (streamsize buffer_size)
  : base (buffer_size)
  , seq_(traits::not_marker (0))
  , check_marker_(false)
{
  buffer_size_ = buffer_size;
  setg (buffer_, buffer_ + buffer_size_, buffer_ + buffer_size_);
}

streamsize
ibuffer::read (octet *data, streamsize n)
{
  streamsize rv = sgetn (data, n);
  return ((0 < rv)
          ? rv
          : sequence_marker ());
}

streamsize
ibuffer::marker ()
{
  check_marker_ = true;
  streamsize rv = ((traits::eof () == sgetc ())
                   ? sequence_marker () : traits::not_marker (0));
  check_marker_ = false;

  return rv;
}

ibuffer::base::int_type
ibuffer::underflow ()
{
  if (traits::is_marker (seq_)) return traits::eof ();

  streamsize rv = (check_marker_
                   ? io_->marker ()
                   : io_->read (buffer_, buffer_size_));

  if (traits::is_marker (rv)) {
    seq_ = rv;
    ctx_ = io_->get_context ();
    return traits::eof ();
  }

  setg (buffer_, buffer_, buffer_ + rv);

  if (egptr () == gptr ()) {
    seq_ = rv;
    return traits::eof ();
  }

  return traits::to_int_type (*gptr ());
}

traits::int_type
ibuffer::sequence_marker ()
{
  traits::int_type rv = seq_;
  seq_ = traits::not_marker (seq_);
  return rv;
}

obuffer::obuffer (streamsize buffer_size)
  : base (buffer_size)
{
  setp (buffer_, buffer_ + buffer_size_ - 1);
}

streamsize
obuffer::write (const octet *data, streamsize n)
{
  return sputn (data, n);
}

void
obuffer::mark (traits::int_type c, const context& ctx)
{
  if (traits::is_marker (c)) {
    if (traits::eoi() == c || traits::eos() == c) {
      sync ();
    }
    io_->mark (c, ctx);
  }
}

obuffer::base::int_type
obuffer:: overflow (base::int_type c)
{
  if (!traits::is_marker (c)) {
    *pptr () = c;
    pbump (1);
  }

  streamsize rv = io_->write (buffer_, pptr () - buffer_);
  traits::move (buffer_, buffer_ + rv, pptr () - buffer_ - rv);
  setp (pptr () - rv, buffer_ + buffer_size_ - 1);

  return traits::not_eof (c);
}

int
obuffer::sync ()
{
  streamsize n = pptr () - buffer_; // remaining number of octets

  if (!n) return 0;

  streamsize rv;

  do
    {
      rv = io_->write (pptr () - n, n);
      n -= rv;
    }
  while (0 < rv && 0 < n);

  traits::move (buffer_, pptr () - n, n);
  setp (buffer_ + n, buffer_ + buffer_size_ - 1);

  return (0 == n ? 0 : -1);
}

}       // namespace utsushi
