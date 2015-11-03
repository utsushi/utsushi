//  functional.hpp -- wrapper for standard compliant reference wrappers
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

#ifndef utsushi_functional_hpp_
#define utsushi_functional_hpp_

/*! \file
 *  \brief Inject standard compliant reference wrappers
 *
 *  C++ acquired several reference wrapper and function object related
 *  API extensions with TR1 [1] based on their Boost implementations.
 *  The API was formally added to the C++11 standard [2] and the Boost
 *  implemetations have been aligned with the standard.  The compiler
 *  and standard library implementations may need some time to catch
 *  up with this.  In the mean time, we still would like to use those
 *  API additions as if it were part of the \c utsushi namespace.  We
 *  do not want to worry about whether we are using the \c std:: or \c
 *  boost:: version.  This header file lets us.
 *
 *  -# http://wikipedia.org/wiki/C++_Technical_Report_1
 *  -# http://wikipedia.org/wiki/C++11
 */

#if __cplusplus >= 201103L

#include <functional>
#define NAMESPACE std

#else   /* emulate C++11 */

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>
#define NAMESPACE boost

#endif

namespace utsushi {

using NAMESPACE::bind;
using NAMESPACE::function;
using NAMESPACE::ref;
using NAMESPACE::reference_wrapper;

}       // namespace utsushi

#undef NAMESPACE

#endif  /* utsushi_functional_hpp_ */
