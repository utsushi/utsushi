//  g3fax.hpp -- convert scanlines to G3 facsimile format
//  Copyright (C) 2012, 2014, 2015  SEIKO EPSON CORPORATION
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

#ifndef filters_g3fax_hpp_
#define filters_g3fax_hpp_

#include <boost/scoped_array.hpp>

#include <utsushi/filter.hpp>

namespace utsushi {
namespace _flt_ {

//! Convert bi-level image data to FAX G31D encoded data
/*! The G31D encoding is part of the ITU-T T.4 standard for facsimile
 *  transmission of black-and-white images.  Note that the original
 *  standard issuing body, CCITT, was renamed to ITU-T in 1993.  You
 *  may still find plenty references to "CCITT" when the standard is
 *  discussed.
 *
 *  For a good description of the G31D and related encodings see:
 *   - http://www.fileformat.info/mirror/egff/ch09_05.htm

 *  For references to the code tables, see for example:
 *   - http://www.iet.unipi.it/m.luise/HTML/SdT/10_4%20Modified%20Huffman%20Coding.htm
 *   - http://www.windsurfnow.co.uk/imedit/ModHuffman.html
 *  Note that these references seem to disagree and which tables are
 *  used in the implementation has not been checked.
 */
class g3fax
  : public filter
{
public:
  /*! Image \a data need not be aligned on an eight pixel boundary but
   *  note that scanline data should be aligned on octet boundaries.
   *  This means that up to seven "padding" bits per scanline may be
   *  present in the \a data.  Such image data padding is not included
   *  in the encoded result.
   *
   *  That is, for a scanline length (image width) of 850 pixels the
   *  encoder consumes 107 octets but ignores the last six bits.  The
   *  resulting encoded scanline will start with a code for a run of
   *  zero or more white pixels and then continues with codes for
   *  alternating runs of black and white pixels.  Each scanline is
   *  terminated with a special EOL code (consisting of eleven zero
   *  bits followed by a single one bit).
   *
   *  The \a data octets are interpreted from their most significant
   *  bit towards the least significant bit.  Bits are assumed to be
   *  light intensities so a value of zero indicates black.  This is
   *  in line with the light oriented nature of scanners.
   */
  streamsize write (const octet *data, streamsize n);

protected:
  void boi (const context& ctx);
  void eoi (const context& ctx);

  boost::scoped_array< octet > partial_line_;
  streamsize                   partial_size_;

private:
  streamsize skip_pbm_header_(const octet *& data, streamsize n);
  bool pbm_header_seen_;
  bool is_light_based_;
};

}       // namespace _flt_
}       // namespace utsushi

#endif /* filters_g3fax_hpp_ */
