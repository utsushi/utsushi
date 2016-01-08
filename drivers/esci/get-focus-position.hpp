//  get-focus-position.hpp -- relative to the glass plate
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

#ifndef drivers_esci_get_focus_position_hpp_
#define drivers_esci_get_focus_position_hpp_

#include "getter.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Find out where the focus is.
    /*!  This command can always be used (with \c B# level scanners),
         irrespective of focussing support.

         \note  The initialize command does not reset the focus.

         \todo  Find out how auto-focus supporting devices behave.  The
                documentation implies that such devices start the focus
                adjustment process when this command is sent.  Then what
                is the use of ::FOCUS_AUTO with set_focus_position?

         \sa timeout_value, set_focus_position
     */
    class get_focus_position : public buf_getter<ESC,LOWER_Q>
    {
    public:
      //!  \copybrief buf_getter::buf_getter
      /*!  \copydetails buf_getter::buf_getter
       */
      get_focus_position (bool pedantic = false);

      //!  Says where the focus is at.
      /*!  The return value is relative to ::FOCUS_GLASS, with values
           less than that below the glass plate.
       */
      uint8_t position (void) const;

      //!  Says whether auto-focussing was successful.
      /*!  This only makes sense of course when auto-focus support is
           available and auto-focus was requested.

           Only a return value of \c false can be interpreted without
           ambiguity.  It means that the device was not able to focus
           automatically.
       */
      bool is_auto_focussed (void) const;

    protected:
      void check_data_block (void) const;
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_get_focus_position_hpp_ */
