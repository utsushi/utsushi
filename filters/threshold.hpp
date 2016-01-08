//  threshold.hpp -- apply a threshold to 8-bit grayscale data
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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

#ifndef filters_threshold_hpp_
#define filters_threshold_hpp_

#include <utsushi/cstdint.hpp>
#include <utsushi/filter.hpp>

namespace utsushi {
namespace _flt_ {

/*! Set all pixel component samples below a certain value to their
 *  minimum value and all other samples to their maximum.
 *
 *  \todo  Generalize to support an arbitrary number of components
 *  \todo  Generalize to support component depths other than eight
 */
class threshold
  : public filter
{
public:
  threshold ();

  streamsize write (const octet *data, streamsize n);

protected:
  void boi (const context& ctx);

  unsigned char threshold_;

  static streamsize
  filter (const octet *in_data, octet *out_data, streamsize n,
          streamsize ppl, unsigned char threshold);

  static void
  set_bit (octet *data, streamsize bit_index, bool is_below);
};

}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_threshold_hpp_ */
