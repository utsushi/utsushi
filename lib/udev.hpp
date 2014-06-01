//  udev.hpp -- OO wrapper around bits and pieces of the libudev API
//  Copyright (C) 2013, 2014  SEIKO EPSON CORPORATION
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

#ifndef _udev_hpp_
#define _udev_hpp_

extern "C" {                    // needed until libudev-150
#include <libudev.h>
}

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

}       // namespace udev_

#endif  /* _udev_hpp_ */
