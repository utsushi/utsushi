//  exception.cpp -- extensions to the std::exception hierarchy
//  Copyright (C) 2013-2015  SEIKO EPSON CORPORATION
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "utsushi/exception.hpp"

using std::string;

namespace utsushi {

system_error::system_error ()
  : std::runtime_error ("")
  , ec_(no_error)
{}

system_error::system_error (error_code ec, const string& message)
  : std::runtime_error (message)
  , ec_(ec)
{}

system_error::system_error (error_code ec, const char *message)
  : std::runtime_error (message)
  , ec_(ec)
{}

const system_error::error_code&
system_error::code () const
{
  return ec_;
}

}       // namespace utsushi
