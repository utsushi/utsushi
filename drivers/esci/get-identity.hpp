//  get-identity.hpp -- probe for basic capabilities
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

#ifndef drivers_esci_get_identity_hpp_
#define drivers_esci_get_identity_hpp_

#include <set>

#include "bounding-box.hpp"
#include "getter.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Basic capability query.
    /*!  The protocol's name sake command, get_identity is used to
         retrieve information regarding the device's command_level(),
         a set of nominally supported resolutions() and the device's
         maximum supported scan_area().  This information is encoded
         in a second reply buffer of model dependent size().

         The first two bytes constitute a short string that indicates
         the command_level().

         The final five bytes encode the scan_area().  The first byte
         is always an \c A and the remaining two pairs encode the scan
         area's maximum main and sub dimensions in pixels.  Each pair
         starts with the least significant byte.  The main dimension
         is normally the shorter and aligned with the orientation of
         the scan head.  The sub dimension aligns with the direction
         that the scan head moves in.

         The intermediate \c 3n bytes make up \c n triplets, each of
         which encodes a supported resolution.  Each triplet consists
         of an \c R, followed by the resolution (in pixels per inch)
         made up of a least and most significant byte (in that order).

         \sa get_hardware_property
     */
    class get_identity : public buf_getter<ESC,UPPER_I>
    {
    public:
      //!  \copybrief buf_getter::buf_getter
      /*!  \copydetails buf_getter::buf_getter
       */
      get_identity (bool pedantic = false);

      //!  Yields the device's command level.
      /*!  The set of other supported commands is basically determined
           by this information.
       */
      std::string command_level (void) const;

      //!  Yields the device's available resolutions.
      /*!  All resolutions are in pixels per inch.

           The specifications are not clear on whether the resolutions
           are in any particular order or even unique for that matter.
           Observation indicates that they are unique and sorted from
           low to high.  Also, the specifications do not indicate if
           the command's reply depends on the option selected via the
           \ref set_option_unit command.
       */
      std::set<uint32_t> resolutions (void) const;

      //!  Yields the device's maximum scan area in pixels.
      /*!  The pixel dimensions are to be divided by the largest of
           the scan resolutions() to obtain the physical scan area
           dimensions in inches.
       */
      bounding_box<uint32_t> scan_area (void) const;
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_get_identity_hpp_ */
