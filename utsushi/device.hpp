//  device.hpp -- interface declarations
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

#ifndef utsushi_device_hpp_
#define utsushi_device_hpp_

#include <csignal>

#include "iobase.hpp"
#include "memory.hpp"
#include "option.hpp"
#include "signal.hpp"

#include "pattern/decorator.hpp"

namespace utsushi {

//!  Access or store a "raw" image data sequence
template< typename IO >
class device
  : public configurable
{
public:
  typedef shared_ptr< device > ptr;

  virtual ~device () {}

  typedef signal< void (traits::int_type) >       marker_signal_type;
  typedef signal< void (streamsize, streamsize) > update_signal_type;

  connection connect_marker (const marker_signal_type::slot_type& slot) const
  {
    return signal_marker_.connect (slot);
  }

  connection connect_update (const update_signal_type::slot_type& slot) const
  {
    return signal_update_.connect (slot);
  }

protected:
  device () {}

  mutable marker_signal_type signal_marker_;
  mutable update_signal_type signal_update_;
};

//!  Interface for image data producers
class idevice
  : public device< input >
  , public input
{
public:
  typedef shared_ptr< idevice > ptr;

  streamsize read (octet *data, streamsize n);
  streamsize marker ();

  //! Requests cancellation of image data production
  /*! This method prepares the instance to return traits::eof(), the
   *  cancellation marker, on a future invocation of read().
   *
   *  The implementation arranges for the cancellation marker to be
   *  returned when read() would normally return traits::eos(), the
   *  end-of-sequence marker, and calls cancel_().  This arrangement
   *  allows instances to produce image data until a suitable point
   *  to cancel has been reached.  Instances can cancel "mid-image"
   *  by returning traits::eof() from sgetn().  They can cancel at
   *  the end of any given image by returning \c false from one of
   *  set_up_image(), obtain_media() or is_consecutive().
   *
   *  Although this function is typically called in response to user
   *  input, it may be called by the instance itself when it detects
   *  a cancellation request from the device.  Error recovery is yet
   *  another situation where one may want to cancel the acquisition
   *  of image data.
   */
  void cancel ();

  using input::buffer_size;
  virtual void buffer_size (streamsize size);

  //! Hint whether the scan sequence will consist of a single image
  /*! There is \e no way the input device can be certain of this.
   *  Filters in the stream may very well split an incoming image into
   *  multiple images (when splitting film negatives into individual
   *  frames for example) or suppress images altogether (during empty
   *  page removal in ADF type scans).
   *
   *  \todo Remove stream construction kludge
   */
  virtual bool is_single_image () const;

protected:
  idevice (const context& ctx = context ());

  //!  Attempts to prepare the object for a new scan sequence
  /*!  The default implementation does nothing and always succeeds.
   *
   *   \return  \c true if successful, \c false otherwise.
   *   \sa set_up_image()
   */
  virtual bool set_up_sequence ();

  //!  Says whether a scan sequence may produce multiple images
  /*!  The default implementation returns \c false, which is normally
   *   the correct thing to do for glass plate based scanning devices.
   */
  virtual bool is_consecutive () const;

  //!  Attempts to provide the device with new image media
  /*!  The default implementation returns \c true, which is typically
   *   the correct thing to do for glass plate based scanning devices.
   *   Devices with an automated document feeder or a film transporter
   *   need to override this method in order to turn the current sheet
   *   over (in the case of duplex scans) or advance to the next sheet
   *   or film frame.
   *
   *   \return  \c true if media was obtained, \c false otherwise.
   */
  virtual bool obtain_media ();

  //!  Attempts to prepare the object for a new image
  /*!  The default implementation never succeeds.
   *
   *   \return  \c true if successful, \c false otherwise.
   *   \sa set_up_sequence()
   */
  virtual bool set_up_image ();

  //!  Releases resources acquired during set_up_image() and sgetn()
  /*!  The default implementation does nothing.
   */
  virtual void finish_image ();

  //!  Produces up to \a n octets of image \a data.
  /*!  The default implementation never produces any octets.
   *
   *   \return  Number of image data octets produced or traits::eof().
   *            If no more octets can be produced, \c 0 is returned,
   *            indicating that all data for the \e current image has
   *            been produced.  Returning traits::eof() cancels the
   *            image acquisition process "mid-image" (and independent
   *            of whether cancel() was called) even if no more octets
   *            can be produced.
   */
  virtual streamsize sgetn (octet *data, streamsize n);

  //! Initiates cancellation of image data production
  /*! The default implementation does nothing.  This effectively makes
   *  the device acquire all image data anyway, but still notifies the
   *  processing pipeline that a cancel was requested.
   *
   *  Subclasses are expected to override the default implementation.
   *  A typical override will arrange for cancellation on the hardware
   *  side and release resources that are no longer needed.
   */
  virtual void cancel_();

  traits::int_type last_marker_;
  sig_atomic_t     do_cancel_;

private:
  streamsize read_(octet *data, streamsize n);
};

//!  Interface for image data consumers
class odevice
  : public device< output >
  , public output
{
public:
  typedef shared_ptr< odevice > ptr;

  void mark (traits::int_type c, const context& ctx);

  using output::buffer_size;
  virtual void buffer_size (streamsize size);
};

//!  Add responsibilities to an \c idevice
/*!  Meant as a convenient starting point for any idevice decorator,
 *   this class implements the full \em public idevice API by simply
 *   forwarding the API call to the decorated object.  This way, any
 *   subclass only needs to override those parts that require added
 *   responsibilities.
 */
template<>
class decorator< idevice >
  : public idevice
{
public:
  typedef shared_ptr< idevice > ptr;

  decorator (ptr instance);

  streamsize read (octet *data, streamsize n);
  streamsize marker ();
  void cancel ();

  streamsize buffer_size () const;
  void buffer_size (streamsize size);
  context get_context () const;

protected:
  ptr instance_;
};

//!  Add responsibilities to an \c odevice
/*!  Meant as a convenient starting point for any odevice decorator,
 *   this class implements the full \em public odevice API by simply
 *   forwarding the API call to the decorated object.  This way, any
 *   subclass only needs to override those parts that require added
 *   responsibilities.
 */
template<>
class decorator< odevice >
  : public odevice
{
public:
  typedef shared_ptr< odevice > ptr;

  decorator (ptr instance);

  streamsize write (const octet *data, streamsize n);
  void mark (traits::int_type c, const context& ctx);

  streamsize buffer_size () const;
  void buffer_size (streamsize size);
  context get_context () const;

protected:
  ptr instance_;
};

}       // namespace utsushi

#endif  /* utsushi_device_hpp_ */
