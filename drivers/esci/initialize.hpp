//  initialize.hpp -- (or reset) scanner settings
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2009  Olaf Meeuwissen
//
//  License: GPL-3.0+
//  Author : EPSON AVASYS CORPORATION
//  Author : Olaf Meeuwissen
//  Origin : FreeRISCI
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

#ifndef drivers_esci_initialize_hpp_
#define drivers_esci_initialize_hpp_

#include "action.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Setting up a well-defined device state.
    /*!  This command returns the device (on the other end of the
         connexion) to a well-defined, but model specific!, state.
         The command resets all scan parameters to their default
         value.  Exceptions to this rule are the gamma tables, color
         matrices and dither patterns, but their use is disabled by
         default.  That is, while custom gamma tables, color matrices
         and dither patterns remain loaded, they will no longer be
         applied after running this command.

         This command does not clear the media values last detected
         in the get_scanner_status and get_extended_status commands.
         It also does not reset the focus position.

         \note
         Despite the extremely basic nature of the command not all
         ESC/I command levels include support for it.

         \sa get_command_parameters, get_scan_parameters,
             get_focus_position
     */
    class initialize : public action<ESC,AT_MARK,2>
    {
    protected:
      virtual void validate_reply (void) const;
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_initialize_hpp_ */
