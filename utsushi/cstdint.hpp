//  cstdint.hpp -- wrapper for standard fixed width integral types
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

#ifndef utsushi_cstdint_hpp_
#define utsushi_cstdint_hpp_

/*! \file
 *  \brief Inject standard compliant fixed width integral types
 *
 *  C++ officially acquired a \c cstdint header with C++11 [1], after
 *  being proposed in TR1 [2].  The various C++ compiler and standard
 *  library implementations may need some time to catch up with these
 *  developments.  In the mean time, we still would like to use some
 *  of these integral types as if they were part of the \c utsushi
 *  namespace.  We do not want to worry about whether we are using the
 *  standard's types, Boost's type or even the C99 types.  This header
 *  file lets us.
 *
 *  -# http://wikipedia.org/wiki/C++11
 *  -# http://wikipedia.org/wiki/C++_Technical_Report_1
 */

#if __cplusplus >= 201103L && !WITH_INCLUDED_BOOST

#include <cstdint>
#define NAMESPACE std

#else   /* emulate C++11 */

#include <boost/cstdint.hpp>
#define NAMESPACE boost

#endif

namespace utsushi {

using NAMESPACE::int8_t;
using NAMESPACE::int16_t;
using NAMESPACE::int32_t;
using NAMESPACE::int64_t;
using NAMESPACE::uint8_t;
using NAMESPACE::uint16_t;
using NAMESPACE::uint32_t;
using NAMESPACE::uint64_t;

}       // namespace utsushi

#undef NAMESPACE

#endif  /* utsushi_cstdint_hpp_ */
