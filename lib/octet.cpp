//  octet.cpp -- type and trait definitions
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

#include "utsushi/octet.hpp"

namespace utsushi {

traits::int_type
traits::to_int_type (const char_type& c)
{
  return 0xff & static_cast<int_type> (c);
}

traits::int_type
traits::eof ()
{
  return std::char_traits< octet >::eof ();
}

traits::int_type
traits::eos ()
{
  return eof () - 1;
}

traits::int_type
traits::eoi ()
{
  return eos () - 1;
}

traits::int_type
traits::boi ()
{
  return eoi () - 1;
}

traits::int_type
traits::bos ()
{
  return boi () - 1;
}

traits::int_type
traits::bof ()
{
  return bos () - 1;
}

traits::int_type
traits::not_marker (const int_type& i)
{
  return (is_marker (i)
          ? bof () - 1
          : i);
}

bool
traits::is_marker (const int_type& i)
{
  return (   eof () == i
          || eos () == i
          || eoi () == i
          || bof () == i
          || bos () == i
          || boi () == i);
}

} // namespace utsushi
