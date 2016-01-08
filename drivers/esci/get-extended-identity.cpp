//  get-extended-identity.cpp -- probe for capabilities
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2009  Olaf Meeuwissen
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

#include "get-extended-identity.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    get_extended_identity::get_extended_identity (bool pedantic)
      : getter<FS,UPPER_I,80> (pedantic)
    {}

    std::string
    get_extended_identity::command_level (void) const
    {
      return to_string (blk_ + 0, 2);
    }

    std::string
    get_extended_identity::product_name (void) const
    {
      return to_string (blk_ + 46, 16);
    }

    std::string
    get_extended_identity::rom_version (void) const
    {
      return to_string (blk_ + 62, 4);
    }

    uint32_t
    get_extended_identity::base_resolution (void) const
    {
      return to_uint32_t (blk_ + 4);
    }

    uint32_t
    get_extended_identity::min_resolution (void) const
    {
      return to_uint32_t (blk_ + 8);
    }

    uint32_t
    get_extended_identity::max_resolution (void) const
    {
      return to_uint32_t (blk_ + 12);
    }

    uint32_t
    get_extended_identity::max_scan_width (void) const
    {
      return to_uint32_t (blk_ + 16);
    }

    bounding_box<uint32_t>
    get_extended_identity::scan_area (const source_value& source) const
    {
      streamsize offset;

      /**/ if (MAIN == source)
        offset = 20;
      else if (ADF  == source)
        offset = 28;
      else if (TPU1 == source)
        offset = 36;
      else if (TPU2 == source)
        offset = 68;
      else
        BOOST_THROW_EXCEPTION (domain_error ("unsupported source"));

      return point<uint32_t> (to_uint32_t (blk_ + offset),
                              to_uint32_t (blk_ + offset + 4));
    }

    bool
    get_extended_identity::is_flatbed_type (void) const
    {
      return !(0x40 & blk_[44]);
    }

    bool
    get_extended_identity::has_lid_option (void) const
    {
      return 0x04 & blk_[44];
    }

    bool
    get_extended_identity::has_push_button (void) const
    {
      return 0x01 & blk_[44];
    }

    bool
    get_extended_identity::adf_is_page_type (void) const
    {
      return 0x20 & blk_[44];
    }

    bool
    get_extended_identity::adf_is_duplex_type (void) const
    {
      return 0x10 & blk_[44];
    }

    bool
    get_extended_identity::adf_is_first_sheet_loader (void) const
    {
      return 0x08 & blk_[44];
    }

    bool
    get_extended_identity::tpu_is_IR_type (void) const
    {
      return 0x02 & blk_[44];
    }

    bool
    get_extended_identity::supports_lamp_change (void) const
    {
      return 0x80 & blk_[44];
    }

    bool
    get_extended_identity::detects_page_end (void) const
    {
      return 0x01 & blk_[45];
    }
    bool
    get_extended_identity::has_energy_savings_setter (void) const
    {
      return 0x02 & blk_[45];
    }

    bool
    get_extended_identity::adf_is_auto_form_feeder (void) const
    {
      return 0x04 & blk_[45];
    }

    bool
    get_extended_identity::adf_detects_double_feed (void) const
    {
      return 0x08 & blk_[45];
    }

    bool
    get_extended_identity::supports_auto_power_off (void) const
    {
      return 0x10 & blk_[45];
    }

    bool
    get_extended_identity::supports_quiet_mode (void) const
    {
      return 0x20 & blk_[45];
    }

    bool
    get_extended_identity::supports_authentication (void) const
    {
      return 0x40 & blk_[45];
    }

    bool
    get_extended_identity::supports_compound_commands (void) const
    {
      return 0x80 & blk_[45];
    }

    byte
    get_extended_identity::bit_depth (const io_direction& io) const
    {
      /**/ if (INPUT  == io)
        return blk_[66];
      else if (OUTPUT == io)
        return blk_[67];

      BOOST_THROW_EXCEPTION (logic_error ("unsupported io_direction"));
    }

    byte
    get_extended_identity::document_alignment (void) const
    {
      return 0x03 & blk_[76];
    }

    void
    get_extended_identity::check_blk_reply (void) const
    {
      check_reserved_bits (blk_,  2, 0xff);
      check_reserved_bits (blk_,  3, 0xff);
      check_reserved_bits (blk_, 76, 0xfc);
      check_reserved_bits (blk_, 77, 0xff);
      check_reserved_bits (blk_, 78, 0xff);
      check_reserved_bits (blk_, 79, 0xff);
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
