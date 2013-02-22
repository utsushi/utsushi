//  iobase.hpp -- common aspects of image data I/O
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

#ifndef utsushi_iobase_hpp_
#define utsushi_iobase_hpp_

#include "context.hpp"
#include "octet.hpp"
#include "memory.hpp"

namespace utsushi {

//!  Common aspects of image data production
class input
{
public:
  typedef shared_ptr< input > ptr;

  virtual ~input ();

  //!  Produces up to \a n octets of image \a data
  /*!  \return the number of image data octets produced.  If no octets
   *   were produced a sequence marker is returned.
   */
  virtual streamsize read (octet *data, streamsize n) = 0;

  //!  Returns the value of the current sequence marker
  /*!  The marker is removed from the image data sequence.  If not at
   *   a sequence marker, a value different from all marker values is
   *   returned and the image data sequence is left untouched.
   */
  virtual streamsize marker () = 0;

  virtual void cancel () {};

  virtual streamsize buffer_size () const;
  virtual context get_context () const;

protected:
  input (const context& ctx = context ());

  streamsize buffer_size_;
  context ctx_;
};

//!  Common aspects of image data consumption
class output
{
public:
  typedef shared_ptr< output > ptr;

  virtual ~output ();

  //!  Consumes up to \a n octets of image \a data
  /*!  \return the number of image data octets consumed.  If no octets
   *   were consumed, zero will be returned.
   */
  virtual streamsize write (const octet *data, streamsize n) = 0;

  //!  Puts a sequence marker in the %output
  /*!  Objects that implement the output interface may need to perform
   *   some special actions whenever a sequence marker is encountered.
   *   This function provides a simple hook mechanism that dispatches
   *   based on the value of \a c.
   *
   *   \internal
   *   The bulk of the output implementations only need to override
   *   some of the protected hook functions to satisfy their needs.
   *   However, implementations that \em delegate to other %output
   *   implementers typically also need to override this function.
   */
  virtual void mark (traits::int_type c, const context& ctx);

  virtual streamsize buffer_size () const;
  virtual context get_context () const;

protected:
  output ();

  //!  Marks the beginning of a scan sequence
  virtual void bos (const context& ctx);
  //!  Marks the beginning of an image
  virtual void boi (const context& ctx);
  //!  Marks the end of an image
  virtual void eoi (const context& ctx);
  //!  Marks the end of a scan sequence
  virtual void eos (const context& ctx);
  //!  Marks the cancellation of image data production
  virtual void eof (const context& ctx);

  streamsize buffer_size_;
  context ctx_;
};

//!  Pipes all image data from \a iref to \a oref
/*!  This convenience operator checks that the %input \a iref is
 *   at the beginning of a scan sequence and, if so, proceeds to
 *   acquire images from \a iref.  Images acquired are sent to the
 *   %output \a oref.  This process continues until \a iref signals
 *   the end of the scan sequence.
 *   Begin and end of the scan sequence are marked on \a oref.
 *
 *   \return a sequence marker.  If completing successfully, that
 *           marker is traits::eos().
 *
 *   \sa operator>>
 */
streamsize operator|  (input& iref, output& oref);

//!  Acquires a single image from \a iref and sends it to \a oref
/*!  This convenience operator checks that the %input \a iref is
 *   at the beginning of an image and, if so, proceeds to acquire
 *   image data from \a iref.  Data acquired is sent to the %output
 *   \a oref. This process continues until \a iref signals the end
 *   of the image.
 *   Begin and end of the image are marked on \a oref.
 *
 *   \return a sequence marker.  If completing successfully, that
 *           marker is traits::eoi().
 *
 *   \sa operator|
 */
streamsize operator>> (input& iref, output& oref);

enum constants {
  default_buffer_size = 8192
};

}       // namespace utsushi

#endif  /* utsushi_iobase_hpp_ */
