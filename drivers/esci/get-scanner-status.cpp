//  get-scanner-status.cpp -- query for device status
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

#include <map>
#include <stdexcept>

#include <boost/assign/list_inserter.hpp>
#include <boost/throw_exception.hpp>

#include <utsushi/quantity.hpp>

#include "get-scanner-status.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

//! \todo Extract and merge with grammar-status.ccp code
namespace {

  using utsushi::length;

  const length inches = 1.0;
  const length mm     = inches / 25.4;

  typedef std::map< uint16_t, media > dictionary;
  dictionary *dict = nullptr;

  void initialize_dictionary ()
  {
    if (dict) return;

    dict = new dictionary;
    boost::assign::insert (*dict)
      (A3V , media (297 * mm, 420 * mm))
      (WLT , media (11.00 * inches, 17.00 * inches))
      (B4V , media (257 * mm, 364 * mm))
      (LGV , media ( 8.50 * inches, 14.00 * inches))
      (A4V , media (210 * mm, 297 * mm))
      (A4H , media (297 * mm, 210 * mm))
      (LTV , media ( 8.50 * inches, 11.00 * inches))
      (LTH , media (11.00 * inches,  8.50 * inches))
      (B5V , media (182 * mm, 257 * mm))
      (B5H , media (257 * mm, 182 * mm))
      (A5V , media (148 * mm, 210 * mm))
      (A5H , media (210 * mm, 148 * mm))
      (EXV , media ( 7.25 * inches, 10.50 * inches))
      (EXH , media (10.50 * inches,  7.25 * inches))
      (UNK , media (length (), length ()))
      ;
  }
}       // namespace
}       // namespace esci

  namespace esci
  {
    get_scanner_status::get_scanner_status (bool pedantic)
      : getter<FS,UPPER_F,16> (pedantic)
    {}

    uint8_t
    get_scanner_status::device_type (void) const
    {
      return 0x03 & (blk_[3] >> 6);
    }

    bool
    get_scanner_status::supports_size_detection (const source_value& source) const
    {
      return 0x0000 != media_value (source);
    }

    bool
    get_scanner_status::media_size_detected (const source_value& source) const
    {
      return (   supports_size_detection (source)
              && UNKNOWN != media_value (source));
    }

    media
    get_scanner_status::media_size (const source_value& source) const
    {
      if (!dict) initialize_dictionary ();

      return dict->at (media_value (source));
    }

    uint16_t
    get_scanner_status::media_value (const source_value& source) const
    {
      /**/ if (MAIN == source)
        return to_uint16_t (blk_ + 7);
      else if (ADF  == source)
        return to_uint16_t (blk_ + 5);

      BOOST_THROW_EXCEPTION (domain_error ("unsupported source"));
    }

    bool
    get_scanner_status::fatal_error (void) const
    {
      return 0x80 & blk_[0];
    }

    bool
    get_scanner_status::is_ready (void) const
    {
      return !(0x40 & blk_[0]);
    }

    bool
    get_scanner_status::is_warming_up (void) const
    {
      return 0x02 & blk_[0];
    }

    bool
    get_scanner_status::can_cancel_warming_up (void) const
    {
      return 0x01 & blk_[0];
    }

    bool
    get_scanner_status::main_error (void) const
    {
      return 0x20 & blk_[3];
    }

    bool
    get_scanner_status::main_media_out (void) const
    {
      return 0x08 & blk_[3];
    }

    bool
    get_scanner_status::main_media_jam (void) const
    {
      return 0x04 & blk_[3];
    }

    bool
    get_scanner_status::main_cover_open (void) const
    {
      return 0x02 & blk_[3];
    }

    bool
    get_scanner_status::adf_detected (void) const
    {
      return (   (0x80 & blk_[1])
              || (0x80 & blk_[10]));
    }

    bool
    get_scanner_status::adf_enabled (void) const
    {
      return (   (0x40 & blk_[1])
              || (0x40 & blk_[10]));
    }

    bool
    get_scanner_status::adf_error (void) const
    {
      return (   (0x20 & blk_[1])
              || (0x20 & blk_[10]));
    }

    bool
    get_scanner_status::adf_double_feed (void) const
    {
      return 0x10 & blk_[10];
    }

    bool
    get_scanner_status::adf_media_out (void) const
    {
      return 0x08 & blk_[1];
    }

    bool
    get_scanner_status::adf_media_jam (void) const
    {
      return 0x04 & blk_[1];
    }

    bool
    get_scanner_status::adf_cover_open (void) const
    {
      return 0x02 & blk_[1];
    }

    bool
    get_scanner_status::adf_tray_open (void) const
    {
      return 0x02 & blk_[10];
    }

    bool
    get_scanner_status::adf_is_duplexing (void) const
    {
      return 0x01 & blk_[1];
    }

    bool
    get_scanner_status::tpu_detected (const source_value& source) const
    {
      return tpu_status_(source, 0x80);
    }

    bool
    get_scanner_status::tpu_enabled (const source_value& source) const
    {
      return tpu_status_(source, 0x40);
    }

    bool
    get_scanner_status::tpu_error (const source_value& source) const
    {
      return tpu_status_(source, 0x20);
    }

    bool
    get_scanner_status::tpu_cover_open (const source_value& source) const
    {
      return tpu_status_(source, 0x02);
    }

    bool
    get_scanner_status::tpu_lamp_error (const source_value& source) const
    {
      return tpu_status_(source, 0x01);
    }

    bool
    get_scanner_status::has_holder_support (void) const
    {
      return 0x00 != blk_[4];
    }

    bool
    get_scanner_status::holder_error (void) const
    {
      return 0x80 & blk_[4];
    }

    byte
    get_scanner_status::holder_type (void) const
    {
      return 0x7f & blk_[4];
    }

    bool
    get_scanner_status::tpu_status_(const source_value& source,
                                    byte mask) const
    {
      /**/ if (TPU1 == source)
        return mask & blk_[2];
      else if (TPU2 == source)
        return mask & blk_[9];

      BOOST_THROW_EXCEPTION (domain_error ("unknown TPU index"));
    }

    void
    get_scanner_status::check_blk_reply (void) const
    {
      check_reserved_bits (blk_,  0, 0x3c);
      check_reserved_bits (blk_,  1, 0x10);
      check_reserved_bits (blk_,  2, 0x1c);
      check_reserved_bits (blk_,  3, 0x11);
      check_reserved_bits (blk_,  6, 0x02);
      check_reserved_bits (blk_,  8, 0x02);
      check_reserved_bits (blk_,  9, 0x1c);
      check_reserved_bits (blk_, 10, 0x0d);
      check_reserved_bits (blk_, 11, 0xff);
      check_reserved_bits (blk_, 12, 0xff);
      check_reserved_bits (blk_, 13, 0xff);
      check_reserved_bits (blk_, 14, 0xff);
      check_reserved_bits (blk_, 15, 0xff);
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
