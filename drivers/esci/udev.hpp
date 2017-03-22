//  udev.hpp -- convenience API
//  Copyright (C) 2016  SEIKO EPSON CORPORATION
//
//  License: BSL-1.0
//  Author : EPSON AVASYS CORPORATION
//
//  This file is part of the 'Utsushi' package.
//  This file is distributed under the terms of the Boost Software
//  License, Version 1.0.
//
//  You ought to have received a copy of the Boost Software License
//  along along with this package.
//  If not, see <http://www.boost.org/LICENSE_1_0.txt>.

#ifndef drivers_esci_udev_hpp_
#define drivers_esci_udev_hpp_

#if __cplusplus >= 201103L
#include <cstdint>
#else
#include <stdint.h>
#endif

extern "C" {                    // needed until libudev-150
#include <libudev.h>
}

class udev_info
{
public:
  udev_info (const char *path);
  ~udev_info ();

  uint16_t usb_vendor_id () const;
  uint16_t usb_product_id () const;

  uint8_t usb_configuration () const;
  uint8_t usb_interface () const;

  uint8_t usb_bus_number () const;
  uint8_t usb_device_address () const;
  uint8_t usb_port_number () const;

private:
  struct udev_device *device_;
};

#endif /* drivers_esci_udev_hpp_ */
