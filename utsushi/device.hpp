//  device.hpp -- interface declarations
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

  virtual connection connect_marker (const marker_signal_type::slot_type& slot) const
  {
    return signal_marker_.connect (slot);
  }

  virtual connection connect_update (const update_signal_type::slot_type& slot) const
  {
    return signal_update_.connect (slot);
  }

protected:
  device ()
    : last_marker_(traits::eof ())
  {}

  traits::int_type last_marker_;

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
   *  Although this function is typically called in response to user
   *  input, it may be called by the instance itself when it detects
   *  a cancellation request from the device.  Error recovery is yet
   *  another situation where one may want to cancel the acquisition
   *  of image data.
   *
   *  It is safe to call this function asynchronously.  It will only
   *  initiate cancellation and return immediately.  Its return does
   *  \e not indicate that cancellation has completed.  Cancellation
   *  has only completed after a subsequent call to read() returns a
   *  traits::eof() or traits::eos() value.  In the latter case, the
   *  cancellation request was ignored.
   *
   *  The implementation arranges for the cancellation marker to be
   *  returned when read() would normally return traits::eos(), the
   *  end-of-sequence marker.  This arrangement allows instances to
   *  produce image data until a suitable point to cancel has been
   *  reached.  Instances can either cancel "mid-image" by returning
   *  traits::eof() from sgetn() or at the end of any given image by
   *  returning \c false from one of set_up_image(), obtain_media(),
   *  is_consecutive() or even set_up_sequence().
   *
   *  \sa cancel_requested()
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

  option::map::ptr actions ();

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

  //! Tells whether cancellation has been requested
  /*! Device implementations that want to support cancellation of the
   *  image acquisition process can use this query to check whether a
   *  request for cancellation has been made.
   *
   *  Care should be taken using this function as its return value may
   *  change asynchronously.  That means that this code
   *
   *  \code
   *  if (!cancel_requested () && cancel_requested ())
   *    {
   *      std::clog << "state changed between calls";
   *    }
   *  \endcode
   *
   *  could in fact produce output, odd as this may seem at first.  We
   *  suggest that functions locally cache the result of a single call
   *  and use the cached value instead.  Doing so makes implementing
   *  your functions a lot easier.  When caching, it is best to cache
   *  at the smallest convenient scope so that cancellation requests
   *  are still noticed as soon as possible.  For example, inside the
   *  body of an iteration statement rather than at function scope.
   *
   *  Using caching, the example from above becomes
   *
   *  \code
   *  bool do_cancel = cancel_requested ();
   *  if (!do_cancel && do_cancel)
   *    {
   *      std::clog << "state changed between calls";
   *    }
   *  \endcode
   *
   *  and be guaranteed to produce no output, ever.
   *
   *  \return  \c true if image acquisition should be cancelled,
   *           \c false otherwise
   */
  bool cancel_requested () const;

  option::map::ptr action_;

private:
  streamsize read_(octet *data, streamsize n);

  //! Image acquisition process state tracker
  /*! When this variable's value equals \c true, image acquisition is
   *  a work_in_progress_.  If \c false there is, very unsurprisingly,
   *  no work_in_progress_.
   *
   *  The value of this variable is maintained by read() and changes
   *  to \c true just before an attempt to change to traits::bos() is
   *  made.  This is meant to allow for cancellation support in time
   *  consuming set_up_sequence() and obtain_media() implementations.
   *  The variable is reset to \c false when the instance transitions
   *  to either traits::eos() or traits::eof().  The cancel_requested_
   *  state tracker is cleared immediately after this (so intervening
   *  invocations of cancel() cannot preemptively cancel any upcoming
   *  work_in_progress_).
   */
  sig_atomic_t work_in_progress_;       // ORDER DEPENDENCY

  //! Cancellation request state tracker
  /*! Reality check!  If there is no work_in_progress_ then it cannot
   *  be cancelled.  For this very reason, cancel_requested_ is only
   *  ever initialized using work_in_progress_ (even at construction
   *  time!).  As long as the latter value is set to \c false before
   *  any assignment, the cancel() function can \e never trigger any
   *  cancellation of work that is yet to be started.
   */
  sig_atomic_t cancel_requested_;
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
  bool is_single_image () const;

  connection connect_marker (const marker_signal_type::slot_type&) const;
  connection connect_update (const update_signal_type::slot_type&) const;

  option::map::ptr options ();

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
