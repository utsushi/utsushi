//  format.hpp -- wrapper for type-safe string formatting support
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

#ifndef utsushi_format_hpp_
#define utsushi_format_hpp_

#ifdef BOOST_FORMAT_HPP
#error "Include this file before <boost/format.hpp> is included."
#endif

//  We need access to otherwise private parts of boost::basic_format
//  in order to support argument count checking in our logging code.

#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
#define BOOST_NO_MEMBER_TEMPLATE_FRIENDS
#endif

#include <boost/format.hpp>

namespace utsushi {

using boost::format;
using boost::wformat;

}       // namespace utsushi

#endif  /* utsushi_format_hpp_ */
