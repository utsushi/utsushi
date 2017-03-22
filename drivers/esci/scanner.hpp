//  scanner.hpp -- API implementation for an ESC/I driver
//  Copyright (C) 2012, 2013, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2013  Olaf Meeuwissen
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

#ifndef drivers_esci_scanner_hpp_
#define drivers_esci_scanner_hpp_

#include <utsushi/connexion.hpp>
#include <utsushi/option.hpp>
#include <utsushi/scanner.hpp>

#include "matrix.hpp"
#include "vector.hpp"

namespace utsushi {

extern "C" {
  //! Create a suitable scanner object from the given \a info
  /*! This factory method contains all the smarts needed to set up a
   *  connexion and determine an appropriate scanner subclass for a
   *  device for the \a info given.  It has a basic understanding of
   *  the most fundamental bits of all supported protocol variants as
   *  well as detailed knowledge of the scanner class hierarchy that
   *  captures all the device (family) specific idiosyncracies.
   *
   *  After instantiating the object, it will be configured so that
   *  its options will be available at the point of return.
   *
   *  When unable to instantiate a suitable scanner object, the \a rv
   *  return value is not modified.
   */
  void libdrv_esci_LTX_scanner_factory (const scanner::info& info,
                                        scanner::ptr& rv);
}

namespace _drv_ {
namespace esci {

//! The root of all ESC/I scanner classes
class scanner
  : public utsushi::scanner
{
public:
  typedef shared_ptr< scanner > ptr;

  //! Makes the object configurable
  /*! Based on the information that the constructor has gathered, this
   *  function decides which options should be exposed and how.  It is
   *  very much like a final stage of object construction and the esci
   *  scanner factory calls it after instantiating one of the various
   *  subclasses.
   *
   *  By separating this step from the constructor proper, our direct
   *  base classes can implement most if not all of the required work
   *  while the instantiated subclass' constructor still has a chance
   *  to patch up what information the direct base class' constructor
   *  managed to collect while "talking" to the device.
   */
  virtual void configure () = 0;

protected:
  scanner (const connexion::ptr& cnx);

  const matrix< double, 3 > profile_matrix_;
  const vector< double, 3 > gamma_exponent_;

  //! Option values in effect during image acquisition
  /*! Storing the option values in this variable at the beginning of
   *  set_up_sequence() allows the implementation to assume that the
   *  values are not modified via external means for the duration of
   *  image acquisition.
   */
  value::map val_;

  /*! Stores the option values in val_ and calls the helper functions
   *  in an order that is thought to be suitable for \e all supported
   *  devices.  The chain of helper functions is guaranteed to start
   *  with a call to set_up_initialize() and return with a call to
   *  set_up_hardware ().
   */
  virtual bool set_up_sequence ();

  virtual void set_up_initialize ();

  //! Makes sure the device takes note of all set up activity
  virtual bool set_up_hardware ();

  // Helper functions for set_up_sequence() and set_up_image()
  // These functions are meant to cover configuring all functionality
  // of all supported devices so that set_up_sequence() can just call
  // them in a suitable order.  When settings should be sent on a per
  // scan basis, set_up_image() can refer to these functions as well.
  // The default implementations do nothing.

  virtual void set_up_auto_area_segmentation () {}
  virtual void set_up_brightness () {}
  virtual void set_up_color_matrices () {}
  virtual void set_up_dithering () {}
  // Meant to absorb film-type and other doc-source dependent options
  virtual void set_up_doc_source () {}
  virtual void set_up_gamma_tables () {}
  // Meant to cover color-mode, bit-depth and image-format (plus any
  // image-format dependent options)
  virtual void set_up_image_mode () {}
  virtual void set_up_mirroring () {}
  // Meant to cover/absorb zoom settings as well and shall be called
  // *before* set_up_scan_area ()
  virtual void set_up_resolution () {}
  virtual void set_up_scan_area () {}
  virtual void set_up_scan_count () {}
  virtual void set_up_scan_speed () {}
  virtual void set_up_sharpness () {}
  virtual void set_up_threshold () {}
  // Corresponds to line-count or buffer size
  virtual void set_up_transfer_size () {}
};

}       // namespace driver
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_scanner_hpp_ */
