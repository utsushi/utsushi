//  set-gamma-table.hpp -- tweak pixels to hardware characteristics
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

#ifndef drivers_esci_set_gamma_table_hpp_
#define drivers_esci_set_gamma_table_hpp_

#include <cmath>

#include <algorithm>
#include <stdexcept>

#include <boost/static_assert.hpp>

#include <utsushi/type-traits.hpp>

#include "constant.hpp"
#include "setter.hpp"
#include "vector.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Tweak color intensities to match display hardware.
    /*!  The same pixel values do not always result in the same color
         intensity when displayed on different types of hardware.  A
         gamma table is used to adjust raw pixel data intensity to an
         appropriate intensity for the output hardware of choice.
         This command allows one to set a custom table.

         \note  The table set with this command applies on top of the
                gamma correction selected with set_gamma_correction.

         \note  The initialize command does \e not reset the gamma
                table.

         \warning  When chaining command output as suggested in the
                   operator<<() documentation, please be aware that
                   something like
                   \code
                   cnx << esc_z (RED , 1.2) << esc_z (GREEN, 1.3)
                       << esc_z (BLUE, 1.4);
                   \endcode
                   results in undefined behaviour.  Either send one
                   command at a time, like so
                   \code
                   cnx << esc_z (RED  , 1.2);
                   cnx << esc_z (GREEN, 1.3);
                   cnx << esc_z (BLUE , 1.4);
                   \endcode
                   or use three different instances.

         \sa set_gamma_correction, set_brightness,
             http://en.wikipedia.org/wiki/Gamma_correction
     */
    class set_gamma_table : public setter<ESC,LOWER_Z,257>
    {
    public:

      //!  Sets a linear gamma table for a color \a component.
      /*!  The default sets linear tables for all color components.
       */
      set_gamma_table& operator() (const color_value& component = RGB);

      //!  Sets an encoding \a gamma table for all color components.
      template <typename T>
      set_gamma_table& operator() (const T& gamma)
      {
        BOOST_STATIC_ASSERT (is_floating_point<T>::value);

        return this->operator() (RGB, gamma);
      }

      //!  Sets an encoding \a gamma table for a color \a component.
      /*!  Given a \a gamma value, this operator computes the 256
           values that make up a gamma table.  The computation uses a
           regular power-law on the [0,1] domain to create a table to
           adjust intensities in anticipation of the display device's
           standardized \a gamma.
       */
      template <typename T>
      set_gamma_table& operator() (const color_value& component,
                                   const T& gamma)
      {
        BOOST_STATIC_ASSERT (is_floating_point<T>::value);

        using std::pow;

        vector<T,256> v;

        for (size_t i = 0; i < v.size (); ++i)
          {
            v[i] = pow (T (i) / 255, 1 / gamma);
          }
        return this->operator() (component, v);
      }

      //!  Sets a custom gamma \a table for a color \a component.
      /*!  This operator takes a gamma \a table defined on the [0,1]
           domain and adjusts it to match the protocol requirements.
       */
      template <typename T>
      set_gamma_table& operator() (const color_value& component,
                                   const vector<T,256>& table)
      {
        BOOST_STATIC_ASSERT (is_floating_point<T>::value);

        using std::max;
        using std::min;

        vector<uint8_t,256> v;

        for (size_t i = 0; i < v.size (); ++i)
          {
            v[i] = min (T (255), max (T (0), 255 * table[i] + 0.5));
          }
        return this->operator() (component, v);
      }
    };

    //!  Sets a custom gamma \a table for a color \a component.
    /*!  This template specialization prepares the table in a form
         understood by the device.
     */
    template <>
    set_gamma_table&
    set_gamma_table::operator() (const color_value& component,
                                 const vector<uint8_t,256>& table);

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_set_gamma_table_hpp_ */
