//  buffer.hpp -- byte sequences for the ESC/I "compound" protocol
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

#ifndef drivers_esci_buffer_hpp_
#define drivers_esci_buffer_hpp_

#include <iterator>
#include <string>

#include "code-point.hpp"

namespace utsushi {
namespace _drv_ {
namespace esci {

template< typename T >
class basic_buffer
  : private std::basic_string< T >
{
  typedef std::basic_string< T > base;
public:
  using typename base::const_iterator;
  using typename base::iterator;
  using typename base::const_reference;
  using typename base::reference;
  using typename base::size_type;
  using typename base::value_type;

  basic_buffer ()
    : base ()
  {}

  basic_buffer (const T *buf)
    : base (buf)
  {}

  basic_buffer (const typename base::size_type& sz)
    : base ()
  {
    base::reserve (sz);
  }

  bool
  operator== (const basic_buffer& that) const
  {
    return static_cast< base > (*this) == static_cast< base > (that);
  }

  using base::begin;
  using base::end;
  using base::reserve;
  using base::resize;
  using base::clear;
  using base::push_back;
  using base::assign;
  using base::size;
  using base::find;
  using base::empty;

  const T * data () const
  {
    return base::data ();
  }

  T * data ()
  {
    return const_cast< T * > (base::data ());
  }

  std::ostream& operator>> (std::ostream& os) const
  {
    return os << static_cast< base > (*this);
  }

  operator bool () const
  {
    return !base::empty ();
  }
};

template< typename T >
std::ostream&
operator<< (std::ostream& os, const basic_buffer< T >& buf)
{
  return buf >> os;
}

typedef basic_buffer< byte > byte_buffer;

namespace decoding {

//! Parser grammar's preferred way of moving on to the next byte
typedef byte_buffer::const_iterator default_iterator_type;

}

namespace encoding {

//! Generator grammar's preferred way of adding yet another byte
typedef std::back_insert_iterator< byte_buffer > default_iterator_type;

}

}       // namespace esci
}       // namespace _drv_
}       // namespace utsushi

#endif  /* drivers_esci_buffer_hpp_ */
