//  device.hpp -- OO wrapper for SANE_Device instances
//  Copyright (C) 2012, 2013, 2015  SEIKO EPSON CORPORATION
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

#ifndef sane_device_hpp_
#define sane_device_hpp_

extern "C" {                    // needed until sane-backends-1.0.14
#include <sane/sane.h>
}

#include <string>
#include <vector>

#include <utsushi/scanner.hpp>

namespace sane {

//! Wraps a \c SANE_Device and handles resource allocation issues
/*! \rationale
 *  The SANE C API specification dictates that a list of devices
 *  returned by sane_get_devices() remains valid and \e unchanged
 *  until another call to that function or a call to sane_exit().
 *  This means that all the strings of each \c SANE_Device have to
 *  be owned by the backend because there is no guarantee that the
 *  utsushi::monitor will keep around all the scanner::info objects
 *  we use to create a list of SANE_Device's for the whole of that
 *  time frame.
 *
 *  We wrap \c SANE_Device objects in a thin C++ layer to make sure
 *  that we have ownership of strings returned by scanner::info API.
 *  That way, we can safely set the \c SANE_Device members to point
 *  to c_str() return values.
 */
class device : public SANE_Device
{
  std::string name_;
  std::string vendor_;
  std::string model_;
  std::string type_;

  //! Implements the common part of object creation and copying
  void init ();

public:
  //! Creates an instance from a scanner \a info
  device (const utsushi::scanner::info& info);
  //! Creates a copy of a device object
  device (const device& dev);
  //! Assigns one device to another
  device& operator= (const device& dev);

  //! Releases all resources associated with a \c SANE_Device list
  static void release ();

  //! Holds on to an array of \c SANE_Device pointers
  /*! The backend "remembers" the last \c device::list it has returned
   *  through sane_get_devices() via this static member variable.  The
   *  backend owns the resources associated with that list and handles
   *  the release() of these resources when appropriate.
   */
  static const SANE_Device **list;

  //! Holds on to the resources associated with a \c SANE_Device list
  /*! This static member variable holds on to the objects pointed to
   *  by the elements of list.
   */
  static std::vector< device > *pool;
};

}       // namespace sane

#endif  /* sane_device_hpp_ */
