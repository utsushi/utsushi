//  get-extended-status.cpp -- query for device status
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

#include <stdexcept>

#include <boost/throw_exception.hpp>

#include "get-extended-status.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    get_extended_status::get_extended_status (bool pedantic)
      : buf_getter<ESC,LOWER_F> (pedantic)
    {}

    std::string
    get_extended_status::product_name (void) const
    {
      return to_string (dat_ + 26, 16);
    }

    bool
    get_extended_status::supports_size_detection (const source_value& source) const
    {
      return 0x0000 != media_value (source);
    }

    uint16_t
    get_extended_status::media_value (const source_value& source) const
    {
      /**/ if (MAIN == source)
        return to_uint16_t (blk_ + 18);
      else if (ADF  == source)
        return to_uint16_t (blk_ + 16);

      BOOST_THROW_EXCEPTION (domain_error ("unsupported source"));
    }

    uint8_t
    get_extended_status::device_type (void) const
    {
      return 0x03 & (dat_[11] >> 6);
    }

    bool
    get_extended_status::is_flatbed_type (void) const
    {
      return !(0x40 & dat_[0]);
    }

    bool
    get_extended_status::has_lid_option (void) const
    {
      return 0x04 & dat_[0];
    }

    bool
    get_extended_status::has_push_button (void) const
    {
      return 0x01 & dat_[0];
    }

    bool
    get_extended_status::fatal_error (void) const
    {
      return 0x80 & dat_[0];
    }

    bool
    get_extended_status::is_warming_up (void) const
    {
      return 0x02 & dat_[0];
    }

    bool
    get_extended_status::main_error (void) const
    {
      return 0x20 & dat_[11];
    }

    bool
    get_extended_status::main_media_out (void) const
    {
      return 0x08 & dat_[11];
    }

    bool
    get_extended_status::main_media_jam (void) const
    {
      return 0x04 & dat_[11];
    }

    bool
    get_extended_status::main_cover_open (void) const
    {
      return 0x02 & dat_[11];
    }

    bool
    get_extended_status::adf_detected (void) const
    {
      return 0x80 & dat_[1];
    }

    bool
    get_extended_status::adf_is_page_type (void) const
    {
      return 0x20 & dat_[0];
    }

    bool
    get_extended_status::adf_is_duplex_type (void) const
    {
      return 0x10 & dat_[0];
    }

    bool
    get_extended_status::adf_is_first_sheet_loader (void) const
    {
      return 0x08 & dat_[0];
    }

    bool
    get_extended_status::adf_enabled (void) const
    {
      return 0x40 & dat_[1];
    }

    bool
    get_extended_status::adf_error (void) const
    {
      return 0x20 & dat_[1];
    }

    bool
    get_extended_status::adf_double_feed (void) const
    {
      return 0x10 & dat_[1];
    }

    bool
    get_extended_status::adf_media_out (void) const
    {
      return 0x08 & dat_[1];
    }

    bool
    get_extended_status::adf_media_jam (void) const
    {
      return 0x04 & dat_[1];
    }

    bool
    get_extended_status::adf_cover_open (void) const
    {
      return 0x02 & dat_[1];
    }

    bool
    get_extended_status::adf_is_duplexing (void) const
    {
      return 0x01 & dat_[1];
    }

    bool
    get_extended_status::tpu_detected (void) const
    {
      return 0x80 & dat_[6];
    }

    bool
    get_extended_status::tpu_enabled (void) const
    {
      return 0x40 & dat_[6];
    }

    bool
    get_extended_status::tpu_error (void) const
    {
      return 0x20 & dat_[6];
    }

    bool
    get_extended_status::tpu_cover_open (void) const
    {
      return 0x02 & dat_[6];
    }

    bounding_box<uint32_t>
    get_extended_status::scan_area (const source_value& source) const
    {
      streamsize offset;

      /**/ if (MAIN == source)
        offset = 12;
      else if (ADF  == source)
        offset =  2;
      else if (TPU1 == source)
        offset =  7;
      else
        BOOST_THROW_EXCEPTION (domain_error ("unsupported source"));

      return point<uint32_t> (to_uint16_t (dat_ + offset),
                              to_uint16_t (dat_ + offset + 2));
    }

    void
    get_extended_status::check_data_block (void) const
    {
      check_reserved_bits (dat_,  6, 0x1d);
      check_reserved_bits (dat_, 11, 0x11);
      check_reserved_bits (dat_, 17, 0x02);
      check_reserved_bits (dat_, 19, 0x02);
      check_reserved_bits (dat_, 20, 0xff);
      check_reserved_bits (dat_, 21, 0xff);
      check_reserved_bits (dat_, 22, 0xff);
      check_reserved_bits (dat_, 23, 0xff);
      check_reserved_bits (dat_, 24, 0xff);
      check_reserved_bits (dat_, 25, 0xff);
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
