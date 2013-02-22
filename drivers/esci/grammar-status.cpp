//  grammar-status.cpp -- component instantiations
//  Copyright (C) 2012  SEIKO EPSON CORPORATION
//
//  License: GPL-3.0+
//  Author : AVASYS CORPORATION
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

#ifndef USE_SINGLE_FILE_GRAMMAR_INSTANTIATION

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//! \copydoc grammar.cpp

#include "grammar-status.ipp"

namespace utsushi {
namespace _drv_ {
namespace esci {

bool
hardware_status::operator== (const hardware_status& rhs) const
{
  return (   medium      == rhs.medium
          && error       == rhs.error
          && focus       == rhs.focus
          && push_button == rhs.push_button);
}

void
hardware_status::clear ()
{
  *this = hardware_status ();
}

hardware_status::result::result (const quad& part, const quad& what)
  : part_(part)
  , what_(what)
{}

bool
hardware_status::result::operator== (const hardware_status::result& rhs) const
{
  return (   part_ == rhs.part_
          && what_ == rhs.what_);
}

template class
decoding::basic_grammar_status< decoding::default_iterator_type >;

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif /* !defined (USE_SINGLE_FILE_GRAMMAR_INSTANTIATION) */
