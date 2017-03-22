//  extended-tweaks.hpp -- address model specific issues
//  Copyright (C) 2016  SEIKO EPSON CORPORATION
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

#ifndef drivers_esci_extended_tweaks_hpp_
#define drivers_esci_extended_tweaks_hpp_

/*! \file
 *  This file contains extended_scanner subclasses that add some model
 *  specific tweaks and cater to model specific idiosyncracies.
 *
 *  \sa libdrv_esci_LTX_scanner_factory()
 */

#include "extended-scanner.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

#define DERIVED_EXTENDED_(base,derived,api)     \
  class derived                                 \
    : public base                               \
  {                                             \
  public:                                       \
    derived (const connexion::ptr& cnx);        \
    api                                         \
  }                                             \
  /**/

DERIVED_EXTENDED_(extended_scanner, GT_S650,
                  void configure ();
);

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_extended_tweaks_hpp_ */
