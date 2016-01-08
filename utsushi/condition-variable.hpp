//  condition-variable.hpp -- wrapper for concurrency synchronization
//  Copyright (C) 2013, 2015  SEIKO EPSON CORPORATION
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

#ifndef utsushi_condition_variable_hpp_
#define utsushi_condition_variable_hpp_

/*! \file
 *  \brief Inject standard compliant synchronization support
 *
 *  C++ acquired a \c std::condition_variable classes with C++11 [1].
 *  Compiler and standard library implementations may need some time
 *  to catch up with these developments.  Boost.Thread is supposed to
 *  be standards compliant enough for our (current) needs so we can
 *  use that if the target platform is not yet up to snuff.  All the
 *  same, we do not want to worry about whether we are using the \c
 *  std or a \c boost class anywhere in the \c utsushi namespace.
 *  This header file lets us.
 *
 *  -# http://wikipedia.org/wiki/C++11
 */

#if __cplusplus >= 201103L && !WITH_INCLUDED_BOOST

#include <condition_variable>
#define NAMESPACE std

#else   /* emulate C++11 */

#include <boost/thread/condition_variable.hpp>
#define NAMESPACE boost

#endif

namespace utsushi {

using NAMESPACE::condition_variable;

}       // namespace utsushi

#undef NAMESPACE

#endif  /* utsushi_condition_variable_hpp_ */
