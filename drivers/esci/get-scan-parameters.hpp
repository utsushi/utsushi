//  get-scan-parameters.hpp -- settings for the next scan
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2008, 2013  Olaf Meeuwissen
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

#ifndef drivers_esci_get_scan_parameters_hpp_
#define drivers_esci_get_scan_parameters_hpp_

#include "getter.hpp"
#include "scan-parameters.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Getting the conditions under which to scan.
    /*!  \copydetails get_command_parameters

         \todo  Check individual settings against documented values in
                check_blk_reply()?
     */
    class get_scan_parameters : public getter<FS,UPPER_S,64>
                              , public scan_parameters
    {
    public:
      //!  \copybrief getter::getter
      /*!  \copydetails getter::getter
       */
      get_scan_parameters (bool pedantic = false);

      using scan_parameters::getter_API;

    protected:
      void check_blk_reply (void) const;

      friend class set_scan_parameters;
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_get_scan_parameters_hpp_ */
