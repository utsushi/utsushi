//  decorator.hpp -- design pattern template implementation
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

#ifndef utsushi_pattern_decorator_hpp_
#define utsushi_pattern_decorator_hpp_

#include "../memory.hpp"

namespace utsushi {

//!  Add responsibilities to an object of type \a T
/*!  This template implements the Decorator pattern in very minimal
 *   form.  Template specialisations need to implement the \em full
 *   \em public API of their base class \a T as well as provide any
 *   specialised constructors.
 *
 *   \see http://en.wikipedia.org/wiki/Decorator_pattern
 */
template< typename T >
class decorator
  : public T
{
public:
  typedef shared_ptr< T > ptr;

  decorator (ptr instance)
    : instance_(instance)
  {}

protected:
  typedef decorator base_;

  ptr instance_;
};

}       // namespace utsushi

#endif  /* utsushi_pattern_decorator_hpp_ */
