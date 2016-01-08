//  array.cpp -- PDF array objects
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

#include "array.hpp"

namespace utsushi {
namespace _flt_ {
namespace _pdf_ {

array::~array ()
{
  store_citer it;

  for (it = _mine.begin (); _mine.end () != it; ++it)
    {
      object* obj = *it;
      delete obj;
      obj = NULL;
    }
}

void
array::insert (object *value)
{
  _store.push_back (value);
}

void
array::insert (primitive value)
{
  primitive *copy = new primitive ();

  *copy = value;
  _mine.push_back (copy);
  insert (copy);
}

void
array::insert (object value)
{
  object *copy = new object ();

  *copy = value;
  _mine.push_back (copy);
  insert (copy);
}

size_t
array::size () const
{
  return _store.size ();
}

const object *
array::operator[] (size_t index) const
{
  return _store[index];
}

void
array::operator>> (std::ostream& os) const
{
  array::store_citer it;

  os << "[ ";
  if (4 < _store.size ())
    {
      os << "\n";
    }
  for (it = _store.begin (); _store.end () != it; ++it)
    {
      os << **it << " ";
      if (4 < _store.size ())
        {
          os << "\n";
        }
    }
  os << "]";
}

}       // namespace _pdf_
}       // namespace _flt_
}       // namespace utsushi
