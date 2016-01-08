//  constraint.cpp -- imposable on utsushi::value objects
//  Copyright (C) 2012, 2013, 2015  SEIKO EPSON CORPORATION
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

#include "utsushi/constraint.hpp"

namespace utsushi {

constraint::constraint ()
{}

constraint::constraint (const value& v)
  : default_(v)
{}

constraint::~constraint ()
{}

const value&
constraint::operator() (const value& v) const
{
  return (default_.type () == v.type ()
          ? v
          : default_);
}

const value&
constraint::default_value () const
{
  return default_;
}

constraint *
constraint::default_value (const value& v)
{
  if (v != operator() (v))
    BOOST_THROW_EXCEPTION
      (violation ("default value violates constraint"));

  default_ = v;
  return this;
}

bool
constraint::is_singular () const
{
  return false;
}

void
constraint::operator>> (std::ostream& os) const
{
  os << default_;
}

constraint::violation::violation (const std::string& arg)
  : std::logic_error (arg)
{}

}       // namespace utsushi
