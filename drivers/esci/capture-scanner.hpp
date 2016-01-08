//  capture-scanner.hpp -- gain exclusive device access
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

#ifndef drivers_esci_capture_scanner_hpp_
#define drivers_esci_capture_scanner_hpp_

#include "action.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Prevent others from using a device.
    /*!  In certain situations you want to prevent other users from
         using a device.  One example is scanning a stack of documents
         from the ADF.  Another, device initiated, example is when one
         is making a copy on an all-in-one type device.  This command
         allows the driver to get exclusive access to the device.  If
         not using the device for a certain amount of time after one
         has gained exclusive access, it is automatically revoked.  A
         more neighbourly way to achieve the same is provided by means
         of the release_scanner command.

         If another user has already obtained exclusive access when
         sending this command, a device_busy exception will result.

         \todo  Document timeout behaviour.

         \sa release_scanner
     */
    class capture_scanner : public action<ESC,PAREN_L,2>
    {
    protected:
      virtual void validate_reply (void) const;
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_capture_scanner_hpp_ */
