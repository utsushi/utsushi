//  set-scan-area.hpp -- for the next scan
//  Copyright (C) 2012, 2015  SEIKO EPSON CORPORATION
//  Copyright (C) 2008  Olaf Meeuwissen
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

#ifndef drivers_esci_set_scan_area_hpp_
#define drivers_esci_set_scan_area_hpp_

#include "bounding-box.hpp"
#include "setter.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Saying what part of an image to scan.
    /*!  Use this command to set the area that should be scanned.  The
         area is specified in terms of pixels, with the origin in the
         top-left corner.  In order to calculate the pixel values, the
         current resolution and zoom settings need to be taken into
         account, like so
         \f[
         n_{pixels} = length \times resolution \times zoom
         \f]
         where \f$zoom\f$ is in percent.

         The default scan area can be computed from the active option
         unit's \e physical scan_area using the formula above.  The
         resulting values define the bottom-right corner.  The origin
         serves as the top-left corner.

         Note that the width of the scan area needs to be a multiple
         of eight pixels.  There are no implicit restrictions on the
         height beyond those imposed by the maximum scan_area.

         \note  Changing the resolution or zoom resets the scan area to
                its default value for the active option unit.
                Selection of a different option unit does so too.

         \note  When scanning in color pixel sequence mode, the maximum
                pixel value that can be set is 21840 for bit depths up
                to 8 and 10920 for bit depths between 9 and 16.  This
                stems from a protocol implied restriction on the data
                block sizes for the start_standard_scan command.  There
                can be at most 65535 bytes of data in a single scan line.

         \sa set_resolution, set_zoom, set_option_unit
     */
    class set_scan_area : public setter<ESC,UPPER_A,8>
    {
    public:
      //!  Sets an area based on \a scan_area's attributes.
      set_scan_area& operator() (bounding_box<uint16_t> scan_area);

      //!  Sets an area in terms of \a top_left and \a bottom_right corners
      set_scan_area&
      operator() (point<uint16_t> top_left, point<uint16_t> bottom_right)
      {
        return this->operator()
          (bounding_box<uint16_t> (top_left, bottom_right));
      }
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_set_scan_area_hpp_ */
