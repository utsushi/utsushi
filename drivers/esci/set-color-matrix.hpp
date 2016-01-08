//  set-color-matrix.hpp -- tweak pixel component values
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

#ifndef drivers_esci_set_color_matrix_hpp_
#define drivers_esci_set_color_matrix_hpp_

#include <cmath>

#include <algorithm>

#include <boost/static_assert.hpp>

#include <utsushi/type-traits.hpp>

#include "matrix.hpp"
#include "setter.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Tweak pixels to taste.
    /*!  A number of devices support color correction in hardware.
         You can influence the result of this process by setting a
         3x3 matrix.  It changes the red, green and blue component
         values of every pixel scanned to values that are returned
         in the image data as follows:
         \f[
         \left[ \begin{array}{c} R \\ G \\ B \end{array} \right]_{out}
           = \left[ \begin{array}{ccc}
                    a_{rr} & a_{rg} & a_{rb} \\
                    a_{gr} & a_{gg} & a_{gb} \\
                    a_{br} & a_{bg} & a_{bb} \\
                    \end{array}
             \right]
           \left[ \begin{array}{c} R \\ G \\ B \end{array} \right]_{in}
         \f]
         where \f$in\f$ denotes the scanned pixel and \f$out\f$ the
         pixel as returned in the image data.

         Limitations in the ESC/I protocol limit the effective range
         and precision of the coefficients \f$a\f$.  This is handled
         transparently by the command implementation so should be of
         no concern to the user.
         For the record, the values are spaced 1/32 apart, rounded
         towards zero and their magnitude ranges from 0 to 3 31/32
         (= 3.96875).

         \note  The matrix is only used when \ref set_color_correction
                has been set to use a ::USER_DEFINED matrix.

         \note  The initialize command does \e not reset the color
                matrix.  In order to use the default matrix select
                the matrix for a color ::CRT_DISPLAY with the \ref
                set_color_correction command.

         \sa set_color_correction
     */
    class set_color_matrix : public setter<ESC,LOWER_M,9>
    {
    public:
      //!  Sets a unit matrix.
      set_color_matrix& operator() ();

      //!  Sets a custom matrix.
      template <typename T>
      set_color_matrix& operator() (const matrix<T,3>& mat)
      {
        BOOST_STATIC_ASSERT (is_floating_point<T>::value);

        rep_ = 0;

        for (size_t i = 0; i < mat.rows (); ++i)
          for (size_t j = 0; j < mat.cols (); ++j)
            {
              T val = 32 * mat[i][j];

              dat_[i + j * mat.rows ()]  = int8_t (min (T (127), abs (val)));
              dat_[i + j * mat.rows ()] |= (0 < val
                                            ? 0x80
                                            : 0x00);
            }

        return *this;
      }
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_set_color_matrix_hpp_ */
