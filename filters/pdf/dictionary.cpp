//  dictionary.cpp -- PDF dictionaries
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

#include "dictionary.hpp"

namespace utsushi {
namespace _flt_ {
namespace _pdf_ {

dictionary::~dictionary ()
{
  store_citer it;

  for (it = _mine.begin (); _mine.end () != it; ++it)
    {
      object* obj = it->second;
      delete obj;
      obj = NULL;
    }
}

void
dictionary::insert (const char *key, object *value)
{
  if (_mine.end () != _mine.find (key))
    {
      delete _mine[key];
    }
  _store[key] = value;
}

void
dictionary::insert (const char *key, primitive value)
{
  primitive *copy = new primitive ();

  *copy = value;
  insert (key, copy);
  _mine[key] = copy;
}

void
dictionary::insert (const char *key, object value)
{
  object *copy = new object ();

  *copy = value;
  insert (key, copy);
  _mine[key] = copy;
}

size_t
dictionary::size () const
{
  return _store.size ();
}

const object *
dictionary::operator[] (const char* key) const
{
  store_citer it = _store.find (key);

  return (_store.end () != it ? it->second : NULL);
}

void
dictionary::operator>> (std::ostream& os) const
{
  dictionary::store_citer it;

  if (1 >= _store.size ())
    {
      it = _store.begin ();
      os << "<< /"
         << it->first
         << " "
         << *it->second
         << " >>";
      return;
    }

  os << "<<\n";
  for (it = _store.begin (); _store.end () != it; ++it)
    {
      os << "/"
         << it->first
         << " "
         << *it->second
         << "\n";
    }
  os << ">>";
}

}       // namespace _pdf_
}       // namespace _flt_
}       // namespace utsushi
