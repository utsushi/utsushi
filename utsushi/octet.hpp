//  octet.hpp -- type and trait definitions
//  Copyright (C) 2012, 2013, 2015  SEIKO EPSON CORPORATION
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

#ifndef utsushi_octet_hpp_
#define utsushi_octet_hpp_

#include <ios>
#include <string>

namespace utsushi {

//! A set of eight bits with no particular interpretation attached
/*! Although it is common to use plain \c char or a \c byte type for
 *  this purpose, the former has an interpretation attached and the
 *  latter does not necessarily consist of eight bits.
 *
 *  \sa  http://en.wikipedia.org/wiki/Octet_(computing)
 */
typedef char octet;

//! Traits extensions for use by image data producers and consumers
/*! The standard character traits only provide for an eof() sequence
 *  marker.  The utsushi streams can handle additional markers that
 *  provide for end of scan sequence and end of image type events.
 *  It is convenient to also cater to corresponding begin markers so
 *  that one can easily instrument any image processing object with
 *  header and footer type hooks.
 */
struct traits
  : std::char_traits< octet >
{
  //! Convert \a c to its equivalent integer representation
  /*! \note This is meant to work with both signed and unsigned octet
   *        types.
   */
  static int_type to_int_type (const char_type& c);

  //! Cancellation marker
  static int_type eof ();
  //! End of scan sequence marker
  static int_type eos ();
  //! End of image marker
  static int_type eoi ();
  //! Begin of image marker
  static int_type boi ();
  //! Begin of scan sequence marker
  static int_type bos ();
  static int_type bof ();

  //! Return a value different from any supported sequence marker
  /*! If \a i is not a sequence marker, \a i is returned.  Otherwise,
   *  some other value is returned.
   */
  static int_type not_marker (const int_type& i);

  //! Tell whether \a i corresponds to a sequence marker
  static bool is_marker (const int_type& i);
};

//! Signed integral type that can be used to count octets
using std::streamsize;

} // namespace utsushi

#endif /* utsushi_octet_hpp_ */
