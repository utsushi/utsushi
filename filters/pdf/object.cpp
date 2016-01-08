//  object.cpp -- PDF objects
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

#include <stdexcept>

#include <boost/throw_exception.hpp>

#include <utsushi/i18n.hpp>

#include "object.hpp"

namespace utsushi {
namespace _flt_ {
namespace _pdf_ {

using std::runtime_error;

object::object ()
{
  _obj_num = 0;
}

object::object (size_t num)
{
  // FIXME: what if num has already been used?
  _obj_num = num;
}

object::~object (void)
{
}

size_t
object::obj_num ()
{
  if (65535 == next_obj_num)
    {
      BOOST_THROW_EXCEPTION
        (runtime_error ("PDF object number overflow"));
    }

  if (is_direct ())
    {
      _obj_num = ++next_obj_num;
    }
  return _obj_num;
}

void
object::operator>> (std::ostream& os) const
{
  os << _obj_num << " 0 R";
}

bool
object::operator== (object& that) const
{
  // FIXME: what if one or both instances have not gotten an object
  //        number yet?
  return _obj_num == that._obj_num;
}

bool
object::is_direct () const
{
  return (0 == _obj_num);
}

size_t object::next_obj_num = 0;

void object::reset_object_numbers ()
{
  next_obj_num = 0;
}

std::ostream&
operator<< (std::ostream& os, const object& o)
{
  o >> os;
  return os;
}

}       // namespace _pdf_
}       // namespace _flt_
}       // namespace utsushi
