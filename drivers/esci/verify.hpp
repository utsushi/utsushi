//  verify.hpp -- ESC/I protocol assumptions and specification compliance
//  Copyright (C) 2012, 2013  EPSON AVASYS CORPORATION
//
//  License: GPL-3.0+
//  Author : Olaf Meeuwissen
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

#ifndef drivers_esci_verify_hpp_
#define drivers_esci_verify_hpp_

#include <boost/optional.hpp>
#include <boost/program_options.hpp>

#include <utsushi/connexion.hpp>

#include "grammar-capabilities.hpp"
#include "grammar-information.hpp"
#include "grammar-parameters.hpp"

//! Program specific global variables
struct verify
{
  static boost::program_options::variables_map vm;

  static utsushi::connexion::ptr cnx;

  static utsushi::_drv_::esci::information  info;
  static utsushi::_drv_::esci::capabilities caps;
  static utsushi::_drv_::esci::parameters   parm;

  static boost::optional< utsushi::_drv_::esci::capabilities > caps_flip;
  static boost::optional< utsushi::_drv_::esci::parameters   > parm_flip;
};

#endif  /* drivers_esci_verify_hpp_ */
