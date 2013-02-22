//  hal.hpp -- OO wrapper around bits and pieces of the HAL API
//  Copyright (C) 2012  SEIKO EPSON CORPORATION
//  Copyright (C) 2008  Olaf Meeuwissen
//
//  License: GPL-3.0+
//  Author : AVASYS CORPORATION
//  Author : Olaf Meeuwissen
//  Origin : FreeRISCI
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

#ifndef _hal_hpp_
#define _hal_hpp_

#include <hal/libhal.h>

#include <string>

#include "utsushi/cstdint.hpp"
#include "utsushi/device-info.hpp"

namespace HAL {

    class device
      : public utsushi::device_info
    {
    public:
      // Old style HAL strings
      device (const std::string& id);
      // New style Utsushi path specifiers (based on sysfs)
      device (const std::string& type, const std::string& path);
      ~device (void);

      std::string subsystem (void) const;
      uint16_t usb_vendor_id (void) const;
      uint16_t usb_product_id (void) const;
      std::string usb_serial (void) const;
      uint8_t usb_configuration (void) const;
      uint8_t usb_interface (void) const;

    private:
      void get_property_(const std::string& name, int& value) const;
      void get_property_(const std::string& name, std::string& value) const;

      std::string udi_;

      LibHalContext *ctx_;
    };

}       // namespace HAL

#endif  /* _hal_hpp_ */
