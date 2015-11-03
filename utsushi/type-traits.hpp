//  type-traits.hpp -- wrapper for selected standard type traits API
//  Copyright (C) 2015  SEIKO EPSON CORPORATION
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

#ifndef utsushi_type_traits_hpp_
#define utsushi_type_traits_hpp_

/*! \file
 *  \brief Inject standard compliant type traits API
 *
 *  C++ grew additional type traits API [1].  Compiler and standard
 *  library implementations may need some time to catch up with these
 *  developments.  When not available, we rely on Boost.TypeTraits to
 *  fill the gap.  All the same, we do not want to worry about whether
 *  we are using \c std:: or \c boost:: type traits API anywhere in
 *  the \c utsushi namespace.  This header file lets us.
 *
 *  -# http://wikipedia.org/wiki/C++11
 */

#if __cplusplus >= 201103L

#include <type_traits>
#define NAMESPACE std

#else   /* emulate C++11 */

#include <boost/type_traits/is_floating_point.hpp>
#include <boost/type_traits/is_base_of.hpp>
#define NAMESPACE boost

#endif

namespace utsushi {

using NAMESPACE::is_base_of;
using NAMESPACE::is_floating_point;

}       // namespace utsushi

#undef NAMESPACE

#endif  /* utsushi_type_traits_hpp_ */
