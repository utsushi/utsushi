//  range.cpp -- allowed values between lower and upper bounds
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

#include "utsushi/format.hpp"
#include "utsushi/range.hpp"

namespace utsushi {

range::range ()
{
  default_ = quantity ();
}

range::~range ()
{}

const value&
range::operator() (const value& v) const
{
  if (default_.type () != v.type ())
    {
      return default_;
    }

  quantity q = v;

  return ((lower_ <= q
           && q <= upper_)
          ? v
          : default_);
}

bool
range::is_singular () const
{
  return lower_ == upper_;
}

void
range::operator>> (std::ostream& os) const
{
  format fmt ("%1%..%2%");
  os << fmt % lower_ % upper_;
}

range *
range::bounds (const quantity& lo, const quantity& hi)
{
  return lower (lo) -> upper (hi);
}

range *
range::offset (const quantity& q)
{
  lower_ = q;
  return this;
}

range *
range::extent (const quantity& q)
{
  upper_ = lower_ + q;
  return this;
}

range *
range::lower (const quantity& q)
{
  lower_ = q;
  return this;
}

range *
range::upper (const quantity& q)
{
  upper_ = q;
  return this;
}

quantity
range::offset () const
{
  return lower_;
}

quantity
range::extent () const
{
  return upper_ - lower_;
}

quantity
range::lower () const
{
  return lower_;
}

quantity
range::upper () const
{
  return upper_;
}

quantity
range::quant () const
{
  return quantity (0);
}

}       // namespace utsushi
