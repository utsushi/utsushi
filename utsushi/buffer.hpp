//  buffer.hpp -- image data for speedy I/O transfers
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

#ifndef utsushi_buffer_hpp_
#define utsushi_buffer_hpp_

#include <streambuf>

#include "iobase.hpp"

namespace utsushi {

//!  Collect octets in temporary storage to improve performance
template <typename IO>
class buffer : protected std::basic_streambuf<octet, traits>
{
  typedef std::basic_streambuf<octet, traits> base;

protected:
  typedef shared_ptr<IO> io_ptr;
  typedef base::int_type int_type;

  io_ptr io_;

  octet *buffer_;

  buffer (streamsize buffer_size = default_buffer_size)
    : buffer_(new octet[buffer_size])
  {}

  ~buffer () { delete [] buffer_; }

public:
  typedef shared_ptr<buffer> ptr;

  //!  Sets a buffer's underlying I/O object
  void open (io_ptr io) { io_ = io; }
};


//!  Collect incoming octets in temporary storage to improve performance
class ibuffer : public buffer<input>, public input
{
  typedef buffer<input>  base;

  traits::int_type seq_;        //!< cached sequence marker value

public:
  typedef shared_ptr<ibuffer> ptr;

  ibuffer (streamsize buffer_size = default_buffer_size);

  streamsize read (octet *data, streamsize n);
  streamsize marker ();

protected:
  //!  Read data from the underlying %input %device
  /*!  Called whenever a read() results in an empty %buffer, this
   *   function tries to fill the %buffer by reading more data from
   *   the object's %device.
   */
  base::int_type underflow ();

  //!  Returns and resets the cached sequence marker
  traits::int_type sequence_marker ();
};

//!  Collect outgoing octets in temporary storage to improve performance
class obuffer : public buffer<output>, public output
{
  typedef buffer<output> base;

public:
  typedef shared_ptr<obuffer> ptr;

  obuffer (streamsize buffer_size = default_buffer_size);

  streamsize write (const octet *data, streamsize n);
  void mark (traits::int_type c, const context& ctx);

protected:
  //!  Write data to the underlying %device
  /*!  Called whenever a write() would completely fill up the %buffer,
   *   this function tries to empty the %buffer by writing its content
   *   to the object's %device.
   */
  base::int_type overflow (base::int_type c);

  //!  Write remaining data to the underlying %device
  /*!  Called when encountering an end-of type mark() in the output,
   *   this member function tries to completely empty the %buffer by
   *   writing its content to the object's %device.
   */
  int sync ();
};

}       // namespace utsushi

#endif  /* utsushi_buffer_hpp_ */
