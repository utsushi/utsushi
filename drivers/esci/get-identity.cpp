//  get-identity.cpp -- probe for basic capabilities
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

#include "get-identity.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    get_identity::get_identity (bool pedantic)
      : buf_getter<ESC,UPPER_I> (pedantic)
    {}

    std::string
    get_identity::command_level (void) const
    {
      return to_string (dat_, 2);
    }

    std::set<uint32_t>
    get_identity::resolutions (void) const
    {
      std::set<uint32_t> result;

      const byte *p = dat_ + 2;

      while (p < dat_ + size () - 5)
        {
          result.insert (to_uint16_t (p + 1));
          p += 3;
        }
      return result;
    }

    bounding_box<uint32_t>
    get_identity::scan_area (void) const
    {
      const byte *p = dat_ + size () - 4;

      return point<uint32_t> (to_uint16_t (p), to_uint16_t (p + 2));
    }

  } // namespace esci
} // namespace _drv_
} // namespace utsushi
