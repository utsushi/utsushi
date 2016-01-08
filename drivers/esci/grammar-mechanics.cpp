//  grammar-mechanics.cpp -- component instantiations
//  Copyright (C) 2012, 2014, 2015  SEIKO EPSON CORPORATION
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

#ifndef USE_SINGLE_FILE_GRAMMAR_INSTANTIATION

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//! \copydoc grammar.cpp

#include "grammar-mechanics.ipp"

namespace utsushi {
namespace _drv_ {
namespace esci {

namespace fcs = code_token::mechanic::fcs;

hardware_request::hardware_request ()
  : ini (false)
{}

bool
hardware_request::operator== (const hardware_request& rhs) const
{
  return (   adf == rhs.adf
          && fcs == rhs.fcs
          && ini == rhs.ini);
}

void
hardware_request::clear ()
{
  *this = hardware_request ();
}

hardware_request::focus::focus ()
  : type (fcs::AUTO)
{}

hardware_request::focus::focus (const integer& pos)
  : type (fcs::MANU)
  , position (pos)
{}

bool
hardware_request::focus::operator== (const hardware_request::focus& rhs) const
{
  return (   type     == rhs.type
          && position == rhs.position);
}

template class
encoding::basic_grammar_mechanics< encoding::default_iterator_type >;

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif /* !defined (USE_SINGLE_FILE_GRAMMAR_INSTANTIATION) */
