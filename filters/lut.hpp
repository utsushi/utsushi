//  lut.hpp -- look-up table based filtering support
//  Copyright (C) 2012, 2014  SEIKO EPSON CORPORATION
//
//  License: GPL-3.0+
//  Author : AVASYS CORPORATION
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

#ifndef filters_lut_hpp_
#define filters_lut_hpp_

#include <utsushi/cstdint.hpp>
#include <utsushi/filter.hpp>

namespace utsushi {
namespace _flt_ {

class lut
  : public filter
{
public:
  typedef int64_t index_type;

  lut ();

  streamsize write (const octet *data, streamsize n);

protected:
  void boi (const context& ctx);
  void eoi (const context& ctx);

  virtual void init_lut ();
  index_type octets2index (const octet *o, streamsize n);
  void index2octets (octet *o, index_type i, streamsize n);

  index_type *lut_;
  index_type rows_;
  uint32_t opr_;                // octets/row
};


class bc_lut
  : public lut
{
public:
  typedef shared_ptr< bc_lut > ptr;

  bc_lut (double brightness = 0.0, double contrast = 0.0);

protected:
  void init_lut ();
};

}       // namespace _flt_
}       // namespace utsushi

#endif  /* filters_lut_hpp_ */
