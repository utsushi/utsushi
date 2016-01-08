//  set-color-matrix.cpp -- tweak pixel component values
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

#include "set-color-matrix.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    set_color_matrix&
    set_color_matrix::operator() ()
    {
      rep_ = 0;

      dat_[0] = 32; dat_[3] =  0; dat_[6] =  0;
      dat_[1] =  0; dat_[4] = 32; dat_[7] =  0;
      dat_[2] =  0; dat_[5] =  0; dat_[8] = 32;

      return *this;
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
