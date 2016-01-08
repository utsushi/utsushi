//  value.hpp -- mediate between utsushi::value and SANE API conventions
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

#ifndef sane_value_hpp_
#define sane_value_hpp_

extern "C" {                    // needed until sane-backends-1.0.14
#include <sane/sane.h>
}

#include <utsushi/constraint.hpp>
#include <utsushi/option.hpp>
#include <utsushi/value.hpp>

namespace sane {

//! Mediate between utsushi::value and SANE API conventions
/*! The implementation is aware of utsushi::value internals and uses a
 *  private copy of the value passed at construction time.  This copy
 *  is used to dispatch API calls based on the value's bounded type.
 *  The visitation mechanism is completely transparent to the API user
 *  as it is implemented internally.
 *
 *  \todo  Get correct maximum size of SANE_TYPE_STRING values
 *  \todo  Fix unit support when quantity grows support for them
 */
class value
  : public utsushi::value
{
public:
  value (const utsushi::value& uv);
  value (const utsushi::value& uv, const utsushi::constraint::ptr& cp);
  value (const utsushi::quantity& q, const SANE_Value_Type& type);
  value (const utsushi::option& gv);

  SANE_Int        size () const;
  SANE_Value_Type type () const;
  SANE_Unit       unit () const;

  value& operator*= (const utsushi::quantity& factor);
  value& operator/= (const utsushi::quantity& factor);

  //! Puts a sane::value into SANE frontend managed memory
  const value& operator>> (void *v) const;

  //! Sets a sane::value to what's in SANE frontend managed memory
  value& operator<< (const void *v);

protected:
  static const SANE_Int default_string_size = 512;

private:
  utsushi::constraint::ptr cp_;
};

}       // namespace sane

#endif  /* sane_value_hpp_ */
