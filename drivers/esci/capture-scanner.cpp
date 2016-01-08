//  capture-scanner.cpp -- gain exclusive device access
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
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

#include <boost/throw_exception.hpp>

#include "capture-scanner.hpp"
#include "exception.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    void
    capture_scanner::validate_reply (void) const
    {
      if (byte (0x80) == rep_)
        return;

      if (byte (0x40) == rep_)
        BOOST_THROW_EXCEPTION (device_busy ());

      if (NAK == rep_)
        BOOST_THROW_EXCEPTION (invalid_command ());

      BOOST_THROW_EXCEPTION (unknown_reply ());
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
