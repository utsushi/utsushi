//  set-scan-parameters.hpp -- for the next scan
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

#ifndef drivers_esci_set_scan_parameters_hpp_
#define drivers_esci_set_scan_parameters_hpp_

#include "get-scan-parameters.hpp"
#include "scan-parameters.hpp"
#include "setter.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Setting the conditions under which to scan.
    /*!  Sending command after command quickly becomes a bore, so this
         command lets you set everything in one fell swoop.  The fine
         print for each of the %setter members normally follows that
         of the corresponding %setter command.  Where different, this
         has been noted in the member function documentation.

         \note  If the parameters were accepted, the zoom percentage
                has been reset to the default (100%).

         \note  After setting the scan parameters with this command the
                scan has to be initiated with start_extended_scan.

         \sa get_scan_parameters
     */
    class set_scan_parameters : public setter<FS,UPPER_W,64>
                              , public scan_parameters
    {
    public:
      set_scan_parameters ();
      set_scan_parameters (const set_scan_parameters& s);

      set_scan_parameters&
      operator= (const set_scan_parameters& s);
      set_scan_parameters&
      operator= (const get_scan_parameters& s);

      using scan_parameters::getter_API;

      //!  Sets the scan resolutions for both directions.
      /*!  All values between get_extended_identity::min_resolution()
           and get_extended_identity::max_resolution() are believed to
           be supported, although performance might be sub-optimal for
           resolutions not reported by the get_identity command (and
           the get_hardware_property command).

           \sa set_resolution
       */
      set_scan_parameters& resolution (uint32_t r_x, uint32_t r_y);

      //!  Sets a pair of resolutions.
      set_scan_parameters&
      resolution (point<uint32_t> r)
      {
        return this->resolution (r.x (), r.y ());
      }

      //!  Sets identical resolutions for both directions.
      set_scan_parameters&
      resolution (uint32_t r)
      {
        return this->resolution (r, r);
      }

      //!  Sets the image area to scan.
      /*!  The scan area width in pixels may not exceed the return
           value of get_extended_identity::max_scan_width().

           When scanning at bit depths in the [1,4] range, the scan
           area's width must be a multiple of eight.

           For devices that do page end detection, the requested scan
           area height is only an upper limit.  The device will stop
           producing image data when it detects the end of the page.
           To find out how many scan lines were obtained, one should
           get_scan_parameters::scan_area() after completion of the
           scan.  In case the page end was detected before the scan
           started, due to a large vertical offset for example, the
           device will not obtain any image data.

           \sa set_scan_area, get_extended_identity::detects_page_end()
       */
      set_scan_parameters& scan_area (bounding_box<uint32_t> a);

      //!  Sets an area in terms of \a top_left and \a bottom_right corners
      set_scan_parameters&
      scan_area (point<uint32_t> top_left, point<uint32_t> bottom_right)
      {
        return this->scan_area
          (bounding_box<uint32_t> (top_left, bottom_right));
      }

      //!  Sets scan color and sequence modes.
      /*!  \copydetails set_color_mode
           \sa set_color_mode
       */
      set_scan_parameters& color_mode (byte mode);

      //!  Sets the number of scan lines per block.
      /*!  This setting can be used to control the size of the image
           data chunks returned by start_standard_scan::operator++().

           The default value is \c 0x00 which results in one line per
           block.  The other values request blocks with the number of
           lines equal to the parameter's numeric value.

           \note  The last block may consist of less lines than set.
           \note  When scanning in line sequence mode (see \c LINE_*
                  \ref color_mode_value), the line count value should
                  be a multiple of three.
           \note  Unlike the start_standard_scan command, sending a
                  start_extended_scan command does \e not clear the
                  current value

           \sa set_line_count
       */
      set_scan_parameters& line_count (uint8_t value);

      //!  Controls the number of shades of the color components.
      /*!  \copydetails set_bit_depth
           \sa set_bit_depth
       */
      set_scan_parameters& bit_depth (uint8_t value);

      //!  Trades quality for speed and vice versa.
      /*!  \copydetails set_scan_mode
           \sa set_scan_mode
       */
      set_scan_parameters& scan_mode (byte mode);

      //!  Changes the active option unit and its mode of behaviour.
      /*!  \copydetails set_option_unit
           \sa set_option_unit
       */
      set_scan_parameters& option_unit (byte mode);

      //!  Sets the film type about to be scanned.
      /*!  \copydetails set_film_type
           \sa option_unit(), set_option_unit
       */
      set_scan_parameters& film_type (byte type);

      //!  Flips the horizontal orientation of the pixels.
      /*!  \copydetails set_mirroring
           \sa set_mirroring
       */
      set_scan_parameters& mirroring (bool active);

      //!  Toggles auto area segmentation activation
      /*!  \copydetails set_auto_area_segmentation
           \sa set_auto_area_segmentation
       */
      set_scan_parameters& auto_area_segmentation (bool active);

      //!  Decides the border between black and white.
      /*!  \copydetails set_threshold
           \sa set_threshold
       */
      set_scan_parameters& threshold (uint8_t value);

      //!  Sets a halftone mode or dither pattern.
      /*!  \copydetails set_halftone_processing
           \sa set_halftone_processing
       */
      set_scan_parameters& halftone_processing (byte mode);

      //!  Controls sharpness of edges in an image.
      /*!  \copydetails set_sharpness
           \sa set_sharpness
       */
      set_scan_parameters& sharpness (int8_t value);

      //!  Adjusts the brightness.
      /*!  \copydetails set_brightness
           \sa set_brightness
       */
      set_scan_parameters& brightness (int8_t value);

      //!  Sets a gamma table.
      /*!  \copydetails set_gamma_correction
           \sa set_gamma_correction
       */
      set_scan_parameters& gamma_correction (byte mode);

      //!  Sets a color matrix.
      /*!  \copydetails set_color_correction.
           \sa set_color_correction
       */
      set_scan_parameters& color_correction (byte mode);

      //!  Sets the lighting mode for the flatbed's lamp.
      /*!  The only documented values are 0x00 through 0x02 but their
           respective meanings are unknown.
           All other values are reserved.

           The default value is 0x00.  Other values may be set when
           get_extended_identity::supports_lamp_change() returns \c
           true.

           There is no command to set this property independently.
       */
      set_scan_parameters& main_lamp_lighting_mode (byte mode);

      //!  Controls the sensitivity of the double feed detector
      /*!  Apart from turning this functionality completely off, it
           can be set to one of ::SENSITIVITY_LO or ::SENSITIVITY_HI.

           \sa get_extended_identity::adf_detects_double_feed(),
               scan_parameters::double_feed_sensitivity(),
               sensitivity_value
       */
      set_scan_parameters& double_feed_sensitivity (byte mode);

      //!  Controls the quiet scan mode setting.
      /*!  \sa quiet_mode_value
       */
      set_scan_parameters& quiet_mode (byte mode);
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_set_scan_parameters_hpp_ */
