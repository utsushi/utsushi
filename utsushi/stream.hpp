//  stream.hpp -- interface declarations
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
template <typename IO>
class stream
{
  typedef shared_ptr< filter<IO> > filter_ptr;

protected:
  typedef shared_ptr<        IO  > io_ptr;
  typedef shared_ptr< device<IO> > device_ptr;
  typedef shared_ptr< buffer<IO> > buffer_ptr;

  io_ptr     io_bottom_;        //!< I/O aspect of stack's bottom element
  device_ptr devbottom_;        //!< %device aspect of stack's bottom element
  device_ptr device_;           //!< %device that caps the stack
  filter_ptr filter_;           //!< top-most %filter on the stack

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
  void attach (io_ptr io_dev, device_ptr device,
               io_ptr io_buf, buffer_ptr buffer)
  {
    if (buffer) {
      buffer ->open (io_dev);
      filter_->open (io_buf);
    } else {
      io_bottom_ = io_dev;
      devbottom_ = device;
    }
  }

public:
  typedef shared_ptr<stream> ptr;

  streamsize buffer_size () const
  { return devbottom_->buffer_size (); }

  device_ptr get_device () const
  { return devbottom_; }
};

//!  Image data producing %streams
class istream : public stream<input>, public input
{
  typedef stream<input> base;

public:
  typedef shared_ptr<istream> ptr;

  streamsize read (octet *data, streamsize n);
  streamsize marker ();
  void cancel () { if (io_bottom_) io_bottom_->cancel (); }

  context get_context () const;

  //!  Pushes a \a %device onto the object's %input stack
  /*!  This completes the object's stack and one can now read() image
   *   data from the \a %device.
   */
  void push (idevice::ptr device);

  //!  Pushes a \a %filter onto the object's %input stack
  /*!  Image data acquired from the object's device will be processed
   *   by all filters on the stack, starting with the \a %filter that
   *   was pushed \e last, before it is made available to read().
   */
  void push (ifilter::ptr filter);

private:
  //!  Pushes %input and %device aspects on to the stack
  template< typename device_ptr >
  void push (io_ptr io, device_ptr device)
  {
    ibuffer::ptr buf (io_bottom_
                      ? new ibuffer (device->buffer_size ())
                      : NULL);
    attach (io, device, buf, buf);
  }
};


//!  Image data consuming %streams
class ostream : public stream<output>, public output
{
  typedef stream<output> base;

public:
  typedef shared_ptr<ostream> ptr;

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
  void push (ofilter::ptr filter);

private:
  //!  Pushes %output and %device aspects on to the stack
  template< typename device_ptr >
  void push (io_ptr io, device_ptr device)
  {
    obuffer::ptr buf (io_bottom_
                      ? new obuffer (device->buffer_size ())
                      : NULL);
    attach (io, device, buf, buf);
  }
};

}       // namespace utsushi

#endif  /* utsushi_stream_hpp_ */
