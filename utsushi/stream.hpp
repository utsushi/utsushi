//  stream.hpp -- interface declarations
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

#ifndef utsushi_stream_hpp_
#define utsushi_stream_hpp_

#include "buffer.hpp"
#include "filter.hpp"

namespace utsushi {

//!  Access or store an image data sequence
/*!  A %stream encapsulates a %device and zero or more filters.  The
 *   %device and filters are maintained in a stack(-like) fashion.
 *   The bottom element of this stack, \em i.e. the filter or device
 *   that was pushed \em first, faces the stream's API user.
 *
 *   A %stream may not be used for I/O activities until a %device has
 *   been pushed onto the stack.  Once this has been done, the %stream
 *   is complete and I/O can be performed.
 *
 *   \note  Once complete, devices and filters can no longer be pushed
 *          onto a %stream.
 */
//!  Image data consuming %streams
class stream
  : public output
{
public:
  typedef shared_ptr<stream> ptr;

  streamsize write (const octet *data, streamsize n);
  void mark (traits::int_type c, const context& ctx);

  //!  Pushes a \a %device onto the object's %output stack
  /*!  This completes the object's stack and one can now write() image
   *   data to the \a %device.
   */
  void push (odevice::ptr device);

  //!  Pushes a \a %filter onto the object's %output stack
  /*!  Image data passed to write() will be processed by all filters
   *   on the stack, starting with the \a %filter that was pushed \e
   *   first, before it is consumed by the object's %device.
   */
  void push (filter::ptr filter);

  streamsize buffer_size () const;

  odevice::ptr get_device () const;

private:
  typedef shared_ptr< device<output> > device_ptr;

  output::ptr out_bottom_;      //!< %output aspect of stack's bottom element
  device_ptr  dev_bottom_;      //!< %device aspect of stack's bottom element
  device_ptr  device_;          //!< %device that caps the stack
  filter::ptr filter_;         //!< top-most %filter on the stack

  //!  Handles the internals of pushing a %device or %filter
  /*!  When pushing the first %device or %filter, its I/O aspect and
   *   %device aspect are recorded as the stack's bottom element.  A
   *   \a %buffer is not needed at this point and the caller should
   *   pass \c NULL to indicate this.  Pushing additional filters or
   *   capping %device requires a %buffer's I/O and %device aspects so
   *   we can %stream the I/O efficiently.
   *
   *   \note  The %buffer type needed is more derived than we can infer
   *          in this template.  Hence, derived classes need to create
   *          one and pass us a pointer to it.
   */
  void attach (output::ptr out, device_ptr device,
               output::ptr buf, buffer::ptr buffer);

  //!  Pushes %output and %device aspects on to the stack
  template< typename device_ptr >
  void push (output::ptr out, device_ptr device)
  {
    buffer::ptr buf;

    if (out_bottom_) buf = make_shared< buffer > (device->buffer_size ());

    attach (out, device, buf, buf);
  }
};

}       // namespace utsushi

#endif  /* utsushi_stream_hpp_ */
