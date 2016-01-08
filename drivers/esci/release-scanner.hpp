//  release-scanner.hpp -- restore general device access
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

#ifndef drivers_esci_release_scanner_hpp_
#define drivers_esci_release_scanner_hpp_

#include "action.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Give others a chance to use the device again.
    /*!  Once done using the device under exclusive access conditions,
         it is common courtesy to say so to the device.  That is done
         with this command.

         If supported, the command always succeeds, regardless of
         whether one has previously gained exclusive access.

         \sa capture_scanner
     */
    class release_scanner : public action<ESC,PAREN_R,2>
    {
    protected:
      virtual void validate_reply (void) const;
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_release_scanner_hpp_ */
