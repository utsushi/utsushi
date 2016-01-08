//  scan-parameters.cpp -- settings for the next scan
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2008, 2013  Olaf Meeuwissen
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

#include "command.hpp"
#include "scan-parameters.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    bool
    scan_parameters::operator== (const scan_parameters& rhs) const
    {
      return 0 == traits::compare (mem_, rhs.mem_,
                                   sizeof (mem_) / sizeof (*mem_));
    }

    scan_parameters::scan_parameters (const byte mem[64])
      : mem_(mem)
    {}

    point<uint32_t>
    scan_parameters::resolution (void) const
    {
      return point<uint32_t> (to_uint32_t (mem_ + 0),
                              to_uint32_t (mem_ + 4));
    }

    bounding_box<uint32_t>
    scan_parameters::scan_area (void) const
    {
      point<uint32_t> offset (to_uint32_t (mem_ +  8),
                              to_uint32_t (mem_ + 12));
      point<uint32_t> extent (to_uint32_t (mem_ + 16),
                              to_uint32_t (mem_ + 20));

      return bounding_box<uint32_t> (offset, offset + extent);
    }

    byte
    scan_parameters::color_mode (void) const
    {
      return mem_[24];
    }

    uint8_t
    scan_parameters::line_count (void) const
    {
      return mem_[28];
    }

    uint8_t
    scan_parameters::bit_depth (void) const
    {
      return mem_[25];
    }

    byte
    scan_parameters::scan_mode (void) const
    {
      return mem_[27];
    }

    byte
    scan_parameters::option_unit (void) const
    {
      return mem_[26];
    }

    byte
    scan_parameters::film_type (void) const
    {
      return mem_[37];
    }

    bool
    scan_parameters::mirroring (void) const
    {
      return mem_[36];
    }

    bool
    scan_parameters::auto_area_segmentation (void) const
    {
      return mem_[34];
    }

    uint8_t
    scan_parameters::threshold (void) const
    {
      return mem_[33];
    }

    byte
    scan_parameters::halftone_processing (void) const
    {
      return mem_[32];
    }

    int8_t
    scan_parameters::sharpness (void) const
    {
      return mem_[35];
    }

    int8_t
    scan_parameters::brightness (void) const
    {
      return mem_[30];
    }

    byte
    scan_parameters::gamma_correction (void) const
    {
      return mem_[29];
    }

    byte
    scan_parameters::color_correction (void) const
    {
      return mem_[31];
    }

    byte
    scan_parameters::main_lamp_lighting_mode (void) const
    {
      return mem_[38];
    }

    byte
    scan_parameters::double_feed_sensitivity (void) const
    {
      return mem_[39];
    }

    byte
    scan_parameters::quiet_mode (void) const
    {
      return mem_[41];
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
