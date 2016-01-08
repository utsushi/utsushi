//  signal.hpp -- wrapper for signal/slot support
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

#ifndef utsushi_signal_hpp_
#define utsushi_signal_hpp_

/*! \file
 *  \brief Wrap the most commonly used Boost.Signals2 API
 */

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>

namespace utsushi {

using boost::signals2::connection;
using boost::signals2::signal;

}       // namespace utsushi

#endif  /* utsushi_signal_hpp_ */
