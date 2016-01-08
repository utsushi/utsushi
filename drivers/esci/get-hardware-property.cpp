//  get-hardware-property.cpp -- probe additional capabilities
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

#include "exception.hpp"
#include "get-hardware-property.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    get_hardware_property::get_hardware_property (bool pedantic)
      : buf_getter<ESC,LOWER_I> (pedantic)
    {}

    uint32_t
    get_hardware_property::base_resolution (void) const
    {
      return to_uint16_t (dat_);
    }

    bool
    get_hardware_property::is_cis (void) const
    {
      return !(0x80 & dat_[2]);
    }

    uint8_t
    get_hardware_property::sensor_type (void) const
    {
      return 0x40 & dat_[2];
    }

    color_value
    get_hardware_property::color_sequence (void) const
    {
      /**/ if (0 == dat_[3])
        return RGB;
      else
        BOOST_THROW_EXCEPTION (range_error ("undocumented color sequence"));
    }

    uint8_t
    get_hardware_property::line_number (const color_value& c) const
    {
      uint8_t shift  = 0;

      /**/ if (RED   == c)
        shift = 4;
      else if (GREEN == c)
        shift = 2;
      else if (BLUE  == c)
        shift = 0;
      else
        BOOST_THROW_EXCEPTION (range_error ("undocumented color value"));

      return 0x03 & (dat_[2] >> shift);
    }

    point<uint8_t>
    get_hardware_property::line_spacing (void) const
    {
      return point<uint8_t> (dat_[4], dat_[5]);
    }

    std::set<uint32_t>
    get_hardware_property::x_resolutions (void) const
    {
      return resolutions_(dat_ + 14);
    }

    std::set<uint32_t>
    get_hardware_property::y_resolutions (void) const
    {
      const byte *p = dat_ + 14;

      while (p < (dat_ + size () - 2)
             && 0 != to_uint16_t (p))
        {
          p += 2;
        }
      return resolutions_(p + 2);
    }

    std::set<uint32_t>
    get_hardware_property::resolutions_(const byte *p) const
    {
      std::set<uint32_t> result;

      while (p < (dat_ + size () - 2)
             && 0 != to_uint16_t (p))
        {
          result.insert (to_uint16_t (p));
          p += 2;
        }
      return result;
    }

    void
    get_hardware_property::check_data_block (void) const
    {
      check_reserved_bits (dat_,  6, 0xff);
      check_reserved_bits (dat_,  7, 0xff);
      check_reserved_bits (dat_,  8, 0xff);
      check_reserved_bits (dat_,  9, 0xff);
      check_reserved_bits (dat_, 10, 0xff);
      check_reserved_bits (dat_, 11, 0xff);
      check_reserved_bits (dat_, 12, 0xff);
      check_reserved_bits (dat_, 13, 0xff);
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
