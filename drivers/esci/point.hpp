//  point.hpp -- class template
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

#ifndef drivers_esci_point_hpp_
#define drivers_esci_point_hpp_

#include "vector.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

  template< typename T >
  class point : public vector< T, 2 >
  {
  public:
    point () {}
    point (const T& x, const T& y)
    {
      this->at (0) = x;
      this->at (1) = y;
    }
    point (const vector< T, 2 >& v)
      : vector< T, 2 > (v)
    {}
    const T& x () const { return this->at (0); }
    const T& y () const { return this->at (1); }
    T& x () { return this->at (0); }
    T& y () { return this->at (1); }
  };

} // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_point_hpp_ */
