//  get-hardware-property.hpp -- probe additional capabilities
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

#ifndef drivers_esci_get_hardware_property_hpp_
#define drivers_esci_get_hardware_property_hpp_

#include <set>

#include "point.hpp"
#include "getter.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Additional capability query.
    /*!  \c D# level scanners support an additional capability query.
         It provides information about the device's sensor structure
         and two sets of resolutions that should be used instead of
         the one from get_identity::resolutions.

         \sa get_identity, x_resolutions(), y_resolutions()
     */
    class get_hardware_property : public buf_getter<ESC,LOWER_I>
    {
    public:
      //!  \copybrief buf_getter::buf_getter
      /*!  \copydetails buf_getter::buf_getter
       */
      get_hardware_property (bool pedantic = false);

      //!  Yields the device's base resolution.
      /*!  It is not clear whether this is the same as the maximum
           resolution provided by the get_identity command.

           \sa line_spacing()
       */
      uint32_t base_resolution (void) const;

      //!  Indicates whether the device uses a contact image sensor.
      /*!  \sa http://en.wikipedia.org/wiki/Contact_image_sensor
       */
      bool is_cis (void) const;

      //!  Yields the sensor's type.
      /*!  What information this number provides is not clear.  The
           documentation indicates that is is normally equal to one.
       */
      uint8_t sensor_type (void) const;

      //!  Yields the device's color sequence.
      /*!  It is unclear what the return value exactly refers to and
           how this information should be used.  It may indicate the
           ordering of the sensor LEDs.
       */
      color_value color_sequence (void) const;

      //!  Yields the device's line number for a color value of \a c.
      /*!  What information these numbers provide is not clear.  The
           documentation indicates that they are normally one for all
           of ::RED, ::GREEN and ::BLUE.  Other color values are not
           supported.
       */
      uint8_t line_number (const color_value& c) const;

      //!  Yields the device's base line spacings.
      /*!  The color component values that make up a single pixel may
           not be located on the same scan line.  If that is the case,
           the base line spacing, combined with the base and actual
           scan resolution indicates where the color component values
           of a single pixel can be found.  The various values are
           related as follows
           \f[
           s_{actual} = s_{base} \times \frac{r_{actual}}{r_{base}}
           \f]
           where \f$s\f$ indicates line spacing and \f$r\f$ resolution.
           The actual line spacing determines how many scan lines the
           color component values are apart.  That is, if one color
           component value is located on scan line \f$n\f$, another
           color component can be found on line \f$n + s_{actual}\f$.

           The documentation indicates that there are two line spacing
           values but is not clear on how these are to be interpreted.
           One interpretation is that they correspond to the spacings
           between the first and second and the second and third color
           components, respectively.

           SANE's epkowa backend assumes both are equal and only uses
           one to re-align the pixel's color components.  It mentions
           that the line spacings seem to be included in the maximum
           scan area's sub direction.

           \todo  Add a picture clarifying the textual explanation.
           \todo  Fix the return type.  The values do not seem to have
                  any connection to the main and sub directions.

           \sa base_resolution()
       */
      point<uint8_t> line_spacing (void) const;

      //!  Yields the available resolutions in the main direction.
      /*!  \copydetails get_identity::resolutions
       */
      std::set<uint32_t> x_resolutions (void) const;
      //!  Yields the available resolutions in the sub direction.
      /*!  \copydetails get_identity::resolutions
       */
      std::set<uint32_t> y_resolutions (void) const;

    protected:
      //!  Turns reply bytes into a resolution set, starting at \a p.
      std::set<uint32_t> resolutions_(const byte *p) const;

      void check_data_block (void) const;
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_get_hardware_property_hpp_ */
