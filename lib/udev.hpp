//  udev.hpp -- OO wrapper around bits and pieces of the libudev API
//  Copyright (C) 2013-2015  SEIKO EPSON CORPORATION
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

#ifndef _udev_hpp_
#define _udev_hpp_

extern "C" {                    // needed until libudev-150
#include <libudev.h>
}

#include <ios>
#include <string>

#include "utsushi/cstdint.hpp"
#include "utsushi/device-info.hpp"

namespace udev_ {

using utsushi::uint8_t;
using utsushi::uint16_t;

class device
  : public utsushi::device_info
{
public:
  device (const std::string& interface, const std::string& path);
  device (const std::string& interface,
          uint16_t vendor_id, uint16_t product_id,
          const std::string& serial_number = std::string ());
  ~device ();

  std::string subsystem () const;
  uint16_t usb_vendor_id () const;
  uint16_t usb_product_id () const;
  std::string usb_serial () const;
  uint8_t usb_configuration () const;
  uint8_t usb_interface () const;
  uint8_t usb_bus_number () const;
  uint8_t usb_port_number () const;
  uint8_t usb_device_address () const;

private:
  struct udev_device *dev_;
};

// Convenience API

//! Find a property's \a value by \a name for a given \a device
/*! If the \a device does not advertise the property, its parent will
 *  be queried and so on until the root of the device hierarchy.  The
 *  \a value is only modified if a matching property is found.
 */
void get_property (struct udev_device *device,
                   const std::string& name, std::string& value);

//! Find a system attribute's \a value by \a name for a given \a device
/*! If the \a device does not advertise the attribute, its parent will
 *  be queried and so on until the root of the device hierarchy.  The
 *  \a value is only modified if a matching attribute is found.
 */
void get_sysattr (struct udev_device *device,
                  const std::string& name, int& value,
                  std::ios_base& (radix)(std::ios_base&) = std::hex);

}       // namespace udev_

#endif  /* _udev_hpp_ */
