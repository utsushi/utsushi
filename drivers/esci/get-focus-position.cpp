//  get-focus-position.cpp -- relative to the glass plate
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

#include "get-focus-position.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    get_focus_position::get_focus_position (bool pedantic)
      : buf_getter<ESC,LOWER_Q> (pedantic)
    {}

    uint8_t
    get_focus_position::position (void) const
    {
      return dat_[1];
    }

    bool
    get_focus_position::is_auto_focussed (void) const
    {
      return !(0x01 & dat_[0]);
    }

    void
    get_focus_position::check_data_block (void) const
    {
      check_reserved_bits (dat_, 0, 0xfe);
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
