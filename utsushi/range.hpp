//  range.hpp -- allowed values between lower and upper bounds
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

#ifndef utsushi_range_hpp_
#define utsushi_range_hpp_

#include "constraint.hpp"

namespace utsushi {

//! Only allow values between lower and upper bounds.
/*! Settings such as a scan area or a resolution are very naturally
 *  expressed in terms of values within an interval.  A %range type
 *  %constraint lets any setting impose such a limitation with ease.
 *
 *  \note
 *  A user interface is free to represent settings with a %range type
 *  constraint with spin boxes, sliders, scrollbars, scales and other
 *  similar UI elements.  It is recommended, however, to provide the
 *  user with a means to set values very precisely.
 *
 *  \todo  Specify default value behaviour when modifying limits
 *  \todo  Support "default value" and "clamping" policies
 */
class range : public constraint
{
public:
  typedef shared_ptr< range > ptr;

  range ();

  virtual ~range ();
  virtual const value& operator() (const value& v) const;

  virtual bool is_singular () const;

  virtual void operator>> (std::ostream& os) const;

  //! Sets the %range's \a lower and \a upper limits
  range * bounds (const quantity& lower, const quantity& upper);
  //! Sets the %range's lower limit
  range * offset (const quantity& q);
  //! Sets the %range's upper limit relative to its lower limit
  range * extent (const quantity& q);
  //! Modifies the %range's lower limit
  range * lower (const quantity& q);
  //! Modifies the %range's upper limit
  range * upper (const quantity& q);

  //! Returns the lower limit of the %range
  quantity offset () const;
  //! Returns the difference between upper and lower limits
  quantity extent () const;
  //! Returns the lower limit of the %range
  quantity lower () const;
  //! Returns the upper limit of the %range
  quantity upper () const;
  //! Returns the granularity with which values in the %range can change
  quantity quant () const;

private:
  quantity lower_;
  quantity upper_;
};

}       // namespace utsushi

#endif  /* utsushi_range_hpp_ */
