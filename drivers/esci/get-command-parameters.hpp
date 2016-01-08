//  get-command-parameters.hpp -- settings for the next scan
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

#ifndef drivers_esci_get_command_parameters_hpp_
#define drivers_esci_get_command_parameters_hpp_

#include "bounding-box.hpp"
#include "getter.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Getting the conditions under which to scan.
    /*!  Just because setting all parameters went without a hitch,
         that does not guarantee that the device will actually use
         whatever you told it to.  Use this command to get current
         parameter settings from the device.

         \todo  Check individual settings against documented values
                in a check_dat_reply() override?
     */
    class get_command_parameters : public buf_getter<ESC,UPPER_S>
    {
    public:
      //!  \copybrief buf_getter::buf_getter
      /*!  \copydetails buf_getter::buf_getter
       */
      get_command_parameters (bool pedantic = false);

      //!  Yields the current main and sub resolution settings.
      point<uint32_t> resolution (void) const;
      //!  Yields the current zoom percentages for both scan directions.
      point<uint8_t> zoom (void) const;
      //!  Yields the current scan area settings.
      bounding_box<uint32_t> scan_area (void) const;

      //!  Yields the current color_mode_value.
      byte color_mode (void) const;
      //!  Yields the current line count value.
      /*!  \sa set_line_count
       */
      uint8_t line_count (void) const;
      //!  Yields the current bit depth value.
      /*!  \sa set_bit_depth
       */
      uint8_t bit_depth (void) const;

      //!  Yields the current scan_mode_value.
      byte scan_mode (void) const;
      //!  Yields the current option_value.
      byte option_unit (void) const;
      //!  Yields the current film_type_value.
      byte film_type (void) const;

      //!  Indicates whether image data will be flipped horizontally.
      bool mirroring (void) const;
      //!  Indicates whether auto area segmentation is activated.
      bool auto_area_segmentation (void) const;
      //!  Yields the current threshold value.
      /*!  \sa set_threshold
       */
      uint8_t threshold (void) const;
      //!  Yields the current halftone_dither_value.
      byte halftone_processing (void) const;

      //!  Yields the current sharpness_value.
      int8_t sharpness (void) const;
      //!  Yields the current brightness_value.
      int8_t brightness (void) const;
      //!  Yields the current gamma_table_value.
      byte gamma_correction (void) const;
      //!  Yields the current color_matrix_value.
      byte color_correction (void) const;
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_get_command_parameters_hpp_ */
