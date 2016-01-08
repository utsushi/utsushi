//  set-dither-pattern.hpp -- diffuse banding in B/W images
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

#ifndef drivers_esci_set_dither_pattern_hpp_
#define drivers_esci_set_dither_pattern_hpp_

#include <boost/static_assert.hpp>

#include "command.hpp"
#include "matrix.hpp"

namespace utsushi {
namespace _drv_ {
  namespace esci
  {
    //!  Diffuse banding artifacts in B/W images.
    /*!  When scanning with a small bit depth, banding artifacts may
         result.  Applying a dither pattern to the raw image data is
         a common approach to make these artifacts less pronounced.
         This command allows one to set up to two custom patterns.

         \note  The dither pattern is used when \ref
                set_halftone_processing has been set to use one of its
                two custom patterns.

         \note  The initialize command does \e not reset the dither
                patterns.

         \sa set_halftone_processing, set_bit_depth,
             http://en.wikipedia.org/wiki/Dither#Digital_photography_and_image_processing,
             http://en.wikipedia.org/wiki/Ordered_dithering
     */
    class set_dither_pattern : public command
    {
    public:
      enum {
        CUSTOM_A = 0x00,
        CUSTOM_B = 0x01,
      };

      set_dither_pattern (void);
      set_dither_pattern (const set_dither_pattern& s);

      ~set_dither_pattern (void);

      set_dither_pattern&
      operator= (const set_dither_pattern& s);

      void operator>> (connexion& cnx);

      //!  Set a default dither \a pattern.
      set_dither_pattern& operator() (byte pattern);

      //!  Set a custom dither \a pattern.
      /*!  Custom dither patterns of different sizes may be set but
           the supported sizes differ between devices.  When trying
           to set a pattern of unsupported size, the device expects
           data for a 16x16 pattern.
       */
      template <size_t size>
      set_dither_pattern& operator() (byte pattern,
                                      const matrix<uint8_t,size>& mat)
      {
        BOOST_STATIC_ASSERT (4 == size || 8 == size || 16 == size);

        rep_ = 0;

        streamsize required = 2 + size * size;

        if (dat_size_ < required)
          {
            delete [] dat_;
            dat_ = new byte [required];
            dat_size_ = required;
          }

        dat_[0] = pattern;
        dat_[1] = size;

        for (size_t i = 0; i < mat.rows (); ++i)
          for (size_t j = 0; j < mat.cols (); ++j)
            dat_[2 + i * mat.cols () + j] = mat[i][j];

        return *this;
      }

    protected:
      static const byte cmd_[2]; //!<  command bytes
      byte       rep_;           //!<  reply byte
      byte      *dat_;           //!<  data block
      streamsize dat_size_;      //!<  data block byte size

      //!  Returns the number of payload bytes.
      streamsize dat_size () const;

      //!  \copybrief setter::validate_cmd_reply
      /*!  \copydetails setter::validate_cmd_reply
       */
      virtual void validate_cmd_reply (void) const;

      //!  \copybrief setter::validate_dat_reply
      /*!  \copydetails setter::validate_dat_reply
       */
      virtual void validate_dat_reply (void) const;
    };

  } // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_set_dither_pattern_hpp_ */
