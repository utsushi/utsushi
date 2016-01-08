//  set-gamma-table.cpp -- tweak pixels to hardware characteristics
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

#include "set-gamma-table.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    set_gamma_table&
    set_gamma_table::operator() (const color_value& component)
    {
      vector<uint8_t,256> table;

      for (size_t i = 0; i < table.size (); ++i)
        {
          table[i] = i;
        }
      return this->operator() (component, table);
    }

    template <>
    set_gamma_table&
    set_gamma_table::operator() (const color_value& component,
                                 const vector<uint8_t,256>& table)
    {
      using std::logic_error;

      /**/ if (RED   == component)
        dat_[0] = UPPER_R;
      else if (GREEN == component)
        dat_[0] = UPPER_G;
      else if (BLUE  == component)
        dat_[0] = UPPER_B;
      else if (RGB   == component)
        dat_[0] = UPPER_M;
      else
        BOOST_THROW_EXCEPTION (logic_error ("unsupported gamma component"));

      rep_ = 0;

      for (size_t i = 0; i < table.size (); ++i)
        {
          dat_[i+1] = table[i];
        }
      return *this;
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
