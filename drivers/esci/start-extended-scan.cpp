//  start-extended-scan.cpp -- to acquire image data
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2009, 2013  Olaf Meeuwissen
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

#include <boost/throw_exception.hpp>

#include "action.hpp"
#include "start-extended-scan.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    const byte start_extended_scan::cmd_[2] = { FS, UPPER_G };

    start_extended_scan::start_extended_scan (bool pedantic)
      : start_scan (pedantic)
      , do_at_end_(false)
      , error_code_mask_(~reserved_error_code_bits_)
      , error_code_(0), chunk_count_(0), final_bytes_(0)
    {
      traits::assign (blk_, sizeof (blk_) / sizeof (*blk_), 0);
    }

    start_extended_scan::start_extended_scan (byte error_code_mask,
                                              bool pedantic)
      : start_scan (pedantic)
      , do_at_end_(false)
      , error_code_mask_(error_code_mask)
      , error_code_(0), chunk_count_(0), final_bytes_(0)
    {
      traits::assign (blk_, sizeof (blk_) / sizeof (*blk_), 0);
    }

    start_extended_scan::~start_extended_scan (void)
    {
      cancel ();
      operator++ ();
    }

    void
    start_extended_scan::operator>> (connexion& cnx)
    {
      cancelled_ = false;
      do_cancel_ = false;
      do_at_end_ = false;

      cnx_ = &cnx;
      cnx_->send (cmd_, sizeof (cmd_) / sizeof (*cmd_));
      cnx_->recv (blk_, sizeof (blk_) / sizeof (*blk_));

      this->validate_info_block ();

      if (detected_fatal_error ()
          || !is_ready ())      // MUST NOT request image data
        {
          traits::assign (blk_ + 2, sizeof (blk_) / sizeof (*blk_) - 2, 0);
        }

      chunk_count_ = to_uint32_t (blk_ +  6);
      final_bytes_ = to_uint32_t (blk_ + 10);

      setup_chunk_(size_(), true);
    }

    chunk
    start_extended_scan::operator++ (void)
    {
      if (!more_chunks_() || cancelled_) return chunk ();

      chunk img;

      if (size_())
        {
          img = fetch_chunk_(size_(), true);

          cnx_->recv (img, img.size (true));
          error_code_ = img.error_code ();
          this->scrub_error_code_();

          if (0 < chunk_count_)
            --chunk_count_;
          else
            final_bytes_ = 0;

          if (detected_fatal_error ()
              || !is_ready ())  // MUST NOT request image data
            {
              chunk_count_ = 0;
              final_bytes_ = 0;
            }

          if (more_chunks_())
            {
              if (is_cancel_requested ())
                cancel ();

              if (!do_cancel_)
                {
                  byte rep = ACK;
                  cnx_->send (&rep, 1);
                }
              else
                {
                  cancelled_ = true;

                  if (is_at_page_end ()
                      && do_at_end_)
                    {
                      end_of_transmission eot;
                      *cnx_ << eot;
                    }
                  else
                    {
                      abort_scan can;
                      *cnx_ << can;
                    }
                }
            }
        }
      return img;
    }

    bool
    start_extended_scan::detected_fatal_error (void) const
    {
      return (   (0x80 & error_code_)
              || (0x80 & blk_[1]));
    }

    bool
    start_extended_scan::is_ready (void) const
    {
      return !(   (0x40 & error_code_)
               || (0x40 & blk_[1]));
    }

    bool
    start_extended_scan::is_at_page_end (void) const
    {
      return 0x20 & error_code_;
    }

    bool
    start_extended_scan::is_cancel_requested (void) const
    {
      return 0x10 & error_code_;
    }

    void
    start_extended_scan::cancel (bool at_area_end)
    {
      do_cancel_ = true;
      do_at_end_ = at_area_end;
    }

    streamsize
    start_extended_scan::size_(void) const
    {
      return (0 == chunk_count_
              ? final_bytes_
              : to_uint32_t (blk_ + 2));
    }

    bool
    start_extended_scan::more_chunks_(void) const
    {
      return !(   0 == chunk_count_
               && 0 == final_bytes_
               && !cancelled_);
    }

    void
    start_extended_scan::validate_info_block (void) const
    {
      if (STX != this->blk_[0])
        BOOST_THROW_EXCEPTION (unknown_reply ());

      if (this->pedantic_)
        {
          check_reserved_bits (this->blk_, 1, 0x2d, "info");
        }
    }

    void
    start_extended_scan::scrub_error_code_(void)
    {
      if (pedantic_)
        {
          check_reserved_bits (&this->error_code_, 0,
                               reserved_error_code_bits_, "errc");
        }
      error_code_ &= ~reserved_error_code_bits_;

      if (pedantic_ && ~error_code_mask_ & error_code_)
        {
          log::brief ("clearing unsupported error code bits (%1$02x)")
            % (~error_code_mask_ & error_code_)
            ;
        }
      error_code_ &= error_code_mask_;
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
