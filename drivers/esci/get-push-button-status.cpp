//  get-push-button-status.cpp -- to check for events
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

#include "get-push-button-status.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    get_push_button_status::get_push_button_status (bool pedantic)
      : buf_getter<ESC,EXCLAM> (pedantic)
    {}

    byte
    get_push_button_status::size_request (void) const
    {
      return dat_[0] >> 5;
    }

    bool
    get_push_button_status::is_duplexing (void) const
    {
      return 0x10 & dat_[0];
    }

    byte
    get_push_button_status::status (void) const
    {
      return 0x03 & dat_[0];
    }

    void
    get_push_button_status::check_blk_reply (void) const
    {
      check_reserved_bits (dat_, 0, 0x0c);
      if (0xe0 == (0xe0 & dat_[0]))     // reserved document size
        {
          log::brief ("%1$s: %2$s[%3$2u] = %4$02x")
            % name ()
            % "data"
            % 0
            % 0xe0;
        }
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
