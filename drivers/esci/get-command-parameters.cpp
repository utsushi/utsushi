//  get-command-parameters.cpp -- settings for the next scan
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

#include "get-command-parameters.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    get_command_parameters::get_command_parameters (bool pedantic)
      : buf_getter<ESC,UPPER_S> (pedantic)
    {}

    point<uint32_t>
    get_command_parameters::resolution (void) const
    {
      return point<uint32_t> (to_uint16_t (dat_ + 3),
                              to_uint16_t (dat_ + 5));
    }

    point<uint8_t>
    get_command_parameters::zoom (void) const
    {
      return point<uint8_t> (dat_[25], dat_[26]);
    }

    bounding_box<uint32_t>
    get_command_parameters::scan_area (void) const
    {
      point<uint32_t> offset (to_uint16_t (dat_ +  8),
                              to_uint16_t (dat_ + 10));
      point<uint32_t> extent (to_uint16_t (dat_ + 12),
                              to_uint16_t (dat_ + 14));

      return bounding_box<uint32_t>
        (offset, point<uint32_t> (extent.x () + offset.x (),
                                  extent.y () + offset.y ()));
    }

    byte
    get_command_parameters::color_mode (void) const
    {
      return dat_[1];
    }

    uint8_t
    get_command_parameters::line_count (void) const
    {
      return dat_[40];
    }

    uint8_t
    get_command_parameters::bit_depth (void) const
    {
      return dat_[17];
    }

    byte
    get_command_parameters::scan_mode (void) const
    {
      return dat_[32];
    }

    byte
    get_command_parameters::option_unit (void) const
    {
      return dat_[42];
    }

    byte
    get_command_parameters::film_type (void) const
    {
      return dat_[44];
    }

    bool
    get_command_parameters::mirroring (void) const
    {
      return dat_[34];
    }

    bool
    get_command_parameters::auto_area_segmentation (void) const
    {
      return dat_[36];
    }

    uint8_t
    get_command_parameters::threshold (void) const
    {
      return dat_[38];
    }

    byte
    get_command_parameters::halftone_processing (void) const
    {
      return dat_[19];
    }

    int8_t
    get_command_parameters::sharpness (void) const
    {
      return dat_[30];
    }

    int8_t
    get_command_parameters::brightness (void) const
    {
      return dat_[21];
    }

    byte
    get_command_parameters::gamma_correction (void) const
    {
      return dat_[23];
    }

    byte
    get_command_parameters::color_correction (void) const
    {
      return dat_[28];
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
