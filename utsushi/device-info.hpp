//  device-info.hpp -- abstraction layer
//  Copyright (C) 2012, 2014, 2015  SEIKO EPSON CORPORATION
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

#ifndef utsushi_device_info_hpp_
#define utsushi_device_info_hpp_

#include <string>

#include "cstdint.hpp"
#include "memory.hpp"

namespace utsushi {

class device_info
{
public:
  typedef shared_ptr< device_info > ptr;

  static device_info::ptr
  create (const std::string& interface, const std::string& path);

  virtual ~device_info ();

  virtual uint16_t usb_vendor_id () const = 0;
  virtual uint16_t usb_product_id () const = 0;
  virtual std::string usb_serial () const = 0;
  virtual uint8_t usb_configuration () const = 0;
  virtual uint8_t usb_interface () const = 0;
  virtual uint8_t usb_bus_number () const = 0;
  virtual uint8_t usb_port_number () const = 0;
  virtual uint8_t usb_device_address () const = 0;
};

} // namespace utsushi

#endif  /* utsushi_device_info_hpp_ */
