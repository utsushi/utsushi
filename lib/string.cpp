//  string.cpp -- bounded type for utsushi:value objects
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

#include <cstring>

#include "utsushi/string.hpp"

namespace utsushi {

string::string (const std::string& s)
  : string_(s)
{}

string::string (const char *str)
  : string_(str)
{}

string::string ()
  : string_()
{}

bool
string::operator== (const string& s) const
{
  return string_ == s.string_;
}

bool
string::operator< (const string& s) const
{
  return string_ < s.string_;
}

string&
string::operator= (const string& s)
{
  string_ = s.string_;
  return *this;
}

string::operator bool () const
{
  return !string_.empty ();
}

string::operator std::string () const
{
  return string_;
}

const char *
string::c_str () const
{
  return string_.c_str ();
}

string::size_type
string::copy (char *dst, size_type n, size_type pos) const
{
  return string_.copy (dst, n, pos);
}

string::size_type
string::size () const
{
  return strlen (c_str ());
}

std::ostream&
operator<< (std::ostream& os, const string& s)
{
  os << s.string_;
  return os;
}

std::istream&
operator>> (std::istream& is, string& s)
{
  std::getline (is, s.string_);
  return is;
}

}       // namespace utsushi
