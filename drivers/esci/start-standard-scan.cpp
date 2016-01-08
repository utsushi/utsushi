//  start-standard-scan.cpp -- to acquire image data
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

#include <boost/throw_exception.hpp>

#include "action.hpp"
#include "setter.hpp"
#include "start-standard-scan.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    const byte start_standard_scan::cmd_[2] = { ESC, UPPER_G };

    start_standard_scan::start_standard_scan (uint8_t line_count,
                                              bool pedantic)
      : start_scan (pedantic)
      , line_count_(line_count)
    {
      traits::assign (blk_, sizeof (blk_) / sizeof (*blk_), 0);
    }

    start_standard_scan::~start_standard_scan (void)
    {
      cancel ();
      operator++ ();
    }

    void
    start_standard_scan::operator>> (connexion& cnx)
    {
      set_line_count ESC_d;
      cnx << ESC_d (line_count_);

      traits::assign (blk_, sizeof (blk_) / sizeof (*blk_), 0);

      cnx_ = &cnx;
      cnx_->send (cmd_, sizeof (cmd_) / sizeof (*cmd_));
    }

    chunk
    start_standard_scan::operator++ (void)
    {
      if (!more_chunks_()) return chunk ();

      cnx_->recv (blk_, 0 == line_count_ ? 4 : 6);

      this->validate_info_block ();

      if (detected_fatal_error ()
          || !is_ready ())      // MUST NOT request image data
        {
          traits::assign (blk_ + 2, sizeof (blk_) / sizeof (*blk_) - 2, 0);
        }

      chunk img;

      if (0 < size_())
        {
          img = chunk (size_());

          cnx_->recv (img, img.size ());

          if (more_chunks_())
            {
              if (!do_cancel_)
                {
                  byte rep = ACK;
                  cnx_->send (&rep, 1);
                }
              else
                {
                  cancelled_ = true;

                  abort_scan can;
                  *cnx_ << can;
                }
            }
        }
      return img;
    }

    bool
    start_standard_scan::detected_fatal_error (void) const
    {
      return 0x80 & blk_[1];
    }

    bool
    start_standard_scan::is_ready (void) const
    {
      return !(0x40 & blk_[1]);
    }

    bool
    start_standard_scan::is_at_area_end (void) const
    {
      return 0x20 & blk_[1];
    }

    color_value
    start_standard_scan::color_attributes (const color_mode_value& mode) const
    {
      if ((0 != line_count_
           && (   LINE_GRB == mode
               || LINE_RGB == mode))
          || (   PIXEL_GRB == mode
              || PIXEL_RGB == mode))
        {
          if (0x04 == this->blk_[1])
            return GRB;
          if (0x08 == this->blk_[1])
            return RGB;
        }
      else
        {
          if (0x00 == this->blk_[1])
            return MONO;
          if (0x04 == this->blk_[1])
            return GREEN;
          if (0x08 == this->blk_[1])
            return RED;
          if (0x0c == this->blk_[1])
            return BLUE;
        }
      BOOST_THROW_EXCEPTION (range_error ("undocumented color attributes"));
    }

    void
    start_standard_scan::cancel (bool at_area_end)
    {
      do_cancel_ = true;
    }

    streamsize
    start_standard_scan::size_(void) const
    {
      uint16_t byte_count = to_uint16_t (blk_ + 2);
      uint16_t line_count = (0 == line_count_
                             ? 1
                             : to_uint16_t (blk_ + 4));

      return byte_count * line_count;
    }

    bool
    start_standard_scan::more_chunks_(void) const
    {
      return !(   is_at_area_end ()
               || cancelled_);
    }

    void
    start_standard_scan::validate_info_block (void) const
    {
      if (STX != this->blk_[0])
        BOOST_THROW_EXCEPTION (unknown_reply ());

      if (this->pedantic_)
        {
          check_reserved_bits (this->blk_, 1, 0x01, "info");
        }
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
