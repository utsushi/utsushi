//  matrix.hpp -- class template
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

#ifndef drivers_esci_matrix_hpp_
#define drivers_esci_matrix_hpp_

#include <algorithm>

#include <utsushi/functional.hpp>

#include "vector.hpp"

#if __cplusplus >= 201103L
#define NS(bind) std::bind
#define NSPH(_1) std::placeholders::_1
#else
#define NS(bind) bind
#define NSPH(_1) _1
#endif

namespace utsushi {
namespace _drv_ {
namespace esci {

  /*! \note  The matrix class template "inherits" all of the arithmetic
   *         operations courtesy of its base class.
   */
  template< typename T, std::size_t rows_, std::size_t cols_ = rows_ >
  class matrix
    : public vector< vector< T, cols_ >, rows_ >
  {
    typedef vector<  T , cols_ > row;
    typedef vector< row, rows_ > base;

  public:
    matrix (const T& t = T())
      : base (row(t))
    {}

    using base::operator=;

    matrix& operator*= (const T& t)
    {
      std::for_each (this->begin (), this->end (),
                     NS(bind) (&row::operator*=, NSPH(_1), t));
      return *this;
    }

    matrix& operator/= (const T& t)
    {
      std::for_each (this->begin (), this->end (),
                     NS(bind) (&row::operator/=, NSPH(_1), t));
      return *this;
    }

    matrix< T, cols_, rows_ >
    transpose () const
    {
      matrix< T, cols_, rows_ > rv;

      for (std::size_t i = 0; i < rows_; ++i)
        {
          for (std::size_t j = 0; j < cols_; ++j)
            {
              rv[j][i] = this->at (i)[j];
            }
        }
      return rv;
    }

    static std::size_t size ()
    {
      return rows_ * cols_;
    }

    static std::size_t rows ()
    {
      return rows_;
    }

    static std::size_t cols ()
    {
      return cols_;
    }
  };

  template< typename T, std::size_t rows_, std::size_t cols_,
            std::size_t n_ >
  matrix< T, rows_, cols_ >
  operator* (const matrix< T, rows_, n_ >& m1,
             const matrix< T, n_, cols_ >& m2)
  {
    matrix< T, rows_, cols_ > rv;

    for (std::size_t i = 0; i < rows_; ++i)
      {
        rv[i] = m2.transpose() * m1[i];
      }
    return rv;
  }

  template< typename T, std::size_t rows_, std::size_t cols_ >
  vector< T, rows_ >
  operator* (const matrix< T, rows_, cols_ >& m,
             const vector< T, cols_ >& v)
  {
    vector< T, rows_ > rv;

    for (std::size_t i = 0; i < rows_; ++i)
      {
        rv[i] = m[i] * v;
      }
    return rv;
  }

} // namespace esci
} // namespace _drv_
} // namespace utsushi

#ifdef NS
#undef NS
#endif
#ifdef NSPH
#undef NSPH
#endif

#endif  /* drivers_esci_matrix_hpp_ */
