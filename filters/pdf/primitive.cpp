//  primitive.cpp -- PDF primitives
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

#include "primitive.hpp"

namespace utsushi {
namespace _flt_ {
namespace _pdf_ {

primitive::primitive ()
  : _str (string ())
{
}

primitive::~primitive (void)
{
}

void
primitive::operator>> (std::ostream& os) const
{
  os << _str;
}

bool
primitive::operator== (const primitive& that) const
{
  return _str == that._str;
}

// FIXME: doesn't do the default assignment just what we want?
void
primitive::operator= (const primitive& that)
{
  _str = that._str;
}

}       // namespace _pdf_
}       // namespace _flt_
}       // namespace utsushi
