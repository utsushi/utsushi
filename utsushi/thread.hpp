//  thread.hpp -- wrapper for concurrent programming
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

#ifndef utsushi_thread_hpp_
#define utsushi_thread_hpp_

/*! \file
 *  \brief Inject a standard compliant \c thread class
 *
 *  C++ acquired a \c std::thread class with C++11 [1].  Compiler and
 *  standard library implementations may need some time to catch up
 *  with these developments.  Boost.Thread is supposed to be standard
 *  compliant so we can use that if the target platform is not yet up
 *  to snuff.  All the same, we do not want to worry about whether we
 *  are using a \c std::thread or a \c boost::thread anywhere in the
 *  \c utsushi namespace.  This header file lets us.
 *
 *  -# http://wikipedia.org/wiki/C++11
 */

#if __cplusplus >= 201103L && !WITH_INCLUDED_BOOST

#include <thread>
#define NAMESPACE std

#else   /* emulate C++11 */

#include <boost/thread/thread.hpp>
#define NAMESPACE boost

#endif

namespace utsushi {

using NAMESPACE::thread;
namespace this_thread = NAMESPACE::this_thread;

}       // namespace utsushi

#undef NAMESPACE

#endif  /* utsushi_thread_hpp_ */
