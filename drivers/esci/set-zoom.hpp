//  set-zoom.hpp -- for the next scans
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

#ifndef drivers_esci_set_zoom_hpp_
#define drivers_esci_set_zoom_hpp_

#include "point.hpp"
#include "setter.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Zooming in, or out.
    /*!  This command allows one to specify a zoom percentage in the
         main and sub scan directions.  Acceptable values are in the
         50% to 200% range, with a documented default of 100% in both
         directions.

         \note  This command resets the scan area to the default value
                for the active option unit.  Hence, it should be sent
                before set_scan_area.

         \note  It is believed that the device combines the zoom
                percentage and the resolution to arrive at an \e
                effective resolution.

         \sa set_scan_area, set_resolution, set_option_unit
     */
    class set_zoom : public setter<ESC,UPPER_H,2>
    {
    public:
      //!  Sets independent main and sub zoom percentages.
      /*!  The zoom percentage in the main direction will be set to \a
           zoom_x, that in the sub direction to \a zoom_y.
       */
      set_zoom& operator() (uint8_t  zoom_x, uint8_t  zoom_y);

      //!  Sets independent main and sub zoom percentages.
      /*!  The zoom percentages in the main and sub directions will be
           set using the x and y components of \a zoom.
       */
      set_zoom&
      operator() (point<uint8_t> zoom)
      {
        return this->operator() (zoom.x (), zoom.y ());
      }

      //!  Sets identical zoom percentage for both scan directions.
      /*!  This single argument convenience overload sets identical
           \a zoom percentages in the main and sub directions.  The
           default resets to the documented value of 100%.
       */
      set_zoom&
      operator() (uint8_t zoom = 100)
      {
        return this->operator() (zoom, zoom);
      }
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_set_zoom_hpp_ */
