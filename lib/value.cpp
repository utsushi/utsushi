//  value.cpp -- union-like construct for various kinds of settings
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

#include "utsushi/value.hpp"

namespace utsushi {

value::value ()
  : value_()
{}

value::value (const quantity& q)
  : value_(q)
{}

value::value (const string& s)
  : value_(s)
{}

value::value (const toggle& t)
  : value_(t)
{}

value::value (const quantity::integer_type& q)
  : value_(quantity (q))
{}

value::value (const quantity::non_integer_type& q)
  : value_(quantity (q))
{}

value::value (const std::string& s)
  : value_(s)
{}

value::value (const char *str)
  : value_(string (str))
{}

bool
value::operator== (const value& val) const
{
  return (value_ == val.value_);
}

const std::type_info&
value::type () const
{
  return value_.type ();
}

std::ostream&
operator<< (std::ostream& os, const value& val)
{
  return os << val.value_;
}

bool
value::none::operator== (const value::none&) const
{
  return true;
}

std::ostream&
operator<< (std::ostream& os, const value::none&)
{
  return os;
}

std::istream&
operator>> (std::istream& is, value::none&)
{
  return is;
}

}       // namespace utsushi
