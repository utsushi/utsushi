//  extended-tweaks.cpp -- address model specific issues
//  Copyright (C) 2016  SEIKO EPSON CORPORATION
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

#include <utsushi/range.hpp>

#include "extended-tweaks.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

GT_S650::GT_S650 (const connexion::ptr& cnx)
  : extended_scanner (cnx)
{
  if (HAVE_MAGICK)              /* enable resampling */
    {
      quantity q (boost::numeric_cast< quantity::integer_type > (defs_.resolution ().x ()));
      constraint::ptr res (from< range > ()
                           -> bounds (50, 4800)
                           -> default_value (q));
      const_cast< constraint::ptr& > (res_) = res;
    }
}

void
GT_S650::configure ()
{
  extended_scanner::configure ();

  descriptors_["enable-resampling"]->active (false);
  descriptors_["enable-resampling"]->read_only (true);
}

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi
