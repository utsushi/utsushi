//  set-zoom.cpp -- for the next scan
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

#include "set-zoom.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    set_zoom&
    set_zoom::operator() (uint8_t zoom_x, uint8_t zoom_y)
    {
      rep_ = 0;

      dat_[0] = zoom_x;
      dat_[1] = zoom_y;

      return *this;
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
