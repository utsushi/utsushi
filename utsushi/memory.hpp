//  memory.hpp -- wrapper for managed memory pointers
//  Copyright (C) 2012-2015  SEIKO EPSON CORPORATION
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

#ifndef utsushi_memory_hpp_
#define utsushi_memory_hpp_

/*! \file
 *  \brief Inject a standard compliant managed memory pointers
 *
 *  C++ acquired \c std::shared_ptr and \c std::weak_ptr templates
 *  with TR1 [1] and their functionality was aligned to the Boost
 *  version with C++11 [2].  The compiler and standard library
 *  implementations may need some time to catch up with this.  In
 *  the mean time, we still would like to use \c shared_ptr and \c
 *  weak_ptr as if it were part of the \c utsushi namespace.  We do
 *  not want to worry about whether we are using the \c std:: or \c
 *  boost:: version.  This header file lets us.
 *
 *  -# http://wikipedia.org/wiki/C++_Technical_Report_1
 *  -# http://wikipedia.org/wiki/C++11
 */

#if __cplusplus >= 201103L && !WITH_INCLUDED_BOOST

#include <memory>
#define NAMESPACE std

#else   /* emulate C++11 */

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#define NAMESPACE boost

#endif

namespace utsushi {

using NAMESPACE::dynamic_pointer_cast;
using NAMESPACE::static_pointer_cast;
using NAMESPACE::make_shared;
using NAMESPACE::shared_ptr;
using NAMESPACE::weak_ptr;

//! Support conversion to \c shared_ptr<T> for pointers that aren't
/*! Sometimes API requirements dictate the use of a \c shared_ptr<T>.
 *  Naively converting a raw pointer to a shared one will result in
 *  deletion of that raw pointer when the shared pointer goes out of
 *  scope.  To prevent this from happening, just pass a null_deleter
 *  with the raw pointer to the \c shared_ptr<T> constructor.
 */
struct null_deleter
{
  void operator() (const void *) const {}
};

}       // namespace utsushi

#undef NAMESPACE

#endif  /* utsushi_memory_hpp_ */
