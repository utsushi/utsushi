//  scanner-inquiry.hpp -- discover device titbits
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

#ifndef drivers_esci_scanner_inquiry_hpp_
#define drivers_esci_scanner_inquiry_hpp_

#include "compound.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

//! Discover device titbits
/*! The scanner_inquiry command, \c FS_Y, is a limited, non-locking
 *  variant of the scanner_control command.  It allows one to obtain
 *  basic device information(), discover capabilities(), look at the
 *  latest settings() and fetch current status().
 *
 *  The complete implementation is provided by the base class.  This
 *  class only exposes the API that is supported for this command.
 */
class scanner_inquiry
  : public compound< FS, UPPER_Y >
{
public:
  scanner_inquiry (bool pedantic = false)
    : base_type_(pedantic)
  {}

  virtual ~scanner_inquiry () {}

  using base_type_::finish;
  using base_type_::get;
  using base_type_::get_information;
  using base_type_::get_capabilities;
  using base_type_::get_parameters;
  using base_type_::get_status;
  using base_type_::extension;
};

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_scanner_inquiry_hpp_ */
