//  get-scan-parameters.cpp -- settings for the next scan
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2008  Olaf Meeuwissen
//
//  License: GPL-3.0+
//  Author : EPSON AVASYS CORPORATION
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "get-scan-parameters.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    get_scan_parameters::get_scan_parameters (bool pedantic)
      : getter<FS,UPPER_S,64> (pedantic)
      , scan_parameters (blk_)
    {}

    void
    get_scan_parameters::check_blk_reply (void) const
    {
      check_reserved_bits (blk_, 39, 0xfc);
      check_reserved_bits (blk_, 40, 0xff);
      check_reserved_bits (blk_, 41, 0xfc);
      for (streamsize i = 42; i < 64; ++i)
        {
          check_reserved_bits (blk_, i, 0xff);
        }
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
