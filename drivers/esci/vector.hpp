//  vector.hpp -- class template
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

#ifndef drivers_esci_vector_hpp_
#define drivers_esci_vector_hpp_

#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>

#include <boost/operators.hpp>

#include <utsushi/array.hpp>

namespace utsushi {
namespace _drv_ {
namespace esci {

  /*! \todo  Add a scalable, easy-to-use construction/initialization
   *         mechanism.  C++0x will provide std:initializer_list<T>,
   *         meaning one could use {} initialization.  Boost has the
   *         Assign library which may be useful.  Another idea is to
   *         rely on Boost.MPL to generate constructors that take up
   *         to size_ T objects (not sure that's possible though).
   */
  template< typename T, std::size_t size_ >
  class vector
    : protected array< T, size_ >
    , boost::equality_comparable< vector< T, size_ >
    , boost::additive< vector< T, size_ >
    , boost::multiplicative< vector< T, size_ >, T
    > > >
  {
    typedef array< T, size_ > base;

  public:
    vector (const T& t = T())
    {
      std::fill_n (this->begin (), size_, t);
    }

    using typename base::size_type;
    using base::operator=;

    bool operator== (const vector& v) const
    {
      return std::equal (this->begin (), this->end (), v.begin ());
    }

    vector& operator+= (const vector& v)
    {
      std::transform (this->begin (), this->end (), v.begin (), this->begin (),
                      std::plus<T> ());
      return *this;
    }

    vector& operator-= (const vector& v)
    {
      std::transform (this->begin (), this->end (), v.begin (), this->begin (),
                      std::minus<T> ());
      return *this;
    }

    vector& operator*= (const T& t)
    {
      std::transform (this->begin (), this->end (), this->begin (),
                      std::bind2nd (std::multiplies<T> (), t));
      return *this;
    }

    vector& operator/= (const T& t)
    {
      std::transform (this->begin (), this->end (), this->begin (),
                      std::bind2nd (std::divides<T> (), t));
      return *this;
    }

    T operator* (const vector& v) const
    {
      return std::inner_product (this->begin (), this->end (), v.begin (),
                                 T());
    }

    using base::operator[];

    static std::size_t size ()
    {
      return size_;
    }
  };

  template< typename T, std::size_t size_ >
  std::ostream& operator<< (std::ostream& os, const vector< T, size_ >& v)
  {
    os << "(";
    if (0 < size_)
      {
        os << v[0];
        for (size_t i = 1; i < size_; ++i)
          {
            os << ", " << v[i];
          }
      }
    os << ")";
    return os;
  }

} // namespace esci
} // namespace _drv_
} // namespace utsushi

#endif  /* drivers_esci_vector_hpp_ */
