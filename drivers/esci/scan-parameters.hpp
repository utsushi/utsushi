//  scan-parameters.hpp -- settings for the next scan
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

#ifndef drivers_esci_scan_parameters_hpp_
#define drivers_esci_scan_parameters_hpp_

#include <boost/operators.hpp>

#include "bounding-box.hpp"
#include "code-point.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Conditions under which to scan.
    /*!  Both the get_scan_parameters and set_scan_parameters classes
     *   provide an \e identical %getter API.  This class implements
     *   that API in such a way that those classes can use non-public
     *   inheritance to expose their %getter API entries.
     *
     *   The main purpose of this class is to prevent duplication of
     *   (boring) code and discrepancies between both %getter APIs.
     */
    class scan_parameters
      : boost::equality_comparable< scan_parameters >
    {
    public:
      bool operator== (const scan_parameters& rhs) const;

    protected:
      scan_parameters (const byte mem[64]);

      //!  \copybrief get_command_parameters::resolution
      /*!  \sa set_scan_parameters::resolution()
       */
      point<uint32_t> resolution (void) const;
      //!  \copybrief get_command_parameters::scan_area
      /*!  \sa set_scan_parameters::scan_area()
       */
      bounding_box<uint32_t> scan_area (void) const;

      //!  \copybrief get_command_parameters::color_mode
      /*!  \sa set_scan_parameters::color_mode()
       */
      byte color_mode (void) const;
      //!  \copybrief get_command_parameters::line_count
      /*!  \sa set_scan_parameters::line_count()
       */
      uint8_t line_count (void) const;
      //!  \copybrief get_command_parameters::bit_depth
      /*!  \sa set_scan_parameters::bit_depth()
       */
      uint8_t bit_depth (void) const;

      //!  \copybrief get_command_parameters::scan_mode
      /*!  \sa set_scan_parameters::scan_mode()
       */
      byte scan_mode (void) const;
      //!  \copybrief get_command_parameters::option_unit
      /*!  \sa set_scan_parameters::option_unit()
       */
      byte option_unit (void) const;
      //!  \copybrief get_command_parameters::film_type
      /*!  \sa set_scan_parameters::film_type()
       */
      byte film_type (void) const;

      //!  \copybrief get_command_parameters::mirroring
      /*!  \sa set_scan_parameters::mirroring()
       */
      bool mirroring (void) const;
      //!  \copybrief get_command_parameters::auto_area_segmentation
      /*!  \sa set_scan_parameters::auto_area_segmentation()
       */
      bool auto_area_segmentation (void) const;
      //!  \copybrief get_command_parameters::threshold
      /*!  \sa set_scan_parameters::threshold()
       */
      uint8_t threshold (void) const;
      //!  \copybrief get_command_parameters::halftone_processing
      /*!  \sa set_scan_parameters::halftone_processing()
       */
      byte halftone_processing (void) const;

      //!  \copybrief get_command_parameters::sharpness
      /*!  \sa set_scan_parameters::sharpness()
       */
      int8_t sharpness (void) const;
      //!  \copybrief get_command_parameters::brightness
      /*!  \sa set_scan_parameters::brightness()
       */
      int8_t brightness (void) const;
      //!  \copybrief get_command_parameters::gamma_correction
      /*!  \sa set_scan_parameters::gamma_correction()
       */
      byte gamma_correction (void) const;
      //!  \copybrief get_command_parameters::color_correction
      /*!  \sa set_scan_parameters::color_correction()
       */
      byte color_correction (void) const;

      //!  Yields the current lighting mode for the flatbed's lamp.
      /*!  \sa set_scan_parameters::main_lamp_lighting_mode()
       */
      byte main_lamp_lighting_mode (void) const;

      byte double_feed_sensitivity (void) const;

      //!  Yields the current quiet scan mode setting.
      /*!  \sa set_scan_parameters::quiet_mode()
       */
      byte quiet_mode (void) const;

    private:
      const byte *const mem_;   //!<  owner's parameter memory area
    };

/*  Provide the get_scan_parameters and set_scan_parameters classes
 *  with a convenient mechanism to make the relevant pieces of the
 *  parameters class API public in a consistent manner.  It is meant
 *  to be usable as
 *
 *    using scan_parameters::getter_API;
 *
 *  hence the slightly odd start of the define.  Additional entries
 *  can be inserted anywhere in the list.  Order is not important.
 */
#define getter_API resolution;                       \
  using scan_parameters::scan_area;                  \
  using scan_parameters::color_mode;                 \
  using scan_parameters::line_count;                 \
  using scan_parameters::bit_depth;                  \
  using scan_parameters::scan_mode;                  \
  using scan_parameters::option_unit;                \
  using scan_parameters::film_type;                  \
  using scan_parameters::mirroring;                  \
  using scan_parameters::auto_area_segmentation;     \
  using scan_parameters::threshold;                  \
  using scan_parameters::halftone_processing;        \
  using scan_parameters::sharpness;                  \
  using scan_parameters::brightness;                 \
  using scan_parameters::gamma_correction;           \
  using scan_parameters::color_correction;           \
  using scan_parameters::main_lamp_lighting_mode;    \
  using scan_parameters::double_feed_sensitivity;    \
  using scan_parameters::quiet_mode;                 \
  /**/

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_scan_parameters_hpp_ */
