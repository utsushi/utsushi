//  set-resolution.hpp -- for the next scans
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

#ifndef drivers_esci_set_resolution_hpp_
#define drivers_esci_set_resolution_hpp_

#include "point.hpp"
#include "setter.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Specifying scan resolutions.
    /*!  Use this command to set the resolutions to use for the main
         and sub scan direction the next scan.  At the very least
         resolutions returned by the get_identity command ought to be
         usable.  Whether any combination of main and sub resolutions
         is acceptable or not is not documented.

         Devices with a \c B# command level (where \c # is a single
         digit) are documented to support all resolutions between 50
         pixels per inch and their maximum resolution, as reported by
         the get_identity command.

         Devices with a \c D# command level only support setting of
         values that are reported by the get_hardware_property.  It
         is not clear whether all combinations of \c x/y resolution
         pairs are supported.

         \note  This command sets the scan area to the default value
                for the active option unit.  Hence, it should be sent
                before set_scan_area.

         \note  It is believed that the device performs \e software
                scaling for any of the resolutions not listed by the
                get_identity command.

         \sa set_scan_area, set_zoom, set_option_unit
     */
    class set_resolution : public setter<ESC,UPPER_R,4>
    {
    public:
      //!  Sets independent main and sub resolutions.
      /*!  The resolution for the main direction will be set to \a
           resolution_x and the sub direction to \a resolution_y.
       */
      set_resolution& operator() (uint16_t resolution_x,
                                  uint16_t resolution_y);

      //!  Sets independent main and sub resolutions.
      /*!  The resolution in the main and sub directions will be set
           using the x and y components of \a resolution.
       */
      set_resolution&
      operator() (point<uint16_t> resolution)
      {
        return this->operator() (resolution.x (), resolution.y ());
      }

      //!  Sets identical resolutions for both scan directions.
      /*!  This single argument convenience overload sets identical
           resolutions in the main and sub directions.
       */
      set_resolution&
      operator() (uint16_t resolution)
      {
        return this->operator() (resolution, resolution);
      }
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_set_resolution_hpp_ */
