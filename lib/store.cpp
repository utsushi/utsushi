//  store.cpp -- collections of allowed values
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

#include <algorithm>

#include "utsushi/store.hpp"

namespace utsushi {

store::~store ()
{}

const value&
store::operator() (const value& v) const
{
  return (store_.end () != std::find (store_.begin (), store_.end (), v)
          ? v
          : default_);
}

constraint *
store::default_value (const value& v)
{
  alternative (v);
  return constraint::default_value (v);
}

bool
store::is_singular () const
{
  return 1 == size ();
}

void
store::operator>> (std::ostream& os) const
{
  if (0 == size ()) return;

  store::const_iterator it (begin ());
  os << *it++;
  while (end () != it)
    os << "|" << *it++;
}

store *
store::alternative (const value& v)
{
  if (store_.end () == std::find (store_.begin (), store_.end (), v))
    {
      store_.push_back (v);
    }
  return this;
}

store::size_type
store::size () const
{
  return store_.size ();
}

store::const_iterator
store::begin () const
{
  return store_.begin ();
}

store::const_iterator
store::end () const
{
  return store_.end ();
}

const value&
store::front () const
{
  return store_.front ();
}

const value&
store::back () const
{
  return store_.back ();
}

}       // namespace utsushi
