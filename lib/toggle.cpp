//  toggle.cpp -- bounded type for utsushi::value objects
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

#include "utsushi/toggle.hpp"

namespace utsushi {

toggle::toggle (const bool& toggle)
  : toggle_(toggle)
{}

toggle::toggle ()
  : toggle_(false)
{}

bool
toggle::operator== (const toggle& t) const
{
  return toggle_ == t.toggle_;
}

toggle::operator bool () const
{
  return toggle_;
}

std::ostream&
operator<< (std::ostream& os, const toggle& t)
{
  os << t.toggle_;
  return os;
}

std::istream&
operator>> (std::istream& is, toggle& t)
{
  is >> t.toggle_;
  return is;
}

}       // namespace utsushi
