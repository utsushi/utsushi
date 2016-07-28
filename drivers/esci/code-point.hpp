//  code-point.hpp -- set used by the ESC/I protocols
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

#ifndef drivers_esci_code_point_hpp_
#define drivers_esci_code_point_hpp_

#include <utsushi/cstdint.hpp>
#include <utsushi/octet.hpp>

namespace utsushi {
namespace _drv_ {
namespace esci {

//! Bit patterns in groups of eight.
/*! The ESC/I protocol documentation refers to the 8-bit patterns that
 *  make up the protocol as bytes throughout.  For consistency's sake,
 *  the driver implementation uses the same name.
 *
 *  \sa code_point
 */
typedef utsushi::octet byte;

//! Documented code points of the ESC/I protocols.
/*! The ESC/I protocols associate a number of 8-bit patterns (bytes)
 *  with special symbols to make it easier to document and remember.
 *  The code_point namespace defines readability constants for all
 *  known patterns.  The current set is a proper subset of US-ASCII
 *  (one of the many ISO/IEC 646 variants) and UTF-8, as well as of
 *  the combination of JIS X 0201 and JIS X 0211.
 *
 *  For the gory details on some standardised character encodings,
 *  see:
 *   - http://en.wikipedia.org/wiki/Character_encoding
 *   - http://en.wikipedia.org/wiki/ISO/IEC_646
 *   - http://en.wikipedia.org/wiki/JIS_X_0201
 *   - http://en.wikipedia.org/wiki/JIS_X_0211
 *   - http://en.wikipedia.org/wiki/ISO/IEC_8859
 *   - http://en.wikipedia.org/wiki/ISO_6429
 *   - http://en.wikipedia.org/wiki/UTF-8
 *
 *  \rationale
 *  It is extremely tempting to use character constants for all but
 *  the first few.  However, the language standard says that mapping
 *  of such constants to integer values is implementation defined.
 *  While most compilers will use ASCII (or UTF-8), there is no such
 *  guarantee and some compilers (\e e.g. GCC compilers) provide
 *  options to specify the input file and execution character sets
 *  that are involved in the mapping process.
 *
 *  \note For ease of use \e all constants defined in the code_point
 *        namespace are imported into the esci namespace.  Constants
 *        can be used without a code_point namespace qualification.
 */
namespace code_point {

  const byte STX = 0x02;        //!< start of text
  const byte EOT = 0x04;        //!< end of transmission
  const byte ACK = 0x06;        //!< acknowledge
  const byte FF  = 0x0c;        //!< form feed
  const byte NAK = 0x15;        //!< negative acknowledge
  const byte CAN = 0x18;        //!< cancel
  const byte PF  = 0x19;        //!< end of medium; \c EM
  const byte ESC = 0x1b;        //!< escape
  const byte FS  = 0x1c;        //!< file separator

  const byte SPACE   = 0x20;
  const byte EXCLAM  = 0x21;
  const byte NUMBER  = 0x23;
  const byte PAREN_L = 0x28;
  const byte PAREN_R = 0x29;
  const byte MINUS   = 0x2d;
  const byte PERIOD  = 0x2e;
  const byte SLASH   = 0x2f;

  const byte DIGIT_0 = 0x30;
  const byte DIGIT_1 = 0x31;
  const byte DIGIT_2 = 0x32;
  const byte DIGIT_3 = 0x33;
  const byte DIGIT_4 = 0x34;
  const byte DIGIT_5 = 0x35;
  const byte DIGIT_6 = 0x36;
  const byte DIGIT_7 = 0x37;
  const byte DIGIT_8 = 0x38;
  const byte DIGIT_9 = 0x39;

  const byte AT_MARK = 0x40;

  const byte UPPER_A = 0x41;
  const byte UPPER_B = 0x42;
  const byte UPPER_C = 0x43;
  const byte UPPER_D = 0x44;
  const byte UPPER_E = 0x45;
  const byte UPPER_F = 0x46;
  const byte UPPER_G = 0x47;
  const byte UPPER_H = 0x48;
  const byte UPPER_I = 0x49;
  const byte UPPER_J = 0x4a;
  const byte UPPER_K = 0x4b;
  const byte UPPER_L = 0x4c;
  const byte UPPER_M = 0x4d;
  const byte UPPER_N = 0x4e;
  const byte UPPER_O = 0x4f;
  const byte UPPER_P = 0x50;
  const byte UPPER_Q = 0x51;
  const byte UPPER_R = 0x52;
  const byte UPPER_S = 0x53;
  const byte UPPER_T = 0x54;
  const byte UPPER_U = 0x55;
  const byte UPPER_V = 0x56;
  const byte UPPER_W = 0x57;
  const byte UPPER_X = 0x58;
  const byte UPPER_Y = 0x59;
  const byte UPPER_Z = 0x5a;

  const byte BRACKET_L = 0x5b;
  const byte BRACKET_R = 0x5d;

  const byte LOWER_A = 0x61;
  const byte LOWER_B = 0x62;
  const byte LOWER_C = 0x63;
  const byte LOWER_D = 0x64;
  const byte LOWER_E = 0x65;
  const byte LOWER_F = 0x66;
  const byte LOWER_G = 0x67;
  const byte LOWER_H = 0x68;
  const byte LOWER_I = 0x69;
  const byte LOWER_L = 0x6c;
  const byte LOWER_M = 0x6d;
  const byte LOWER_N = 0x6e;
  const byte LOWER_O = 0x6f;
  const byte LOWER_P = 0x70;
  const byte LOWER_Q = 0x71;
  const byte LOWER_R = 0x72;
  const byte LOWER_S = 0x73;
  const byte LOWER_T = 0x74;
  const byte LOWER_W = 0x77;
  const byte LOWER_X = 0x78;
  const byte LOWER_Y = 0x79;
  const byte LOWER_Z = 0x7a;

  //! An unused code_point for unit testing purposes
  /*! It may be used to check for out-of-bounds reads and writes in
   *  byte buffers in unit test implementations.  Due to its nature,
   *  the value of this constant is an implementation detail.
   */
  const byte HEDGE = 0x99;

}       // namespace code_point

// Import all code_point constants into the esci namespace for ease
// of use.  The namespace itself is mostly a code documentation aid.

using namespace code_point;

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_code_point_hpp_ */
