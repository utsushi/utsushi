//  buffer.hpp -- image data for speedy I/O transfers
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

#ifndef utsushi_buffer_hpp_
#define utsushi_buffer_hpp_

#include <streambuf>

#include "iobase.hpp"

namespace utsushi {

//!  Collect octets in temporary storage to improve performance
class buffer
  : protected std::basic_streambuf<octet, traits>
  , public output
{
public:
  typedef shared_ptr<buffer> ptr;

  buffer (streamsize buffer_size = default_buffer_size);
  ~buffer ();

  streamsize write (const octet *data, streamsize n);
  void mark (traits::int_type c, const context& ctx);

  //!  Sets a buffer's underlying output object
  void open (output::ptr output);

protected:
  typedef std::basic_streambuf<octet, traits> base;
  typedef base::int_type int_type;

  //!  Write data to the underlying %device
  /*!  Called whenever a write() would completely fill up the %buffer,
   *   this function tries to empty the %buffer by writing its content
   *   to the object's %device.
   */
  int_type overflow (int_type c);

  //!  Write remaining data to the underlying %device
  /*!  Called when encountering an end-of type mark() in the output,
   *   this member function tries to completely empty the %buffer by
   *   writing its content to the object's %device.
   */
  int sync ();

private:

  output::ptr output_;

  octet *buffer_;

  streamsize max_size_;
  streamsize min_size_;
};

}       // namespace utsushi

#endif  /* utsushi_buffer_hpp_ */
