//  compound-tweaks.hpp -- address model specific issues
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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

#ifndef drivers_esci_compound_tweaks_hpp_
#define drivers_esci_compound_tweaks_hpp_

/*! \file
 *  This file contains compound_scanner subclasses that add some model
 *  specific tweaks and cater to model specific idiosyncracies.
 *
 *  \sa libdrv_esci_LTX_scanner_factory()
 */

#include "compound-scanner.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

#define DERIVED_COMPOUND_(base,derived,api)     \
  class derived                                 \
    : public base                               \
  {                                             \
  public:                                       \
    derived (const connexion::ptr& cnx);        \
    api                                         \
  }                                             \
  /**/

DERIVED_COMPOUND_(compound_scanner, DS_40,
                  void configure ();
);
DERIVED_COMPOUND_(compound_scanner, DS_5x0,
                  void configure ();
);
DERIVED_COMPOUND_(compound_scanner, DS_760_860,);
DERIVED_COMPOUND_(compound_scanner, DS_xxx00,);
DERIVED_COMPOUND_(DS_xxx00, DS_x500,);
DERIVED_COMPOUND_(DS_xxx00, DS_x0000,);
DERIVED_COMPOUND_(compound_scanner, PX_M7050,);

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_compound_tweaks_hpp_ */
