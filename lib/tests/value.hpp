//  value.hpp -- bounded type fixture templates
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

#ifndef lib_tests_value_hpp_
#define lib_tests_value_hpp_

#include <vector>

#include <boost/lexical_cast.hpp>

#include "utsushi/key.hpp"
#include "utsushi/option.hpp"

//  Common bits of all bounded type fixtures
template <typename T>
struct bounded_type_fixture_base
  : public std::vector< T >
{
  typedef typename std::vector< T >::size_type size_type;

  static const std::string key_;

  bounded_type_fixture_base (const size_type& size = 5)
    : std::vector< T > (size)
  {}

  utsushi::key key (const size_type& i) const
  {
    return key_ + boost::lexical_cast< std::string > (i);
  }

  //  For symmetry with key ()
  const T& value (const size_type& i) const
  {
    return this->operator[] (i);
  }
};

template <typename T>
const std::string
bounded_type_fixture_base< T >::key_ = typeid (T).name ();

//  Bounded type fixture template and specializations

template <typename T>
struct bounded_type_fixture
  : bounded_type_fixture_base< T >
{
  typedef typename bounded_type_fixture_base< T >::size_type size_type;

  bounded_type_fixture (const size_type size) {};
};

template <>
struct bounded_type_fixture< utsushi::quantity >
  : bounded_type_fixture_base< utsushi::quantity >
{
  bounded_type_fixture (const size_type size = 10)
    : bounded_type_fixture_base< utsushi::quantity > (size)
  {
    for (size_type i = 0; i < size; ++i)
      {
        (*this)[i] = int (i);
        ++i;
        (*this)[i] = 0.5 * i;
      }
  }
};

template <>
struct bounded_type_fixture< utsushi::string >
  : bounded_type_fixture_base< utsushi::string >
{
  bounded_type_fixture ()
  {
    for (size_type i = 0; i < size (); ++i)
      {
        (*this)[i] = std::string (i, 'x');
      }
  }
};

template <>
struct bounded_type_fixture< utsushi::toggle >
  : bounded_type_fixture_base< utsushi::toggle >
{
  bounded_type_fixture (const size_type& size = 2)
    : bounded_type_fixture_base< utsushi::toggle > (size)
  {
    for (size_type i = 0; i < size; ++i)
      {
        (*this)[i] = (0 == i % 2);
      }
  }
};

#endif  /* lib_tests_value_hpp_ */
