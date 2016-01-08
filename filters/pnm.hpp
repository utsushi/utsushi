//  pnm.hpp -- PNM image format support
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

#ifndef filters_pnm_hpp_
#define filters_pnm_hpp_

#include <utsushi/filter.hpp>

namespace utsushi {
namespace _flt_ {

//! Turn a sequence of image data into portable any map format
/*! The portable any map (PNM) family of formats include formats for
 *  bi-level (PBM), grey-scale (PGM) and color (PPM) images.
 *
 *  The canonical format specifications can be found at:
 *   - http://netpbm.sourceforge.net/doc/pbm.html
 *   - http://netpbm.sourceforge.net/doc/pgm.html
 *   - http://netpbm.sourceforge.net/doc/ppm.html
 *
 *  The PGM and PPM formats are specified in a light oriented way such
 *  that zero, the minimum sample value, means "light completely off"
 *  (\e i.e. black) and the maximum sample value to "light fully on".
 *  The PBM specification, however, is \e ink oriented and uses zero
 *  to mean "no ink" and one to mean "inked" (\e i.e. black).
 *
 *  The implementation automatically switches to the most appropriate
 *  format for each image in the sequence based on the stream context
 *  properties at the beginning of image.
 *
 *  \note  Only "raw" variants of the PNM formats are supported.  The
 *         "plain" variants are not supported.
 *
 *  \todo  Generalize to support sample value bit depths other than 8
 *         for PGM and PPM formats (primarily 16)
 *  \todo  Extend to support the PAM format as well?
 */
class pnm
  : public filter
{
public:
  streamsize write (const octet *data, streamsize n);

protected:
  void boi (const context& ctx);
};

}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_pnm_hpp_ */
