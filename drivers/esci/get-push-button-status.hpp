//  get-push-button-status.hpp -- to check for events
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

#ifndef drivers_esci_get_push_button_status_hpp_
#define drivers_esci_get_push_button_status_hpp_

#include "getter.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Check for button push events.
    /*!  A number of devices have one or more button that can be used
         to scan in different ways.  After a button has been pushed,
         button status can be retrieved through this command.

         Although technically the command has a variable reply size,
         only replies of size one are documented.

         It is not clear how one should go about detecting push events
         as they happen.  This command only seems to support detection
         of the last push event, without any information as to when it
         happened.

         \sa get_extended_status::has_push_button(),
             get_extended_identity::has_push_button()
     */
    class get_push_button_status : public buf_getter<ESC,EXCLAM>
    {
    public:
      //!  \copybrief buf_getter::buf_getter
      /*!  \copydetails buf_getter::buf_getter
       */
      get_push_button_status (bool pedantic = false);

      //!  Yields the status of the most recent push event.
      /*!  A return value of \c 0x00 indicates no buttons were pushed.
           Values up to \c 0x03 are documented but their interpretation
           is not known.
       */
      byte status (void) const;
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_get_push_button_status_hpp_ */
