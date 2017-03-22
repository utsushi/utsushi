//  device-info.cpp -- abstraction layer
//  Copyright (C) 2012, 2013  SEIKO EPSON CORPORATION
//  Copyright (C) 2015  Olaf Meeuwissen
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "utsushi/device-info.hpp"

#include "utsushi/cstdint.hpp"
#include "utsushi/regex.hpp"

#if HAVE_LIBUDEV
#include "udev.hpp"
#endif

namespace utsushi {

device_info::ptr
device_info::create (const std::string& interface, const std::string& path)
{
  device_info::ptr rv;

  if (!rv && interface == "usb")
    {
      const regex re (  "(0x)?([[:xdigit:]]{1,4})"      // VID
                       ":(0x)?([[:xdigit:]]{1,4})"      // PID
                      "(:(.*))?");                      // serial
      smatch m;
      if (regex_match (path, m, re))
        {
          uint16_t vid = strtol (m[2].str().c_str (), NULL, 16);
          uint16_t pid = strtol (m[4].str().c_str (), NULL, 16);

#if HAVE_LIBUDEV
          rv = make_shared< udev_::device > (interface, vid, pid,
                                             m[6].str ());
#endif
        }
    }

#if HAVE_LIBUDEV
  if (!rv) rv = make_shared< udev_::device > (interface, path);
#endif

  return rv;
}

device_info::~device_info ()
{}

} // namespace utsushi
