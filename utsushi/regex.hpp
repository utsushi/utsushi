//  regex.hpp -- wrapper for regular expression support
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

#ifndef utsushi_regex_hpp_
#define utsushi_regex_hpp_

/*! \file
 *  \brief Inject standard compliant \c regex API
 *
 *  C++ acquired standardized regular expression support with C++11
 *  [1].  Compiler and standard library implementations may need some
 *  time to catch up with these developments.  Boost.Regex is supposed
 *  to be standard compliant so we can use that if the target platform
 *  is not yet up to snuff.  All the same, we do not want to worry
 *  about whether we are using \c std::regex or \c boost::regex API
 *  anywhere in the \c utsushi namespace.  This header file lets us.
 *
 *  -# http://wikipedia.org/wiki/C++11
 */

#if __cplusplus >= 201103L && !WITH_INCLUDED_BOOST

#include <regex>
#define NAMESPACE std

#else   /* emulate C++11 */

#include <boost/regex.hpp>
#define NAMESPACE boost

#endif

namespace utsushi {

using NAMESPACE::cmatch;
using NAMESPACE::regex;
using NAMESPACE::regex_match;
using NAMESPACE::sregex_iterator;
using NAMESPACE::smatch;

}       // namespace utsushi

#undef NAMESPACE

#endif  /* utsushi_regex_hpp_ */
